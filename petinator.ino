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

#include "configuration.h"

/****************************************
 * Thermistor
 * Wiring: https://learn.adafruit.com/thermistor/using-a-thermistor
 *  Use a 4.7k resistor (or appropriate for your thermistor)
 *    Pin                  ==> Arduino Pin
 *    ------------------------------------
 *    Resistor (4.7K)      ==> Vcc (5v)
 *    Resistor (4.7K)      ==> A0
 *    Thermistor Positive  ==> A0
 *    Thermistor Negative  ==> Gnd
 */
unsigned long lastTempUpdate;  // tracks clock time of last temp update
double current_temp = 0;       // celsius
double last_temp = 0;          // celsius
int extruderTempErrors = 0;    // tracks temporary errors in the thermistor

thermistor temp_sensor(THERMISTOR_PIN, THERMISTOR_TYPE);

bool updateTemperature() {
  if ((millis() - lastTempUpdate) > TEMP_READ_DELAY) {
    current_temp = temp_sensor.analog2temp();  // get temp reading
    lastTempUpdate = millis();
  }
}

/*******************
 * Decouping
 * checks and timers
 */
unsigned long decoupleTestPeriod = 1000;        // Time in ms between setting and testing decoupling while heating.
unsigned long decoupleTestPeriodHeated = 1000;  // Time in ms between setting and testing decoupling when fully heated.
//#define decouplingTestMaxHoldVariance 20        //Max temperature fluctuation once the heater has reached its window
#define decouplingTestMinTempRise 0.5
#define preheatTimeCutoff 10000  // Time in millis to allow heater to heat, before checking for decoupling
unsigned long lastDecoupleTest;  // Last time of decoupling sensor-heater test
unsigned long preheatTime;       // Time in millis when heating started
double lastDecoupleTemp;         // Last temperature of decoupling sensor-heater test
int heating = 0;                 //0 - Off, 1 - Heating, 2 - stabilised

/*******************
 * PID
 * pid settings and gains
 */
double target_temp = DEFAULT_TEMP;  // when enabled, temp_set_point will be set to this
double temp_set_point = 0;          // the value used by autopid
double heater_pwm = 0;              // heater's pwm will be controlled by this value
double last_pwm = 0;                // keeps track of the last value for less frequent updating
bool heatingEnabled = false;
bool fault = false;  // keeps track of any critical errors, and shuts down the machine

