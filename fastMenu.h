
/*************************
 * FastMenu.h
 * 
 * An efficient, fast menu library
*/
#include "configuration.h"
#include <stdint.h>
#include <Wire.h>
#include <hd44780.h>                        // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header
#include <hd44780ioClass/hd44780_pinIO.h>   // Arduino pin i/o class header


#define MAX_LINES 16
#define MAX_SCREENS 16



/********************************************
 * -------------- FastLineGeneric -----------------*
********************************************/
class FastLineGeneric {
protected:
  bool _focusable = false;  // whether the line is focusable
  const uint8_t _column;    // what row and column the line starts on
  const uint8_t _row;

  void (*select_func)();  // Function to run when select is called
  void (*up_func)();      // Function to run when up is called
  void (*down_func)();    // Function to run when down is called
  void (*left_func)();    // Function to run when left is called
  void (*right_func)();   // Function to run when right is called


public:
  /// @brief Constructor for one variable/constant.
  /// @param column - the column at which the line starts
  /// @param row - the row at which the line is printed
  /// @param &text - variable/constant to be printed
  FastLineGeneric(uint8_t column, uint8_t row)
    : _column(column), _row(row) {
  }


  bool focusable() {
    return _focusable;
  }


  /// @brief Controls
  void select() {
    select_func();
  };
  void up(){};
  void down(){};
  void left(){};
  void right(){};


  /// @brief Adds a variable to be displayed at the end of the line
  /// @param select_callback - The function to run on select
  void add_controls(void (*select_callback)()) {
    select_func = select_callback;
  }


  /// @brief Adds a variable to be displayed at the end of the line
  /// @param select_callback - The function to run on select
  /// @param up_callback - The function to run on up
  /// @param down_callback - The function to run on down
  void add_controls(void (*select_callback)(), void (*up_callback)(), void (*down_callback)()) {
    add_controls(select_callback);
    _focusable = true;
    up_func = up_callback;
    down_func = down_callback;
  }


  /// @brief Adds a variable to be displayed at the end of the line
  /// @param select_callback - The function to run on select
  /// @param up_callback - The function to run on up
  /// @param down_callback - The function to run on down
  /// @param left_callback - The function to run on left
  /// @param right_callback - The function to run on right
  void add_controls(void (*select_callback)(),
                    void (*up_callback)(), void (*down_callback)(), void (*left_callback)(), void (*right_callback)()) {
    add_controls(select_callback, up_callback, down_callback);
    left_func = left_callback;
    right_func = right_callback;
  }


  /// @brief Prints the line text using a Print object
  /// @param outdev - The Print object to send output to
  virtual void print_line(hd44780 &outdev);
};


/********************************************
 * -------------- FastLine -----------------*
********************************************/
template<typename T = int>
class FastLine : public FastLineGeneric {
private:
  const char *_line;  // line to display
  T _variable;        // variable to display after the string


public:
  /// @brief Constructor for one variable/constant.
  /// @param column - the column at which the line starts
  /// @param row - the row at which the line is printed
  /// @param &text - variable/constant to be printed
  FastLine(uint8_t column, uint8_t row, const char *text)
    : FastLineGeneric(column, row), _line(text) {
  }

  /// @brief Constructor for one variable/constant.
  /// @param column - the column at which the line starts
  /// @param row - the row at which the line is printed
  /// @param &text - variable/constant to be printed
  /// @param &variable - variable to be printed and automatically updated
  FastLine(uint8_t column, uint8_t row, const char *text, T &variable)
    : FastLine(column, row, text) {
    _variable = variable;
  }

  /// @brief Prints the line text using a Print object
  /// @param outdev - The Print object to send output to
  void print_line(hd44780 &outdev) override;
};



/********************************************
 * -------------- FastScreen ---------------*
 * Manages lines and controls               *
********************************************/
class FastScreen {
private:
  uint8_t _num_lines = 0;
  typedef FastLineGeneric *FastLinePtr;
  FastLinePtr *lines = new FastLinePtr[MAX_LINES];
  uint8_t _current_line = 0;
  bool focused = false;

  void set_line(int line_idx) {
    _current_line = line_idx;
    // wrap the line
    if (_current_line >= _num_lines) {
      _current_line = 0;
    }
  }

public:
  /// @brief Constructor
  FastScreen() {
  }


