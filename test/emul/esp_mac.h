#ifndef ESP_MAC_H
#define ESP_MAC_H

#include <stdint.h>

typedef enum { ESP_MAC_WIFI_STA = 0 } esp_mac_type_t;

// A fixed address, so anything derived from it is the same in every test run
inline int esp_read_mac(uint8_t* mac, esp_mac_type_t type) {
  (void)type;
  static const uint8_t address[6] = {0x3C, 0x84, 0x27, 0xC5, 0xA1, 0xF0};
  for (int i = 0; i < 6; i++) {
    mac[i] = address[i];
  }
  return 0;
}

#endif
