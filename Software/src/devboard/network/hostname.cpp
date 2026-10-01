#include "hostname.h"

#include <esp_mac.h>  // esp_read_mac()

std::string custom_hostname;  // If not set, defaults to default_hostname()

String default_hostname() {
  static String cached;
  if (cached.isEmpty()) {
    uint8_t mac_bytes[6];
    esp_read_mac(mac_bytes, ESP_MAC_WIFI_STA);  // reads eFuse directly, valid even before WiFi starts
    char mac_suffix[5];
    snprintf(mac_suffix, sizeof(mac_suffix), "%02x%02x", mac_bytes[4], mac_bytes[5]);
    cached = "battery-emulator-" + String(mac_suffix);
  }
  return cached;
}

String active_hostname() {
  if (!custom_hostname.empty()) {
    return String(custom_hostname.c_str());
  }
  return default_hostname();
}
