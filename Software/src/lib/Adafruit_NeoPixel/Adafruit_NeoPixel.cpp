/* * Lightweight Adafruit_NeoPixel Implementation
 * Keeps the CPU usage as low as possible for RTOS stability.
 */

#include "Adafruit_NeoPixel.h"

extern "C" void espShow(uint16_t pin, uint8_t *pixels, uint8_t numBytes);

Adafruit_NeoPixel::Adafruit_NeoPixel(int16_t p) : pin(p), pixels(NULL) {
  updateLength();
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  if (pixels) free(pixels);
}

void Adafruit_NeoPixel::updateLength(void) {
  if (pixels) free(pixels);
  numBytes = numLEDs * 3;
  pixels = (uint8_t *)malloc(numBytes);
  if (pixels) memset(pixels, 0, numBytes);
}

void Adafruit_NeoPixel::updateLength(uint16_t n) {
  numLEDs = n;
  updateLength();
}

void Adafruit_NeoPixel::updateType(uint8_t t) {
  (void)t;
}

uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void Adafruit_NeoPixel::setPin(int16_t p) {
  if (pin >= 0) pinMode(pin, INPUT);
  pin = p;
  if (p >= 0) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void Adafruit_NeoPixel::begin(void) {
  setPin(pin);
}

void Adafruit_NeoPixel::clear(void) {
  if (pixels) memset(pixels, 0, numBytes);
}

uint16_t Adafruit_NeoPixel::numPixels(void) const {
  return numLEDs;
}

void Adafruit_NeoPixel::setColorOrder(uint32_t o) {
  color_order = o;
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if (!pixels || n >= numLEDs) return;

  uint8_t r = (uint8_t)(c >> 16);
  uint8_t g = (uint8_t)(c >> 8);
  uint8_t b = (uint8_t)c;

  uint16_t offset = n * 3;

  if (color_order == GRB) {
    pixels[offset] = g;
    pixels[offset + 1] = r;
    pixels[offset + 2] = b;
  } else {
    pixels[offset] = r;
    pixels[offset + 1] = g;
    pixels[offset + 2] = b;
  }
}

void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  setPixelColor(n, Color(r, g, b));
}

void Adafruit_NeoPixel::show(void) {
  if (!pixels || pin < 0) return;
  espShow(pin, pixels, numBytes);
}