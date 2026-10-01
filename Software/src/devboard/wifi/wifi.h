#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <string>

extern std::string ssid;
extern std::string password;
extern uint16_t wifi_channel;
extern std::string ssidAP;
extern std::string passwordAP;
// Factory-default AP password. While the AP runs with this password, it is only
// kept enabled for a limited provisioning window (see wifi.cpp).
extern const char* DEFAULT_AP_PASSWORD;

void init_WiFi();
void wifi_monitor();
void connectToWiFi();
void FullReconnectToWiFi();

bool wifi_connected();

// In the real wifi.h
#ifndef UNIT_TEST
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void onApStaConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onApStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
#else
// Mock declarations for unit tests
typedef int WiFiEvent_t;
typedef int WiFiEventInfo_t;
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

void init_WiFi_AP();

extern bool wifiap_enabled;
extern bool ap_active;
extern bool espnow_enabled;
// Optional list of ESP-NOW receiver MAC addresses. Any separator is accepted
// ("AA:BB:CC:DD:EE:FF, 11-22-33-44-55-66"). Empty = broadcast to every device.
extern std::string espnow_peer_macs;
extern bool wifi_static_IP_enabled;
// Held in memory as native IPAddress; persisted to NVM as dotted-quad strings
extern IPAddress wifi_static_local_IP;
extern IPAddress wifi_static_gateway;
extern IPAddress wifi_static_subnet;
extern IPAddress wifi_static_dns;  // Unset (0.0.0.0) = use the gateway as resolver

#endif
