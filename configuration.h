/******************************************
* configuration.h
* Contains all the essential configurations
* for the PETInator.
*******************************************/

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
#define TEMP_READ_DELAY 100 // how frequently the thermistor should be read
#define MAX_TEMP 240
#define DEFAULT_TEMP 198 // default temp to use as target
#define TEMP_VARIANCE 3 // range in which the stepper should be allowed to run
#define TEMP_INCREMENT_SIZE 1 // How much temp jumps each button press


/******************************************
* PID
* PID settings and gains
*/
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255
#define DEFAULT_KP .22 //0.22
#define DEFAULT_KI .92 //0.92
#define DEFAULT_KD 0.1 // 0.1

#define BANG_BANG_RANGE 4 // higher or lower than BANG_BANG_RANGE will not use PID


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
*/
#define EEPROM_INITIALIZED_SIGN 12297829382473000000 // in binary, 101010...010

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


/******************************************
* Pinout
* Physical pins used
* Note: Step pin must be PWM capable. 
* See https://github.com/gin66/FastAccelStepper
* for a list of usable pins.
*******************************************/
#define THERMISTOR_PIN A0 // Must be an analog pin
#define HEATER_PIN 6

#define UP_BTN 12
#define SELECT_BTN 11
#define DOWN_BTN 10

// Stepper pins
#define ENABLE_PIN 7
#define DIR_PIN 8
// Motor driving pins, whether stepper or not, should be PWM capable
#ifdef USES_STEPPER
  #define STEP_PIN 9
#else
  #define MOTOR_PWM_PIN 9
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
  
