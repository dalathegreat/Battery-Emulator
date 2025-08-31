#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <inttypes.h>
#include "../../../USER_SETTINGS.h"
#include "../../datalayer/datalayer.h"
#include "types.h"

#ifndef UNIT_TEST
// Real implementation for production
#include <Print.h>

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

  static void println(const char* str) { (void)str; }

  Logging() {}
};

// Test macros - empty implementations
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#define DEBUG_PRINTLN(str) ((void)0)

#endif

extern Logging logging;

#endif  // __LOGGING_H__
