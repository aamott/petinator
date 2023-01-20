/****************************************************
* The PETInator
*
██████╗ ███████╗████████╗██╗███╗   ██╗ █████╗ ████████╗ ██████╗ ██████╗
██╔══██╗██╔════╝╚══██╔══╝██║████╗  ██║██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
██████╔╝█████╗     ██║   ██║██╔██╗ ██║███████║   ██║   ██║   ██║██████╔╝
██╔═══╝ ██╔══╝     ██║   ██║██║╚██╗██║██╔══██║   ██║   ██║   ██║██╔══██╗
██║     ███████╗   ██║   ██║██║ ╚████║██║  ██║   ██║   ╚██████╔╝██║  ██║
╚═╝     ╚══════╝   ╚═╝   ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
* Generated with this fancy tool:
* - https://www.coolgenerator.com/ascii-text-generator
* Repository:
* - https://github.com/aamott/petinator
*
*****************************************************/

#include <AutoPID.h>
#include <thermistor.h>
#include <AccelStepper.h>
#include <ezButton.h>
#include <EEPROM.h>
#include "fastMenu.cpp"
// display libraries
#include <Wire.h>
#include <hd44780.h>

#ifdef I2C_LCD
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header
#else
#include <hd44780ioClass/hd44780_pinIO.h>  // Arduino pin i/o class header
#endif

#include "configuration.h"

/****************************************
 * Thermistor
 * Wiring: https://learn.adafruit.com/thermistor/using-a-thermistor
 *  Use a 4.7k resistor (or appropriate for your thermistor)
 *    Pin                  ==> Arduino Pin
 *    ------------------------------------
 *    Thermistor Negative  ==> Gnd
 *    Thermistor Positive  ==> A0
 *    Resistor (4.7K)      ==> A0
 *    Resistor (4.7K)      ==> Vcc (5v)
 */
unsigned long lastTempUpdate;  // tracks clock time of last temp update
double current_temp = 0;       // celsius
double last_temp = 0;          // celsius
thermistor temp_sensor(THERMISTOR_PIN, THERMISTOR_TYPE);

void updateTemperature() {
  if ((millis() - lastTempUpdate) > TEMP_READ_DELAY) {
    current_temp = temp_sensor.analog2temp();  // get temp reading
    lastTempUpdate = millis();
  }
}

/*******************
 * PID
 * pid settings and gains
 */
double target_temp = DEFAULT_TEMP;  // when enabled, temp_set_point will be set to this
double temp_set_point = 0;          // the value used by autopid
double heater_pwm = 0;              // heater's pwm will be controlled by this value
double last_pwm = 0;                // keeps track of the last value for less frequent updating
bool heatingEnabled = false;
bool error_thrown = false;
// thermal runaway
int recorded_temps[CHECKS_PER_PERIOD];  // keeps track of temps over THERMAL_PROTECTION_PERIOD seconds
int current_index = 0;                  // current write index for temps
int next_index = 0;                     // next index to write temp into/last index written to
bool cooling = false;
#define TEMP_CHECK_MS THERMAL_PROTECTION_PERIOD * 1000 / CHECKS_PER_PERIOD  // how many milliseconds between temperature checks
unsigned long last_thermal_check_time = 0;                                  // last time temp was checked for thermal errors
// PID timing
unsigned long last_PID_time = 0;  // last time PID was run

float KP = DEFAULT_KP, KI = DEFAULT_KI, KD = DEFAULT_KD;

