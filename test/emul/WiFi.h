#pragma once

#include "WString.h"

typedef enum { WIFI_STA, WIFI_AP_STA } wifi_mode_t;

typedef enum {
  WL_IDLE_STATUS = 0,
  WL_CONNECTED = 3,
  // WL_CONNECT_FAILED = 4,
  // WL_CONNECTION_LOST = 5,
  // WL_DISCONNECTED = 6

} wl_status_t;

// Mock WiFi types
typedef enum {
  ARDUINO_EVENT_WIFI_STA_DISCONNECTED,
  ARDUINO_EVENT_WIFI_STA_CONNECTED,
  ARDUINO_EVENT_WIFI_STA_GOT_IP,
} arduino_event_id_t;
typedef void* arduino_event_info_t;
typedef int wifi_event_id_t;

#define WiFiEvent_t arduino_event_id_t
#define WiFiEventInfo_t arduino_event_info_t
#define WiFiEventId_t wifi_event_id_t

class IPAddress {
 public:
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {}
  String toString() { return String("1.2.3.4"); }
};

class WiFiClass {
 public:
  IPAddress softAPIP() { return IPAddress(192, 168, 4, 1); }
  String SSID() { return String("TestSSID"); }
  int RSSI() {
    return -50;  // Example RSSI value
  }
  int channel() {
    return 6;  // Example WiFi channel
  }
  String getHostname() { return String("TestHostname"); }
  IPAddress localIP() { return IPAddress(192, 168, 1, 100); }
  wl_status_t status() {
    return WL_CONNECTED;  // Example status
  }
  void setHostname(const char* hostname) {}
  void mode(int mode) {}
  void setAutoReconnect(bool autoReconnect) {}
  void config(IPAddress local_IP, IPAddress gateway, IPAddress subnet) {}
  void begin(const char* ssid, const char* password, int channel) {}
  String macAddress() { return String("08:F9:E0:D1:06:8C"); }
  void disconnect() {}
  bool reconnect() { return true; }
  void softAP(const char* ssid, const char* password) {}
  void onEvent(void (*callback)(arduino_event_id_t event, arduino_event_info_t info), arduino_event_id_t eventType) {}
};
extern WiFiClass WiFi;
