#pragma once

#ifdef ETHERNET

#include <ETH.h>
#include <WiFi.h>  // WiFiEvent_t / WiFiEventInfo_t (arduino-esp32 dispatches ETH events through this too)

// Bring up the on-board Ethernet PHY using pins provided by the HAL
void init_Ethernet();

// True after the PHY reports link-up and the interface has an IP
bool ethernet_connected();

extern bool eth_static_IP_enabled;
extern IPAddress eth_static_local_IP;
extern IPAddress eth_static_gateway;
extern IPAddress eth_static_subnet;
extern IPAddress eth_static_dns;  // Unset (0.0.0.0) = use the gateway as resolver

#endif  // ETHERNET
