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
#include <LiquidMenu.h>
#include <LiquidCrystal.h>
#include <FastAccelStepper.h>
#include <ezButton.h>
#include <EEPROM.h>

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

bool updateTemperature() {
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

float KP = DEFAULT_KP, KI = DEFAULT_KI, KD = DEFAULT_KD;

// input/output variables passed by reference, so they are updated automatically
AutoPID heaterPID(&current_temp, &temp_set_point, &heater_pwm, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

void increase_temp() {
  if (target_temp + TEMP_VARIANCE < MAX_TEMP) {
    target_temp += TEMP_INCREMENT_SIZE;
    if (heatingEnabled) {
      temp_set_point = target_temp;
    }
  }
}

void decrease_temp() {
  target_temp -= TEMP_INCREMENT_SIZE;
  if (heatingEnabled) {
    temp_set_point = target_temp;
  }
}

void toggle_heater() {
  heatingEnabled = !heatingEnabled;
  if (heatingEnabled) {
    temp_set_point = target_temp;
  } else {
    temp_set_point = 0;
  }
}

void disable_heater() {
  heatingEnabled = false;
  temp_set_point = 0;
}

void heater_loop() {

  // only throw an error once (until a reset)
  if (error_thrown) {
    return;
  }

  // update the temperature and PID
  updateTemperature();
  heaterPID.run();

  // apply new PID value
  if (int(heater_pwm) != int(last_pwm)) {
    last_pwm = heater_pwm;
    analogWrite(HEATER_PIN, int(heater_pwm));
  }

  // light up LED when we're at setpoint +-1 degree
  digitalWrite(LED_BUILTIN, heaterPID.atSetPoint(1));

  // Check for heating errors
  // overheat
  if (current_temp > MAX_TEMP) {
    disable_heater();
    throw_error("Max Temp Hit!");
    error_thrown = true;
  } 
  // thermal overshoot
  else if (heatingEnabled && current_temp - target_temp > HEATER_OVERSHOOT) {
    disable_heater();
    throw_error("Heater Overshoot!");
    error_thrown = true;
  }

  // TODO: Implement thermal runaway protection to detect faulty thermistor
}

/******************************************
 * Motor
 */
long target_speed = DEFAULT_SPEED;
bool pullingEnabled = false;

#ifdef USES_STEPPER
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

/***************************
 * Run Motor if Temp Reached
 * Callback to see if stepper should continue to run
 * or be shut off.
 * TODO: add thermal panic protection (temp suddenly drops,
 * goes above TEMP_VARIANCE bound, or doesn't follow heater
 * for too long)
 * then rename the function to remain descriptive
 */
void runMotorIfTempReached(double current_temp, double target_temp) {
  if (pullingEnabled && current_temp >= target_temp - TEMP_VARIANCE) {
    // Temp is reached
    if (!stepper->isRunning()) {
      // only tell the stepper to run if it isn't already
      stepper->runForward();
    }
  } else if (stepper->isRunning()) {
    // Don't run stepper until temp is reached
    stepper->stopMove();
  }
}

void increase_speed() {
  target_speed += SPEED_INC;
  if (target_speed > MAX_SPEED) {
    target_speed = MAX_SPEED;
  }
  if (stepper->isRunning()) {
    stepper->setSpeedInHz(target_speed);
    stepper->runForward();
  }
}

void decrease_speed() {
  target_speed -= SPEED_INC;
  if (target_speed < 0) {
    target_speed = 0;
  }
  if (stepper->isRunning()) {
    stepper->setSpeedInHz(target_speed);
    stepper->runForward();
  }
}

void toggle_puller() {

  // if(pullingEnabled) stepper->stopMove();
  pullingEnabled = !pullingEnabled;
}

void disable_puller() {
  pullingEnabled = false;
  stepper->stopMove();
}

#else  // end USES_STEPPER, start USES_PWM_MOTOR
bool motor_running = false;
/***************************
 * Run Motor if Temp Reached
 */
void runMotorIfTempReached(double current_temp, double target_temp) {
  if (pullingEnabled && current_temp >= target_temp - TEMP_VARIANCE) {
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
  if (eeprom_initialized) {
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
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address++, *p++);
  }
}

double EEPROM_readDouble(int address) {
  double value = 0.0;
  byte *p = (byte *)(void *)&value;
  for (int i = 0; i < sizeof(value); i++) {
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
 */
// makes sure the display doesn't update too fast
unsigned long last_update = 0;

#ifdef I2C_LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCD_ADDRESS, COLUMNS, ROWS);  // if the address 0x27 doesn't work, try 0x3f
                                                    /* Wiring:
                                                    *    i2c LCD Module  ==>   Arduino Pin
                                                    *    SCL             ==>     A5
                                                    *    SDA             ==>     A4
                                                    *    Vcc             ==>     Vcc (5v)
                                                    *    Gnd             ==>     Gnd      */
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
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

LiquidLine welcome_line1(4, 0, "Welcome");
LiquidLine welcome_line2(1, 1, "To Filamaker");
LiquidScreen welcome_screen(welcome_line1, welcome_line2);

// Status
LiquidLine actual_temp_line(0, 0, "Temp: ", current_temp, " C");
LiquidLine actual_speed_line(0, 1, "Speed: ", target_speed);  // current_speed will fluctuate a lot. Better to show current speed.
// LiquidScreen status_screen(actual_temp_line, actual_speed_line);

// Edit
LiquidLine set_temp_line(0, 2, "Set Temp: ", target_temp, " C");
LiquidLine set_speed_line(0, 3, "Set Speed: ", target_speed);
// LiquidScreen edit_screen(set_temp_line, set_speed_line);

// Enable
LiquidLine enable_heater_line(0, 4, "Start Heater: ", heatingEnabled);
LiquidLine enable_puller_line(0, 5, "Start Puller: ", pullingEnabled);
// LiquidScreen enable_screen(enable_heater_line, enable_puller_line, set_temp_line, set_speed_line, actual_temp_line, actual_speed_line);

//  EEPROM
LiquidLine save_parameters_line(0, 6, "Save: ", saved_status);

// LiquidLine start_line(0, 0, "Start: ", menu_message);
// LiquidScreen enable_screen(start_line);
LiquidScreen main_screen;
LiquidScreen error_screen;

LiquidMenu menu(lcd);


/*
* Displays an error message and stops pulling and heating
*/
template <typename A>
void throw_error(const A &message) {
  LiquidLine error_line(0, 0, message);
  error_screen.add_line(error_line);
  error_screen.set_displayLineCount(1);
  menu.add_screen(error_screen);

  // switch to the newly created error screen
  menu.change_screen(2);

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

#ifdef I2C_LCD
  lcd.init();
  lcd.backlight();
#endif

  lcd.begin(COLUMNS, ROWS);

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

/***********
 * Puller
 */
#ifdef USES_STEPPER
  engine.init();
  stepper = engine.stepperConnectToPin(STEP_PIN);
  if (stepper) {
    stepper->setDirectionPin(DIR_PIN);
    stepper->setEnablePin(ENABLE_PIN);
    stepper->setAutoEnable(true);

    stepper->setSpeedInHz(target_speed);
    stepper->setAcceleration(ACCELERATION);
  }
#else  // a DC motor is assumed
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, 1);  // disable motor
#endif

  /***********
     * Menu
     */
  set_temp_line.attach_function(1, increase_temp);
  set_temp_line.attach_function(2, decrease_temp);

  set_speed_line.attach_function(1, increase_speed);
  set_speed_line.attach_function(2, decrease_speed);

  enable_heater_line.attach_function(1, toggle_heater);

  enable_puller_line.attach_function(1, toggle_puller);
  // start_line.attach_function(1, toggle_running);

  save_parameters_line.attach_function(1, SaveParameters);

  set_temp_line.set_decimalPlaces(0);
  actual_temp_line.set_decimalPlaces(0);

  main_screen.add_line(enable_heater_line);
  main_screen.add_line(enable_puller_line);
  main_screen.add_line(set_temp_line);
  main_screen.add_line(set_speed_line);
  main_screen.add_line(save_parameters_line);
  main_screen.add_line(actual_temp_line);
  main_screen.add_line(actual_speed_line);
  main_screen.set_displayLineCount(2);
  menu.add_screen(main_screen);
}

void loop() {
  // check if motor should keep running. Motor won't run until temperature is reached.
  runMotorIfTempReached(current_temp, target_temp);

  up_btn.loop();
  select_btn.loop();
  down_btn.loop();

  /*********
   * Temperature
   */
  heater_loop();

  /***********
   * Menu
   */
  // skip menu updates and controls if an error has been thrown. Leave the error on display.
  if (error_thrown) {
    return;
  }

  if (int(current_temp) != int(last_temp) && millis() - last_update > MIN_DISPLAY_UPDATE_MILLIS) {
    last_update = millis();
    last_temp = current_temp;
    menu.update();
  }
  bool up_pressed = false;
  bool down_pressed = false;
  bool select_pressed = false;

  if (!up_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    up_pressed = true;
    last_press_time = millis();
  }

  else if (!down_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    down_pressed = true;
    last_press_time = millis();
  }

  else if (!select_btn.getState() && millis() - last_press_time > AUTO_PRESS_DELAY) {
    select_pressed = true;
    last_press_time = millis();
  }

  // Navigation
  switch (controlState) {
    case CTRL_SET_SCREEN:  // cycling screens
      if (up_pressed) {
        menu.previous_screen();
      } else if (down_pressed) {
        menu.next_screen();
      }

      break;

    case CTRL_SET_LINE:  // cycling through lines
      if (up_pressed) {
        menu.switch_focus(false);
      } else if (down_pressed) {
        menu.switch_focus(true);
        // menu.switch_focus();
      }

      break;

    case CTRL_SET_SETTING:  // changing a setting
      if (up_pressed) {
        menu.call_function(1);
      } else if (down_pressed) {
        menu.call_function(2);
      }

      break;

    default:  // invalid state
      controlState = CTRL_SET_SCREEN;
      break;
  }

  if (select_pressed) {
    // if the line has two functions, it uses both arrows and needs control state changed
    if (menu.is_callable(2)) {
      if (controlState + 1 < CTRL_OUT_OF_BOUNDS) {
        controlState = (ControlState)(controlState + 1);
      } else {
        controlState = CTRL_SET_LINE;
      }
    }
    // otherwise, it is either a status or a button. Just try to run the function.
    else {
      menu.call_function(1);
    }
  }
}
