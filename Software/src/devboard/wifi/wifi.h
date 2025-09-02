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
extern bool static_IP_enabled;
extern uint16_t static_local_IP1;
extern uint16_t static_local_IP2;
extern uint16_t static_local_IP3;
extern uint16_t static_local_IP4;
extern uint16_t static_gateway1;
extern uint16_t static_gateway2;
extern uint16_t static_gateway3;
extern uint16_t static_gateway4;
extern uint16_t static_subnet1;
extern uint16_t static_subnet2;
extern uint16_t static_subnet3;
extern uint16_t static_subnet4;

#endif
