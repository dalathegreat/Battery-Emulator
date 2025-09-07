#ifndef ARDUINO_H
#define ARDUINO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "HardwareSerial.h"
#include "Logging.h"
#include "Print.h"

#include "esp-hal-gpio.h"

// Arduino base constants for print formatting
constexpr int BIN = 2;
constexpr int OCT = 8;
constexpr int DEC = 10;
constexpr int HEX = 16;
// Arduino type aliases
using byte = uint8_t;
#define boolean bool
// Arduino random functions
inline long random(long max) {
  (void)max;
  return 0;  // Return a predictable value for testing
}

inline long random(long min, long max) {
  (void)min;
  (void)max;
  return min;  // Return the minimum value for predictability
}

inline void randomSeed(unsigned long seed) {
  (void)seed;
}

inline uint16_t word(uint8_t highByte, uint8_t lowByte) {
  return (static_cast<uint16_t>(highByte) << 8) | lowByte;
}

inline uint16_t word(uint16_t w) {
  return w;
}

// Bit manipulation functions
inline uint8_t bitRead(uint8_t value, uint8_t bit) {
  return (value >> bit) & 0x01;
}

inline void bitSet(uint8_t& value, uint8_t bit) {
  value |= (1UL << bit);
}

inline void bitClear(uint8_t& value, uint8_t bit) {
  value &= ~(1UL << bit);
}

inline void bitWrite(uint8_t& value, uint8_t bit, uint8_t bitvalue) {
  if (bitvalue) {
    bitSet(value, bit);
  } else {
    bitClear(value, bit);
  }
}

// Byte extraction functions
inline uint8_t lowByte(uint16_t w) {
  return static_cast<uint8_t>(w & 0xFF);
}

inline uint8_t highByte(uint16_t w) {
  return static_cast<uint8_t>(w >> 8);
}

template <typename T>
inline const T& min(const T& a, const T& b) {
  return (a < b) ? a : b;
}

template <typename T>
inline const T& max(const T& a, const T& b) {
  return (a > b) ? a : b;
}
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
inline int analogRead(uint8_t pin) {
  (void)pin;
  return 0;  // Return 0 for predictable tests
}

// Mock WiFi types
typedef int WiFiEvent_t;
typedef int WiFiEventInfo_t;

// Mock WiFi functions
inline void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;
}

inline void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;
}

unsigned long micros();
// Can be previously declared as a macro in stupid eModbus
#undef millis
unsigned long millis();

void delay(unsigned long ms);
void delayMicroseconds(unsigned long us);
int max(int a, int b);

bool ledcAttachChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel);
bool ledcWrite(uint8_t pin, uint32_t duty);

class ESPClass {
 public:
  size_t getFlashChipSize() {
    // This is a placeholder for the actual implementation
    // that retrieves the flash chip size.
    return 4 * 1024 * 1024;  // Example: returning 4MB
  }
};

extern ESPClass ESP;

#endif
