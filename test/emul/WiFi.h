#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include "IPAddress.h"

class WiFiClass {
 public:
  IPAddress localIP() { return IPAddress((uint32_t)0); }
};

inline WiFiClass WiFi;

#endif