// input/output variables passed by reference, so they are updated automatically
AutoPID heaterPID(&current_temp, &temp_set_point, &heater_pwm, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

void increase_temp() {
  if (target_temp + TEMP_VARIANCE < MAX_TEMP) {
    target_temp += TEMP_INCREMENT_SIZE;
    if (heatingEnabled) {
      temp_set_point = target_temp;
      reset_recorded_temps();
    }
  }
}

void decrease_temp() {
  target_temp -= TEMP_INCREMENT_SIZE;
  if (heatingEnabled) {
    temp_set_point = target_temp;
    reset_recorded_temps();
  }
}

void toggle_heater() {
  heatingEnabled = !heatingEnabled;
  if (heatingEnabled) {
    temp_set_point = target_temp;
  } else {
    disable_heater();
  }
}

void disable_heater() {
  heatingEnabled = false;
  temp_set_point = 0;
  digitalWrite(HEATER_PIN, 0);
  reset_recorded_temps();
}

void reset_recorded_temps() {
  for (int i = 0; i < CHECKS_PER_PERIOD; i++) {
    recorded_temps[i] = -1;
  }
}

void check_thermal_safety() {
  // Thermal runaway checks
  if (millis() - last_thermal_check_time > TEMP_CHECK_MS &&  // time to check again
      (current_temp < target_temp - TEMP_VARIANCE ||         // heating
       current_temp > target_temp + TEMP_VARIANCE)) {        // cooling
    last_thermal_check_time = millis();

    // switch between heating and cooling thermal fault detection
    if (current_temp < target_temp - TEMP_VARIANCE && cooling) {  // if below temp but not heating
      // set to heating mode
      cooling = false;
      // reset the temps array
      reset_recorded_temps();
    } else if (current_temp > target_temp + TEMP_VARIANCE && !cooling) {  // if above temp but not cooling
      // set to cooling mode
      cooling = true;
      // reset the temps array
      reset_recorded_temps();
    }

    // Move forward in the temps array
    current_index = next_index;
    next_index += 1;
    // check for wrapping
    if (next_index >= CHECKS_PER_PERIOD) {
      next_index = 0;
    }

    // Write current temp
    recorded_temps[current_index] = current_temp;
    int first_temp = recorded_temps[next_index];

    // Check for thermal runaway
    if (first_temp != -1) {              // -1 is uninitialized
      if (target_temp > current_temp) {  // heating up
        cooling = false;
        if (!cooling && current_temp - first_temp < THERMAL_PROTECTION_HYSTERESIS) {
          disable_heater();
          throw_error("Thermal Runaway!");
          error_thrown = true;
        }
      } else if (cooling && first_temp - current_temp < THERMAL_PROTECTION_HYSTERESIS) {
        // target_temp < current_temp -- cooling down
        cooling = true;
        disable_heater();
        throw_error("Cooling Failed!");
        error_thrown = true;
      }
    }
  }

  // Overheat checks
  if (current_temp > MAX_TEMP) {
    disable_heater();
    throw_error("Max Temp Hit!");
    error_thrown = true;
  }
}

/// @brief Runs the heater. Returns true if the heater up to temp.
bool heater_loop() {
  // only throw an error once (until a reset)
  if (error_thrown) {
    return false;
  }

  updateTemperature();

  if (heatingEnabled) {
    // Update PID
    if (millis() - last_PID_time > PID_PERIOD) {
      heaterPID.run();
    }
    if (int(heater_pwm) != int(last_pwm)) {
      last_pwm = heater_pwm;
      analogWrite(HEATER_PIN, int(heater_pwm));
    }
    // light up LED when we're at setpoint +-1 degree
    digitalWrite(LED_BUILTIN, heaterPID.atSetPoint(1));

    // Thermal Protection Checks
    check_thermal_safety();

    return current_temp > target_temp - TEMP_VARIANCE;  // heated
  }

  return false;
}

/******************************************
 * Motor
 */
long target_speed = DEFAULT_SPEED;
bool pullingEnabled = false;

#ifdef USES_STEPPER
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

/***************************
 * Run Motor if Temp Reached
 * Callback to see if stepper should continue to run
 * or be shut off.
 */
void runMotorIfTempReached(bool at_temp) {

  stepper.runSpeed();

  if (pullingEnabled && at_temp) {
    // Temp is reached
    if (stepper.speed() == 0) {
      // only tell the stepper to run if it isn't already
      stepper.enableOutputs();
      stepper.setSpeed(target_speed);
    }
  } else {
    // Don't run stepper until temp is reached and enabled
    stepper.setSpeed(0);
  }
}

void increase_speed() {
  target_speed += SPEED_INC;
  if (target_speed > MAX_SPEED) {
    target_speed = MAX_SPEED;
  }
  if (stepper.speed() != 0) {
    stepper.setSpeed(target_speed);
  }
}

void decrease_speed() {
  target_speed -= SPEED_INC;
  if (target_speed < 0) {
    target_speed = 0;
  }
  if (stepper.speed() != 0) {
    stepper.setSpeed(target_speed);
  }
}

void toggle_puller() {
  pullingEnabled = !pullingEnabled;
  if (!pullingEnabled) stepper.setSpeed(0);
  else {
    stepper.setSpeed(target_speed);
    stepper.enableOutputs();
  }
}

void disable_puller() {
  pullingEnabled = false;
  stepper.setSpeed(0);
  stepper.disableOutputs();
}


#else  // end USES_STEPPER, start USES_PWM_MOTOR
bool motor_running = false;
/***************************
 * Run Motor if Temp Reached
 */
void runMotorIfTempReached(bool at_temp) {
  if (pullingEnabled && at_temp) {
    // Temp is reached
    if (!motor_running) {
      // only tell the motor to run if it isn't already
      motor_running = true;
      analogWrite(MOTOR_PWM_PIN, target_speed);
      digitalWrite(ENABLE_PIN, 0);
    }
  } else if (motor_running) {
    // Don't run motor until temp is reached
    motor_running = false;
    digitalWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(ENABLE_PIN, 1);
  }
}

void increase_speed() {
  target_speed += SPEED_INC;
  if (target_speed > MAX_SPEED) {
    target_speed = MAX_SPEED;
  }
  if (motor_running) {
    analogWrite(MOTOR_PWM_PIN, target_speed);
  }
}

void decrease_speed() {
  target_speed -= SPEED_INC;
  if (target_speed < 0) {
    target_speed = 0;
  }
  if (motor_running) {
    analogWrite(MOTOR_PWM_PIN, target_speed);
  }
}

void toggle_puller() {
  if (pullingEnabled) {
    pullingEnabled = false;
  } else {
    pullingEnabled = true;
  }
}
// END USES_PWM_MOTOR
#endif

/******************************************
 * EEPROM
 */
bool saved_status = false;

// variable addresses - each one needs enough space for itself,
// hence the use of sizeof(previous variable)
const int initialized_address = 0;
const int TtAddress = initialized_address + sizeof(int(EEPROM_INITIALIZED_SIGN));
const int KpAddress = TtAddress + sizeof(target_temp);
const int KiAddress = KpAddress + sizeof(KP);
const int KdAddress = KiAddress + sizeof(KI);
const int TsAddress = KdAddress + sizeof(KD);

bool eeprom_initialized() {
  int sign;
  EEPROM.get(initialized_address, sign);
  return sign == EEPROM_INITIALIZED_SIGN;
}

void InitializeEeprom() {
  EEPROM.put(initialized_address, EEPROM_INITIALIZED_SIGN);
  EEPROM_writeDouble(TtAddress, DEFAULT_TEMP);
  EEPROM_writeDouble(KpAddress, DEFAULT_KP);
  EEPROM_writeDouble(KiAddress, DEFAULT_KI);
  EEPROM_writeDouble(KdAddress, DEFAULT_KD);
  EEPROM_writeDouble(TsAddress, DEFAULT_SPEED);
}

void SaveParameters() {
  if (target_temp != EEPROM_readDouble(TtAddress)) {
    EEPROM_writeDouble(TtAddress, target_temp);
  }
  if (KP != EEPROM_readDouble(KpAddress)) {
    EEPROM_writeDouble(KpAddress, KP);
  }
  if (KI != EEPROM_readDouble(KiAddress)) {
    EEPROM_writeDouble(KiAddress, KI);
  }
  if (KD != EEPROM_readDouble(KdAddress)) {
    EEPROM_writeDouble(KdAddress, KD);
  }
  if (target_speed != EEPROM_readDouble(TsAddress)) {
    EEPROM_writeDouble(TsAddress, target_speed);
  }
  saved_status = true;
}

void LoadParameters() {
  if (eeprom_initialized()) {
    // Load from EEPROM
    target_temp = EEPROM_readDouble(TtAddress);
    KP = EEPROM_readDouble(KpAddress);
    KI = EEPROM_readDouble(KiAddress);
    KD = EEPROM_readDouble(KdAddress);
    target_speed = EEPROM_readDouble(TsAddress);
  } else {
    target_temp = DEFAULT_TEMP;
    KP = DEFAULT_KP;
    KI = DEFAULT_KI;
    KD = DEFAULT_KD;
    target_speed = DEFAULT_SPEED;
  }
}

void EEPROM_writeDouble(int address, double value) {
  byte *p = (byte *)(void *)&value;
  for (unsigned int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address++, *p++);
  }
}

