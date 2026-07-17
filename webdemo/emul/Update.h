#pragma once

class Updater {
 public:
  static bool begin(size_t size, int command = 0) {
    (void)size;
    (void)command;
    return true;
  }
  static bool end(bool evenIfRemaining = false) {
    (void)evenIfRemaining;
    return true;
  }
  static size_t write(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    return len;
  }
  static size_t write(uint8_t data) {
    (void)data;
    return 1;
  }
  static void abort() {}
  static int getError() { return 0; }
  static const char* errorString(int error) {
    (void)error;
    return "No error";
  }
  static bool setMD5(const char* md5) {
    (void)md5;
    return true;
  }
};
Updater Update;

#define UPDATE_SIZE_UNKNOWN 0
#define U_SPIFFS 0
#define U_FLASH 1
