#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <inttypes.h>
#include "Print.h"
#include "types.h"

class Logging : public Print {
  void add_timestamp(size_t size);

 public:
  virtual size_t write(const uint8_t* buffer, size_t size);
  virtual size_t write(uint8_t) { return 0; }
  void printf(const char* fmt, ...);
  void log_bms_status(real_bms_status_enum bms_status);
  Logging() {}
};

extern Logging logging;

#ifdef DEBUG_LOG
#define LOG_PRINT(fmt, ...) logging.printf(fmt, ##__VA_ARGS__)
#define LOG_PRINTLN(str) logging.println(str)
#else
#define LOG_PRINT(fmt, ...) \
  do {                      \
  } while (0)
#define LOG_PRINTLN(str) \
  do {                   \
  } while (0)
#endif

#endif  // __LOGGING_H__
