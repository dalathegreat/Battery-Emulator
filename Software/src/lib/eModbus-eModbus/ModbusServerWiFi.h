// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_SERVER_WIFI_H
#define _MODBUS_SERVER_WIFI_H
#include "options.h"
#include <WiFi.h>

#undef SERVER_END
#define SERVER_END server.end();

#include "ModbusServerTCPtemp.h"
using ModbusServerWiFi = ModbusServerTCP<WiFiServer, WiFiClient>;

#endif
