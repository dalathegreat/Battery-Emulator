/*!
 * @file Adafruit_NeoPixel.h
 *
 * This is part of Adafruit's NeoPixel library for the Arduino platform,
 * allowing a broad range of microcontroller boards (most AVR boards,
 * many ARM devices, ESP8266 and ESP32, among others) to control Adafruit
 * NeoPixels, FLORA RGB Smart Pixels and compatible devices -- WS2811,
 * WS2812, WS2812B, SK6812, etc.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing products
 * from Adafruit!
 *
 * Written by Phil "Paint Your Dragon" Burgess for Adafruit Industries,
 * with contributions by PJRC, Michael Miller and other members of the
 * open source community.
 *
 * This file is part of the Adafruit_NeoPixel library.
 *
 * Adafruit_NeoPixel is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Adafruit_NeoPixel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NeoPixel.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ADAFRUIT_NEOPIXEL_H
#define ADAFRUIT_NEOPIXEL_H

#include <Arduino.h>

/*!
    @brief  Class that stores state and functions for interacting with
            Adafruit NeoPixels and compatible devices.
*/
class Adafruit_NeoPixel {

public:
  // Constructor: number of LEDs, pin number, LED type
  Adafruit_NeoPixel(int16_t pin = -1);
  Adafruit_NeoPixel(void);

  void show(void);
  void setPin(int16_t p);
  void setPixelColor(uint32_t c);
  void updateLength(void);

private:

protected:
  uint8_t numBytes = 3;  ///< Size of 'pixels' buffer below
  int16_t pin;        ///< Output pin number (-1 if not yet set)
  uint8_t *pixels;    ///< Holds LED color values (3 or 4 bytes each)
  uint8_t rOffset = 0b01;    ///< Red index within each 3- or 4-byte pixel
  uint8_t gOffset = 0b00;    ///< Index of green byte
  uint8_t bOffset = 0b10;    ///< Index of blue byte
};

#endif // ADAFRUIT_NEOPIXEL_H
