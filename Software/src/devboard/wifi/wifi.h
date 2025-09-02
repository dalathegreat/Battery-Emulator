#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <string>

extern std::string ssid;
extern std::string password;
extern uint16_t wifi_channel;
extern std::string ssidAP;
extern std::string passwordAP;
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

void init_WiFi_AP();

// Initialise mDNS
void init_mDNS();

extern bool wifi_enabled;
extern bool wifiap_enabled;
extern bool mdns_enabled;

#endif
