#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <inttypes.h>
#include <string>
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
#ifndef SMALL_FLASH_DEVICE
  void set_next_severity(uint8_t sev);  // syslog severity for the next assembled line
#endif
  Logging() {}
};

// Production macros
#define DEBUG_PRINTF(fmt, ...)                                                                  \
  do {                                                                                          \
    if (datalayer.system.info.web_logging_active || datalayer.system.info.usb_logging_active || \
        datalayer.system.info.syslog_logging_active) {                                          \
      logging.printf(fmt, ##__VA_ARGS__);                                                       \
    }                                                                                           \
  } while (0)

#define DEBUG_PRINTLN(str)                                                                      \
  do {                                                                                          \
    if (datalayer.system.info.web_logging_active || datalayer.system.info.usb_logging_active || \
        datalayer.system.info.syslog_logging_active) {                                          \
      logging.println(str);                                                                     \
    }                                                                                           \
  } while (0)
#ifndef SMALL_FLASH_DEVICE
#define LOG_SET_NEXT_SEVERITY(sev) logging.set_next_severity(sev)
#else
#define LOG_SET_NEXT_SEVERITY(sev) ((void)0)
#endif

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
  static void print(long num) { (void)num; }
  static void print(unsigned long num) { (void)num; }
  static void print(long long num) { (void)num; }
  static void print(unsigned long long num) { (void)num; }
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
  static void println(long num) { (void)num; }
  static void println(unsigned long num) { (void)num; }
  static void println(long long num) { (void)num; }
  static void println(unsigned long long num) { (void)num; }
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
#define LOG_SET_NEXT_SEVERITY(sev) ((void)0)

#endif

extern Logging logging;

#ifndef SMALL_FLASH_DEVICE
// Remote syslog config (loaded from NVS in comm_nvm.cpp, consumed in logging.cpp)
extern std::string syslog_ip;
extern uint16_t syslog_port;
extern uint8_t syslog_facility;  // 0..23
void syslog_backlog_flush(void);
#endif

#endif  // __LOGGING_H__
