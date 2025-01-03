#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <inttypes.h>
#include "Print.h"

extern const char* version_number;  // The current software version, shown on webserver

class Logging : public Print {
  void add_timestamp(size_t size);

 public:
  virtual size_t write(const uint8_t* buffer, size_t size);
  virtual size_t write(uint8_t) { return 0; }
  void printf(const char* fmt, ...);
  Logging() {
    printf(
        "Battery emulator %s build "__DATE__
        " " __TIME__ "\n",
        version_number);
  }
};

extern Logging logging;
#endif  // __LOGGING_H__