// input/output variables passed by reference, so they are updated automatically
AutoPID heaterPID(&current_temp, &temp_set_point, &heater_pwm, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

void safety_check() {

  // Check for obvious sensor errors
  if (current_temp < MIN_DEFECT_TEMP) {  // no temperature sensor, disable heater
    if (extruderTempErrors < 10) {       // Ignore temporary failures
      extruderTempErrors++;
    } else {
      heatingEnabled = false;  //No thermistor
      fault = true;
    }
  } else if (current_temp > MAX_DEFECT_TEMP) {  // short in sensor, disable heater
    if (extruderTempErrors < 10) {              // Ignore temporary failures
      extruderTempErrors++;
    } else {
      heatingEnabled = false;  //Thermistor short
      fault = true;
    }
  } else if (current_temp > MAX_TEMP) {  // over temperature, disable heater
    if (extruderTempErrors < 10) {       // Ignore temporary failures
      extruderTempErrors++;
    } else {
      heatingEnabled = false;  //Over Temp
      fault = true;
    }
  }

  if (current_temp >= target_temp - TEMP_VARIANCE && heating != 2 && !fault) {
    heating = 2;  //temperature is cose to/at setpoint
    //Serial.println("Reached setpoint");
  }

  // Run test if heater and sensor are decoupled
  unsigned long time = millis();
  bool decoupleTestRequired = !fault && decoupleTestPeriod > 0 && (time - lastDecoupleTest) > decoupleTestPeriod && (time - preheatTime) > preheatTimeCutoff;  // time enough for temperature change?
  if (decoupleTestRequired && heatingEnabled) {                                                                                                                // Only test when powered

    // Serial.println("decoupleTestRequired");
    if (heating == 1) {                                                   // Phase 1: Heating fully until target range is reached
      if (current_temp - lastDecoupleTemp < decouplingTestMinTempRise) {  // failed test
        if (extruderTempErrors < 10) {                                    // Ignore short temporary failures
          extruderTempErrors++;
          //Serial.println("E 1+");
        } else {
          // Serial.print("Error:Temp. raised too slow. Rise = ");
          // Serial.print(current_temp - lastDecoupleTemp);
          // Serial.print(" after ");
          // Serial.print(time - lastDecoupleTest);
          // Serial.println(" ms");
          // error = 1;
          heatingEnabled = false;  //Over Temp
          fault = true;            //Heater decoupled during rising
        }
      }
      lastDecoupleTest = time;
    } else {                                                   // Phase 2: Holding temperature inside a target corridor
      if (fabs(current_temp - target_temp) > TEMP_VARIANCE) {  // failed test
        if (extruderTempErrors < 10) {                         // Ignore short temporary failures
          extruderTempErrors++;
          // Serial.println("E 2+");
        } else {
          // Serial.print("Error:Temp. raised to slow. Rise = ");
          // Serial.print(current_temp - lastDecoupleTemp);
          // Serial.print(" after ");
          // Serial.print(time - lastDecoupleTest);
          // Serial.println(" ms");
          // error = 2;
          heatingEnabled = false;  //Over Temp
          fault = true;            //Heater decoupled during hold
        }
        lastDecoupleTest = time;
      } else {
        lastDecoupleTest = time - decoupleTestPeriod + decoupleTestPeriodHeated;  // once running test every second
      }
    }
    lastDecoupleTemp = current_temp;
  }
}

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
  if (heatingEnabled && !fault) {
    temp_set_point = target_temp;
    preheatTime = millis();
    heating = 1;  //Heater is rising to setpoint
  } else {
    temp_set_point = 0;
    heating = 0;  //Heater is off
  }
}

/******************************************
 * Motor
 */
long target_speed = DEFAULT_SPEED;
int pullingEnabled = false;

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
  if (pullingEnabled) {
    // stepper->stopMove();
    pullingEnabled = false;
  } else {
    pullingEnabled = true;
  }
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
// Here the column is 3, the row is 1 and the string is "Hello Menu".
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
// LiquidLine start_line(0, 0, "Start: ", menu_message);
// LiquidScreen enable_screen(start_line);
LiquidScreen main_screen;

LiquidMenu menu(lcd);

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
  enable_heater_line.attach_function(2, toggle_heater);

  enable_puller_line.attach_function(1, toggle_puller);
  enable_puller_line.attach_function(2, toggle_puller);
  // start_line.attach_function(1, toggle_running);

  set_temp_line.set_decimalPlaces(0);
  actual_temp_line.set_decimalPlaces(0);

  main_screen.add_line(enable_heater_line);
  main_screen.add_line(enable_puller_line);
  main_screen.add_line(set_temp_line);
  main_screen.add_line(set_speed_line);
  main_screen.add_line(actual_temp_line);
  main_screen.add_line(actual_speed_line);
  main_screen.set_displayLineCount(2);
  // menu.add_screen(welcome_screen);
  // menu.add_screen(status_screen);
  // menu.add_screen(edit_screen);
  // menu.add_screen(enable_screen);
  menu.add_screen(main_screen);
}

void loop() {
  updateTemperature();
  heaterPID.run();
  safety_check();

  // check if motor should keep running. Motor won't run until temperature is reached.
  runMotorIfTempReached(current_temp, target_temp);

  up_btn.loop();
  select_btn.loop();
  down_btn.loop();

  /*********
     * Temperature
     */
  if (int(heater_pwm) != int(last_pwm)) {
    last_pwm = heater_pwm;
    analogWrite(HEATER_PIN, int(heater_pwm));
  }
  digitalWrite(LED_BUILTIN, heaterPID.atSetPoint(1));  // light up LED when we're at setpoint +-1 degree

  /***********
     * Menu
     */
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
    if (controlState + 1 < CTRL_OUT_OF_BOUNDS) {
      controlState = (ControlState)(controlState + 1);
    } else {
      controlState = 1;
    }
  }
}