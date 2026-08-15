#include "network_status.h"

#include <WiFi.h>

#include "../utils/logging.h"  // logging, syslog_start()
#include "../wifi/wifi.h"      // wifi_connected()
#include "mdns.h"              // init_mDNS()

#ifdef ETHERNET
#include "../ethernet/ethernet.h"
#endif

extern const char* version_number;  // defined in Software.cpp

bool network_connected() {
#ifdef ETHERNET
  if (ethernet_connected()) {
    return true;
  }
#endif
  return wifi_connected();
}

IPAddress network_localIP() {
#ifdef ETHERNET
  if (ethernet_connected()) {
    return ETH.localIP();
  }
#endif
  return WiFi.localIP();
}

void network_bring_services_up(const IPAddress& ip) {
  // One-shot boot notice — fires once per boot, not on every reconnect.
  static bool boot_logged = false;
  if (!boot_logged) {
    boot_logged = true;
    LOG_SET_NEXT_SEVERITY(5);  // RFC 5424 severity 5 = Notice
    logging.printf("Bootup complete, running version %s\n", version_number);
  }

  LOG_SET_NEXT_SEVERITY(5);  // notice
  logging.printf("Got IP address: %s\n", ip.toString().c_str());
  syslog_start();  // safe to call more than once
#ifndef SMALL_FLASH_DEVICE
  init_mDNS();
#endif
}
