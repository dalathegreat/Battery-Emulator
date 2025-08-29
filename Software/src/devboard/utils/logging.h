#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <Print.h>
#include <inttypes.h>
#include "../../../USER_SETTINGS.h"
#include "../../datalayer/datalayer.h"
#include "types.h"

class Logging : public Print {
  void add_timestamp(size_t size);

 public:
  virtual size_t write(const uint8_t* buffer, size_t size);
  virtual size_t write(uint8_t) { return 0; }
  void printf(const char* fmt, ...);
  Logging() {}
};

extern Logging logging;

// Replace compile-time macros with runtime checks
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
#endif  // __LOGGING_H__