  /// @brief add a line to the screen
  /// @param line - a line object to add
  /// @return true if the line was successfully added
  bool add_line(FastLineGeneric &line) {
    // check that there are not too many lines
    if (_num_lines >= MAX_LINES) {
      return false;
    }

    // add the line
    lines[_num_lines++] = &line;
    return true;
  }


  /// @brief select the current line. If focused, passes controls through to the line.
  void select() {
    // focus/unfocus the line
    if (focused == false) {
      // run the line's select
      lines[_current_line]->select();

      // focus the line if applicable
      if (lines[_current_line]->focusable()) {
        focused = true;
      }
    } else if (focused == true) {
      // unfocus the line
      focused = false;
    }
  }

  /// @brief move selection up
  void up() {
    if (focused) {
      lines[_current_line]->up();
    } else {
      // _current_line--;
      set_line(_current_line - 1);
    }
  }


  /// @brief move selection down
  void down() {
    if (focused) {
      lines[_current_line]->down();
    } else {
      // _current_line++;
      set_line(_current_line + 1);
    }
  }


  /// @brief move selection up
  void left() {
    if (focused) {
      lines[_current_line]->left();
    }
  }


  /// @brief move selection down
  void right() {
    if (focused) {
      lines[_current_line]->right();
    }
  }


  /// @brief Prints lines to screen
  /// @param start_idx first line to print
  /// @param num_lines the number of lines to print
  /// @param outdev the device to print to
  void print_lines(uint8_t start_idx, uint8_t num_lines, hd44780 &outdev) {
    if (start_idx + num_lines < _num_lines) {
      for (uint8_t i = 0; i < num_lines; i++) {
        // move to the start of the line
        outdev.setCursor(0, i);
        // ask the line to print
        lines[i + start_idx]->print_line(outdev);
      }
    }
  }

  uint8_t get_current_idx() {
    return _current_line;
  }
};



/********************************************
 * -------------- FastMenu -----------------*
 * Manages input and display                *
********************************************/
class FastMenu {
private:
  uint8_t _num_screens = 0;
  uint8_t _current_screen_idx = 0;
  uint8_t _current_top_idx = 0;
  typedef FastScreen *FastScreenPtr;
  FastScreenPtr *_screens = new FastScreenPtr[MAX_SCREENS];
  const uint8_t _columns, _rows;
  LCD_CLASS &_lcd;


public:

  /// @brief Class constructor
  /// @param &lcd - A reference to an initialized hd44780 lcd object or an inheriting object
  FastMenu(LCD_CLASS &lcd, uint8_t columns = 16, uint8_t rows = 2)
    : _lcd(lcd), _columns(columns), _rows(rows) {
  }


  /// @brief Add a screen to the menu
  /// @param screen - the screen to add
  /// @return true if the screen was added, false otherwise
  bool add_screen(FastScreen &screen) {
    // check for overflow
    if (_num_screens >= MAX_SCREENS) {
      return false;
    }

    // add the screen
    _screens[_num_screens++] = &screen;
    return true;
  }


  /// @brief updates the top line's index
  void update_top_idx() {
    // get the index of the top line
    //(idx-1)-(idx-1)%rows
    uint8_t current_idx = _screens[_current_screen_idx]->get_current_idx();
    _current_top_idx = (current_idx - 1) - (current_idx - 1) % _rows;
  }


  /// @brief select control
  void select() {
    _screens[_current_screen_idx]->select();
  }


  /// @brief up control
  void up() {
    _screens[_current_screen_idx]->up();

    // update the index of the top line
    update_top_idx();
  }


  /// @brief down control
  void down() {
    _screens[_current_screen_idx]->down();

    // update the index of the top line
    update_top_idx();
  }


  /// @brief left control
  void left() {
    _screens[_current_screen_idx]->left();
  }


  /// @brief right control
  void right() {
    _screens[_current_screen_idx]->right();
  }

  /// @brief switch to the next screen
  void next_screen() {
    set_screen(_current_screen_idx + 1);
  }

  // @brief switch to the previous screen
  void previous_screen() {
    set_screen(_current_screen_idx - 1);
  }

  void set_screen(int screen_idx) {
    if (screen_idx < _num_screens) {
      _current_screen_idx = screen_idx;
    }
  }


  /// @brief Display menu to the screen
  void display() {
    _lcd.clear();

    _screens[_current_screen_idx]->print_lines(_current_top_idx, _rows, _lcd);
  }

  /// @brief update the menu
  void update() {
    display();
  }
};