double EEPROM_readDouble(int address) {
  double value = 0.0;
  byte *p = (byte *)(void *)&value;
  for (unsigned int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(address++);
  }
  return value;
}

/******************************************
 * Buttons
 */
long last_press_time = 0;

ezButton up_btn(UP_BTN);
ezButton select_btn(SELECT_BTN);
ezButton down_btn(DOWN_BTN);

enum ControlState : uint8_t {
  CTRL_SET_SCREEN = 0,   // in this state the encoder cycles the screens
  CTRL_SET_LINE = 1,     // in this state it cycles the focus through the lines
  CTRL_SET_SETTING = 2,  // in this state it "increases or decreases" the value of a setting
  CTRL_OUT_OF_BOUNDS,    // this is automatically set to 3
};

ControlState controlState = CTRL_SET_LINE;

/****************************************
 * DISPLAY
 * Wiring: https://create.arduino.cc/projecthub/Hack-star-Arduino/learn-to-use-lcd-1602-i2c-parallel-with-arduino-uno-f73f07
 *    i2c LCD Module  ==>   Arduino Pin
 *    SCL             ==>     A5
 *    SDA             ==>     A4
 *    Vcc             ==>     Vcc (5v)
 *    Gnd             ==>     Gnd
 */
// makes sure the display doesn't update too fast
unsigned long last_update = 0;

