#ifndef UUGP_WIFI_CLIENT_MOCK_H
#define UUGP_WIFI_CLIENT_MOCK_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class WiFiClient {
 public:
  bool connected() { return false; }
  void stop() {}
  void setConnectionTimeout(int) {}
  bool connect(IPAddress, uint16_t) { return false; }
  size_t write(const uint8_t*, size_t) { return 0; }
  void flush() {}
  int available() { return 0; }
  int read() { return -1; }
};

#endif
