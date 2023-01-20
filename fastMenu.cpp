
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

// /********************************************
//  * -------------- FastLineGeneric -----------------*
// ********************************************/

/// @brief Prints the line text using a Print object
/// @param lcd - The Print object to send output to
template<typename T>
void FastLine<T>::print_line(hd44780 &outdev) {
  // TODO: find a way to limit decimal point on floats
  outdev.print(_line);
  outdev.print(*_variable);
  // clear the display. faster than clearing the display;
  outdev.print("         ");
}