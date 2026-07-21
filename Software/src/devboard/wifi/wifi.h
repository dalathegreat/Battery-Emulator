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
extern std::string custom_hostname;

void init_WiFi();
void wifi_monitor();
void connectToWiFi();
void FullReconnectToWiFi();

// In the real wifi.h
#ifndef UNIT_TEST
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
#else
// Mock declarations for unit tests
typedef int WiFiEvent_t;
typedef int WiFiEventInfo_t;
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

// Returns the default hostname ("battery-emulator-" + last two bytes of the MAC, lowercase)
// used when no custom hostname is configured. Safe to call at any time (reads eFuse directly).
String default_hostname();

void init_WiFi_AP();

extern bool wifi_enabled;
extern bool wifiap_enabled;
extern bool ap_active;
extern bool mdns_enabled;
extern bool espnow_enabled;
extern bool static_IP_enabled;
// Stored as dotted-quad strings; parsed with IPAddress::fromString() when the interface is brought up.
extern std::string static_local_IP;
extern std::string static_gateway;
extern std::string static_subnet;
extern std::string static_dns;  // Empty = use the gateway as resolver

#endif
