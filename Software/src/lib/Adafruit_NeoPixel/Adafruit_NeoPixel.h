/* * Optimized for Battery Emulator
 * Lightweight version tailored specifically for led_handler.cpp
 */

#ifndef ADAFRUIT_NEOPIXEL_H
#define ADAFRUIT_NEOPIXEL_H

#include <Arduino.h>

// (Dummy Defines)
#define NEO_GRB     0x01
#define NEO_KHZ800  0x02

enum led_color_order { RGB, GRB };

class Adafruit_NeoPixel {
public:
  Adafruit_NeoPixel(int16_t pin = -1);
  ~Adafruit_NeoPixel();

  void begin(void);
  void show(void);
  void clear(void);
  uint16_t numPixels(void) const;
  void setPixelColor(uint16_t n, uint32_t c);
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);

  void setColorOrder(uint32_t o);

  void updateType(uint8_t t);
  void updateLength(uint16_t n);
  void setPin(int16_t p);
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);

protected:
  uint16_t numLEDs = 1;
  uint8_t numBytes = 3;
  int16_t pin;
  uint8_t *pixels;
  uint32_t color_order = RGB;

  void updateLength(void);
};

#endif