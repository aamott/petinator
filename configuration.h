/******************************************
* configuration.h
* Contains all the essential configurations
* for the PETInator.
*******************************************/

#ifndef CONFIGURATION_H
  #define CONFIGURATION_H

/******************************************
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
#define THERMISTOR_TYPE 5 // See https://github.com/miguel5612/Arduino-ThermistorLibrary for possible values
#define TEMP_READ_DELAY 100 // how frequently the thermistor should be read in ms
#define MAX_TEMP 240            // maximum temperature the thermistor can read (deg C).
#define DEFAULT_TEMP 198 // default temp to use as target
#define TEMP_VARIANCE 3 // how close the actual temp should be to the target temp to be considered "heated"
#define TEMP_INCREMENT_SIZE 1 // How much temp jumps each button press

// Thermal Protection
// Provides some protection against fire. If the thermistor falls out, shorts,
// or is damaged, it may read a much lower temperature than the the heater
// is at. If the temperature doesn't come at least a few degrees
// closer to the target (hysteresis) within a set time (period), 
// we stop heating and pulling. 
// If you are getting Thermal Runaway or Cooling Failed errors, increase the 
// period or decrease the hysteresis. 
#define THERMAL_PROTECTION_PERIOD 20        // Seconds
#define THERMAL_PROTECTION_HYSTERESIS 4     // Degrees Celsius
#define CHECKS_PER_PERIOD 20 // How many times to check for thermal runaway during the period


/******************************************
* PID
* PID settings and gains
*/
#define PID_PERIOD 100 // how often PID should run in milliseconds
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255
// PID values
// Make sure to change EEPROM_INITIALIZED_SIGN when you update these!
#define DEFAULT_KP 1000 //1000
#define DEFAULT_KI 0 //0
#define DEFAULT_KD 0 // 0

#define BANG_BANG_RANGE 40 // higher or lower than BANG_BANG_RANGE will not use PID


/******************************************
* Puller Motor
*/
#define USES_STEPPER // if a stepper is not used, a DC motor is assumed
#ifdef USES_STEPPER
    #define MAX_SPEED 30000 // in steps per second
    #define ACCELERATION 5000
    #define DEFAULT_SPEED 30000 // default stepper speed in steps per second
    #define SPEED_INC 100 // speed increment size in steps per second
#else
    #define USES_PWM_MOTOR
    #define MAX_SPEED 255 // PWM max. Almost always 256. 
    #define DEFAULT_SPEED 200 // default PWM value
    #define SPEED_INC 5 // speed increment size in steps per second
#endif

/******************************************
 * EEPROM
 * NOTE: You'll have to change this every time you 
 * make a change to any saved values.
 * TODO: Figure out a way to randomize this at
 * compile time so it overwrites saved values after
 * fresh uploads.
*/
#define EEPROM_INITIALIZED_SIGN -10923 //binary 1010101010101011

/******************************************
* Buttons
*/
#define AUTO_PRESS_DELAY 200 // How quickly the button press should repeat when held


/******************************************
* DISPLAY
* Wiring: https://create.arduino.cc/projecthub/Hack-star-Arduino/learn-to-use-lcd-1602-i2c-parallel-with-arduino-uno-f73f07
*/
// Use display with I2C controller
// requires changing LiquidMenu_config.h - see https://github.com/VaSe7u/LiquidMenu/issues/36
#define I2C_LCD

// 1602 display would be 2 rows, 16 columns
#define COLUMNS 16
#define ROWS 2

// Display updates take significant time. Don't update too often.
#define MIN_DISPLAY_UPDATE_MILLIS 500


// Define the class name
#ifdef I2C_LCD
  #define LCD_CLASS hd44780_I2Cexp
#else
  #define LCD_CLASS hd44780_pinIO
#endif


/******************************************
* Pinout
* Physical pins used
*******************************************/
#define THERMISTOR_PIN A0 // Analog input
#define HEATER_PIN 6

#define UP_BTN 12
#define SELECT_BTN 11
#define DOWN_BTN 10

// Stepper pins
#define ENABLE_PIN 7
#define DIR_PIN 8
// Motor driving pins, whether stepper or not, should be PWM capable
// https://github.com/gin66/FastAccelStepper for details
#ifdef USES_STEPPER
  #define STEP_PIN 9 // PWM capable
#else
  #define MOTOR_PWM_PIN 9 // PWM capable
#endif

// LCD
#ifdef I2C_LCD
  /* Wiring:
    *    i2c LCD Module  ==>   Arduino
    *    SCL             ==>     A5
    *    SDA             ==>     A4
    *    Vcc             ==>     Vcc (5v)
    *    Gnd             ==>     Gnd      
  */
  #define LCD_ADDRESS 0x27
#else
  /* Wiring:
    *  LCD Module ==>   Arduino Pin
    *  D7         ==> 0
    *  D6         ==> 1
    *  D5         ==> 2
    *  D4         ==> 3
    *  E          ==> 4
    *  RW         ==> Gnd
    *  RS         ==> 5 (or 5v)
  */
  #define LCD_D7 0
  #define LCD_D6 1
  #define LCD_D5 2
  #define LCD_D4 3
  #define LCD_E  4
  #define LCD_RS 5
#endif
  
#endif // CONFIGURATION_H