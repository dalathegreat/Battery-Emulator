#ifndef EMUL_UPDATE_H
#define EMUL_UPDATE_H

#include <Arduino.h>
#include <Print.h>
#include <stddef.h>
#include <stdint.h>

#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

#define U_FLASH 0
#define U_SPIFFS 100
#define U_AUTH 200

class UpdateClass {
 public:
  bool setMD5(const char* expected_md5
#ifndef UPDATE_NOCRYPT
              ,
              bool calc_post_decryption = true
#endif /* #ifdef UPDATE_NOCRYPT */
  ) {
    return true;
  }
  bool begin(size_t size = UPDATE_SIZE_UNKNOWN, int command = U_FLASH, int ledPin = -1, uint8_t ledOn = LOW,
             const char* label = NULL) {
    return true;
  }
  bool end(bool evenIfRemaining = false) { return true; }

  void printError(Print& out) {}
  bool hasError() const { return false; }
  size_t write(uint8_t* data, size_t len) { return len; }
};

extern UpdateClass Update;

#endif
