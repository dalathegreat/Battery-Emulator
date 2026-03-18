/*Based on the Adafruit Neopixel library, which has been heavily modified to support only 1x RGB/GRB LED for lowest possible CPU usage*/

#include "Adafruit_NeoPixel.h"

extern "C" void espShow(uint16_t pin, uint8_t *pixels, uint8_t numBytes);

Adafruit_NeoPixel::Adafruit_NeoPixel(int16_t p) : pixels(NULL) {
  updateLength();
  setPin(p);
}

void Adafruit_NeoPixel::updateLength(void) {
  free(pixels);
  pixels = (uint8_t *)malloc(numBytes);
  if (pixels) memset(pixels, 0, numBytes);
}

void Adafruit_NeoPixel::setPin(int16_t p) {
  if (pin >= 0) pinMode(pin, INPUT);
  pin = p;
  if ((p >= 0)) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void Adafruit_NeoPixel::setColorOrder(uint32_t o) {
  color_order = o;
}

void Adafruit_NeoPixel::show(void) {
  if (!pixels) return;
  espShow(pin, pixels, numBytes);
}

void Adafruit_NeoPixel::setPixelColor(uint32_t c) {
  uint8_t *p = pixels;
  uint8_t r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
  if (color_order == GRB) {  // GRB color order 
    p[rOffset] = g;
    p[gOffset] = r;
    p[bOffset] = b;
  } else {                   // default RGB color order  
    p[rOffset] = r;
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

