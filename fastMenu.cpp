
// /*************************
//  * FastMenu.h
//  *
//  * An efficient, fast menu library
// */
#include <stdint.h>
#include <Wire.h>
#include <hd44780.h>                        // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header
#include <hd44780ioClass/hd44780_pinIO.h>   // Arduino pin i/o class header
#include "fastMenu.h"


// #define MAX_VARS 4
// #define MAX_LINES 16
// #define MAX_SCREENS 16


// /********************************************
//  * -------------- FastLineGeneric -----------------*
// ********************************************/

/// @brief Adds a variable to be displayed at the end of the line
/// @param select_callback - The function to run on select
// void FastLineGeneric::add_controls(void (*select_callback)()) {
//   select_func = select_callback;
// }


// /// @brief Adds a variable to be displayed at the end of the line
// /// @param select_callback - The function to run on select
// /// @param up_callback - The function to run on up
// /// @param down_callback - The function to run on down
// void FastLineGeneric::add_controls(void (*select_callback)(), void (*up_callback)(), void (*down_callback)()) {
//   add_controls(select_callback);
//   _focusable = true;
//   up_func = up_callback;
//   down_func = down_callback;
// }


// /// @brief Adds a variable to be displayed at the end of the line
// /// @param select_callback - The function to run on select
// /// @param up_callback - The function to run on up
// /// @param down_callback - The function to run on down
// /// @param left_callback - The function to run on left
// /// @param right_callback - The function to run on right
// void FastLineGeneric::add_controls(void (*select_callback)(),
//                             void (*up_callback)(), void (*down_callback)(), void (*left_callback)(), void (*right_callback)()) {
//   add_controls(select_callback, up_callback, down_callback);
//   left_func = left_callback;
//   right_func = right_callback;
// }


/// @brief Prints the line text using a Print object
/// @param lcd - The Print object to send output to
template<typename A, typename T>
void FastLine<A, T>::print_line(Print &outdev) {
  // TODO: find a way to limit decimal point on floats
  outdev.print(_variable);
}


/********************************************
 * -------------- FastScreen ---------------*
********************************************/

// /// @brief add a line to the screen
// /// @param line
// /// @return true if line was successfully added
// bool FastScreen::add_line(FastLineGeneric* line) {
//   // check that there are not too many lines
//   if (num_lines >= MAX_LINES) {
//     return false;
//   }

//   // add the line
//   lines[num_lines++] = line;
//   return true;
// }


// /// @brief Focuses the current line, calling its select function(s)
// void FastScreen::select() {
//   // focus/unfocus the line
//   if (focused == false) {
//     // run the line's select
//     lines[current_line]->select();

//     // focus the line if applicable
//     if (lines[current_line]->focusable()) {
//       focused = true;
//     }
//   } else if (focused == true) {
//     // unfocus the line
//     focused = false;
//   }
// }


// /// @brief move selection up
// void FastScreen::up() {
//   if (focused) {
//     lines[current_line]->up();
//   } else {
//     current_line--;
//   }
// }


// /// @brief move selection down
// void FastScreen::down() {
//   if (focused) {
//     lines[current_line]->down();
//   } else {
//     current_line++;
//   }
// }


// /// @brief move selection up
// void FastScreen::left() {
//   if (focused) {
//     lines[current_line]->left();
//   }
// }


// /// @brief move selection down
// void FastScreen::right() {
//   if (focused) {
//     lines[current_line]->right();
//   }
// }


// /// @brief Prints lines to screen
// /// @param start_idx first line to print
// /// @param num_lines the number of lines to print
// /// @param outdev the device to print to
// void FastScreen::print_lines(uint8_t start_idx, uint8_t num_lines, Print &outdev) {
//   for (uint8_t line_idx = start_idx; line_idx < start_idx + num_lines; line_idx++) {
//     lines[line_idx]->print_line(outdev);
//   }
// }



/********************************************
 * -------------- FastMenu -----------------*
********************************************/

// /// @brief Add a screen to the menu
// /// @param screen - the screen to add
// /// @return true if the screen was added, false otherwise
// bool FastMenu::add_screen(FastScreen &screen) {
//   if (_num_screens >= MAX_SCREENS) {
//     return false;
//   }
//   _screens[_num_screens++] = &screen;
//   return true;
// }

// /// @brief updates the top line's index
// void FastMenu::update_top_idx() {
//   // get the index of the top line
//   //(idx-1)-(idx-1)%rows
//   uint8_t current_idx = _screens[_current_screen_idx]->get_current_idx();
//   _current_top_idx = (current_idx - 1) - (current_idx - 1) % _rows;
// }


// /// @brief select control
// void FastMenu::select() {
//   _screens[_current_screen_idx]->select();
// }


// /// @brief up control
// void FastMenu::up() {
//   _screens[_current_screen_idx]->up();

//   // update the index of the top line
//   update_top_idx();
// }


// /// @brief down control
// void FastMenu::down() {
//   _screens[_current_screen_idx]->down();

//   // update the index of the top line
//   update_top_idx();
// }


// /// @brief left control
// void FastMenu::left() {
//   _screens[_current_screen_idx]->left();
// }


// /// @brief right control
// void FastMenu::right() {
//   _screens[_current_screen_idx]->right();
// }


// /// @brief Display menu to the screen
// void FastMenu::display() {
//   _lcd.clear();

//   _screens[_current_screen_idx]->print_lines(_current_top_idx, _rows, _lcd);
// }


// // /// @brief update the menu
// // void FastMenu::update() {
// // }