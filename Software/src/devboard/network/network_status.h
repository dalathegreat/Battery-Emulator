#pragma once

#include <WiFi.h>

// Shared network-state helpers

bool network_connected();

IPAddress network_localIP();

// Bring up the interface-agnostic services that run once any interface acquires
// an IP: log the address, start syslog, and start the mDNS responder. Called
// from GOT_IP handlers
void network_bring_services_up(const IPAddress& ip, const char* iface);

#ifdef ETHERNET
// Set the default route and DNS source to the highest-priority connected
// interface (Ethernet > WiFi STA). Call from every GOT_IP and DISCONNECTED
// handler so the choice is always deterministic.
void network_update_default_interface();
#endif
