#pragma once

#include <WiFi.h>

// Shared network-state helpers

bool network_connected();

IPAddress network_localIP();

// Bring up the interface-agnostic services that run once any interface acquires
// an IP: log the address, start syslog, and start the mDNS responder. Called
// from GOT_IP handlers
void network_bring_services_up(const IPAddress& ip);
