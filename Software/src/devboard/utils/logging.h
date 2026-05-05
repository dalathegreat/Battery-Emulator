#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <inttypes.h>
#include "../../datalayer/datalayer.h"
#include "Print.h"
#include "types.h"

#ifndef UNIT_TEST
// Real implementation for production

class Logging : public Print {
  void add_timestamp(size_t size);

 public:
  virtual size_t write(const uint8_t* buffer, size_t size);
  virtual size_t write(uint8_t) { return 0; }
  void printf(const char* fmt, ...);
  Logging() {}
};

// Production macros
#define DEBUG_PRINTF(fmt, ...)                                                                  \
  do {                                                                                          \
    if (datalayer.system.info.web_logging_active || datalayer.system.info.usb_logging_active) { \
      logging.printf(fmt, ##__VA_ARGS__);                                                       \
    }                                                                                           \
  } while (0)

#define DEBUG_PRINTLN(str)                                                                      \
  do {                                                                                          \
    if (datalayer.system.info.web_logging_active || datalayer.system.info.usb_logging_active) { \
      logging.println(str);                                                                     \
    }                                                                                           \
  } while (0)

#else
// Mock implementation for tests
#include <cstdarg>
#include <cstdio>

class Logging {
 public:
  // Mock methods that do nothing
  size_t write(const uint8_t* buffer, size_t size) { return size; }
  size_t write(uint8_t) { return 0; }

  static void printf(const char* fmt, ...) {
    // Empty implementation - silence unused parameter warnings
    (void)fmt;
  }

  // Overloaded print methods for different data types
  static void print(const char* str) { (void)str; }
  static void print(char c) { (void)c; }
  static void print(int8_t num) { (void)num; }
  static void print(uint8_t num) { (void)num; }
  static void print(int16_t num) { (void)num; }
  static void print(uint16_t num) { (void)num; }
  static void print(int32_t num) { (void)num; }
  static void print(uint32_t num) { (void)num; }
  static void print(int64_t num) { (void)num; }
  static void print(uint64_t num) { (void)num; }
  static void print(float num) { (void)num; }
  static void print(double num) { (void)num; }
  static void print(bool b) { (void)b; }

  static void print(double num, int precision) {
    (void)num;
    (void)precision;
  }

  static void print(float num, int precision) {
    (void)num;
    (void)precision;
  }

  static void print(int32_t num, int base) {
    (void)num;
    (void)base;
  }

  static void print(uint32_t num, int base) {
    (void)num;
    (void)base;
  }

  static void println(const char* str) { (void)str; }
  static void println(char c) { (void)c; }
  static void println(int8_t num) { (void)num; }
  static void println(uint8_t num) { (void)num; }
  static void println(int16_t num) { (void)num; }
  static void println(uint16_t num) { (void)num; }
  static void println(int32_t num) { (void)num; }
  static void println(uint32_t num) { (void)num; }
  static void println(int64_t num) { (void)num; }
  static void println(uint64_t num) { (void)num; }
  static void println(float num) { (void)num; }
  static void println(double num) { (void)num; }
  static void println(bool b) { (void)b; }
  static void println() {}  // Empty println

  Logging() {}
};

// Test macros - empty implementations
#define DEBUG_PRINT(fmt, ...) ((void)0)
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#define DEBUG_PRINTLN(str) ((void)0)

#endif

extern Logging logging;

#endif  // __LOGGING_H__
