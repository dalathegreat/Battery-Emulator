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
void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

void init_WiFi_AP();

// Initialise mDNS
void init_mDNS();

extern bool wifi_enabled;
extern bool wifiap_enabled;
extern bool mdns_enabled;

#endif
