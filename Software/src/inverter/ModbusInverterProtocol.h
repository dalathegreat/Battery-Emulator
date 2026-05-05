#ifndef MODBUS_INVERTER_PROTOCOL_H
#define MODBUS_INVERTER_PROTOCOL_H

#include "../lib/eModbus-eModbus/ModbusMessage.h"
#include "../lib/eModbus-eModbus/ModbusServerRTU.h"
#include "InverterProtocol.h"

#include <HardwareSerial.h>

#include <stdint.h>
#include <map>

// The abstract base class for all Modbus inverter protocols
class ModbusInverterProtocol : public InverterProtocol {
  const char* interface_name() { return "RS485 / Modbus"; }
  InverterInterfaceType interface_type() { return InverterInterfaceType::Modbus; }

 protected:
  ModbusInverterProtocol(int serverId);
  ~ModbusInverterProtocol();

  ModbusMessage FC03(ModbusMessage request);
  ModbusMessage FC06(ModbusMessage request);
  ModbusMessage FC16(ModbusMessage request);
  ModbusMessage FC23(ModbusMessage request);

  // The highest Modbus register we allow reads/writes from
  static const int MBPV_MAX = 30000;
  // The Modbus server ID we respond to
  int _serverId;
  // The Modbus registers themselves
  std::map<uint16_t, uint16_t> mbPV;

  ModbusServerRTU MBserver;
};

#endif
