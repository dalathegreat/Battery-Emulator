// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_SERVER_ETHERNET_H
#define _MODBUS_SERVER_ETHERNET_H
#include "options.h"
#if HAS_ETHERNET == 1
#include <Ethernet.h>
#include <SPI.h>

#undef SERVER_END
#define SERVER_END // NIL for Ethernet

// Create own non-virtual EthernetServer class
class EthernetServerEM : public EthernetServer {
public:
  explicit EthernetServerEM(uint16_t port) : EthernetServer(port) { }
  void begin(uint16_t port = 0) override { }
};

#include "ModbusServerTCPtemp.h"
using ModbusServerEthernet = ModbusServerTCP<EthernetServerEM, EthernetClient>;
#endif

#endif
