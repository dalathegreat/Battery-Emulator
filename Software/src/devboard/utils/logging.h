#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <Print.h>
#include <inttypes.h>
#include "../../../USER_SETTINGS.h"
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
#define DEBUG_PRINTF(fmt, ...) logging.printf(fmt, ##__VA_ARGS__)
#define DEBUG_PRINTLN(str) logging.println(str)
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#define DEBUG_PRINTLN(str) ((void)0)
#endif

#endif  // __LOGGING_H__