#ifdef I2C_LCD
#ifdef LCD_ADDRESS
hd44780_I2Cexp lcd(LCD_ADDRESS, COLUMNS, ROWS);  // if the address 0x27 doesn't work, try 0x3f
#else
hd44780_I2Cexp lcd;  // declare lcd object: auto locate & auto config expander chip
#endif

#else

hd44780_pinIO lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
/* Wiring:
 *  LCD Module  ==>   Arduino Pin
 *  D7  ==> 0
 *  D6  ==> 1
 *  D5  ==> 2
 *  D4  ==> 3
 *  E   ==> 4
 *  RW  ==> Gnd
 *  RS  ==> 5 (5v)
 */
#endif

// Status
FastLine<double> actual_temp_line(0, 0, "Temp(C): ", current_temp);
FastLine<long> actual_speed_line(0, 1, "Speed: ", target_speed);  // current_speed will fluctuate a lot. Better to show target speed.

// Edit
FastLine<double> set_temp_line(0, 2, "Set Temp(C): ", target_temp);
FastLine<long> set_speed_line(0, 3, "Set Speed: ", target_speed);

// Enable
FastLine<bool> enable_heater_line(0, 4, "Start Heater: ", heatingEnabled);
FastLine<bool> enable_puller_line(0, 5, "Start Puller: ", pullingEnabled);

//  EEPROM
FastLine<bool> save_parameters_line(0, 6, "Save: ", saved_status);

FastScreen main_screen;
FastScreen error_screen;

FastMenu menu(lcd, COLUMNS, ROWS);


/*
* Displays an error message and stops pulling and heating
*/
void throw_error(const char *message) {
  FastLine<> error_line(0, 0, message);
  error_screen.add_line(error_line);
  menu.add_screen(error_screen);

  // switch to the newly created error screen
  menu.set_screen(1);

  // stop pulling and heating
  disable_heater();
  disable_puller();
}


/********************************
 * Setup
*/
void setup() {
  pinMode(THERMISTOR_PIN, INPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  lcd.begin(COLUMNS, ROWS);
  lcd.print(BOOT_MESSAGE);
  delay(BOOT_DELAY);
  lcd.clear();
  lcd.home();

  /***********
     * EEPROM
     */
  if (eeprom_initialized()) {
    LoadParameters();
  } else {
    InitializeEeprom();
  }
  heaterPID.setGains(KP, KI, KD);

  /***********
     * Temp
     */
  // if temperature is more than 4 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  heaterPID.setBangBang(BANG_BANG_RANGE);
  // set PID update interval
  heaterPID.setTimeStep(TEMP_READ_DELAY);
  // initialize the temp array
  reset_recorded_temps();


/***********
 * Puller
 */
#ifdef USES_STEPPER
  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false, false, true);
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.disableOutputs();

  stepper.setSpeed(target_speed);
  stepper.setAcceleration(ACCELERATION);
#else  // a DC motor is assumed
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, 1);  // disable motor
#endif

  /***********
     * Menu
     */
  set_temp_line.add_controls(NULL, increase_temp, decrease_temp);
  set_speed_line.add_controls(NULL, increase_speed, decrease_speed);
  enable_heater_line.add_controls(toggle_heater);
  enable_puller_line.add_controls(toggle_puller);
  save_parameters_line.add_controls(SaveParameters);

  // set_temp_line.set_decimalPlaces(0);
  // actual_temp_line.set_decimalPlaces(0);

  main_screen.add_line(enable_heater_line);
  main_screen.add_line(enable_puller_line);
  main_screen.add_line(set_temp_line);
  main_screen.add_line(set_speed_line);
  main_screen.add_line(save_parameters_line);
  main_screen.add_line(actual_temp_line);
  main_screen.add_line(actual_speed_line);
  menu.add_screen(main_screen);
}

void loop() {
  /*********
   * Temperature
   */
  bool heated = heater_loop();


  // check if motor should keep running. Motor won't run until temperature is reached.
  runMotorIfTempReached(heated);


  /***********
   * Menu
   */
  // skip menu updates and controls if an error has been thrown. Leave the error on display.
  if (error_thrown) {
    return;
  }

  up_btn.loop();
  select_btn.loop();
  down_btn.loop();

  if (millis() - last_update > MIN_DISPLAY_UPDATE_MILLIS) {
    last_update = millis();
    last_temp = current_temp;
    menu.update();
  }

  if (!up_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    last_press_time = millis();
    menu.up();
  }

  else if (!down_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    last_press_time = millis();
    menu.down();
  }

  else if (!select_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    last_press_time = millis();
    menu.select();
  }
}
