#ifndef MODBUS_INVERTER_PROTOCOL_H
#define MODBUS_INVERTER_PROTOCOL_H

#include <HardwareSerial.h>
#include <stdint.h>
#include "../lib/eModbus-eModbus/ModbusServer.h"
#include "InverterProtocol.h"

extern uint16_t mbPV[];

// The abstract base class for all Modbus inverter protocols
class ModbusInverterProtocol : public InverterProtocol {
  const char* interface_name() { return "RS485 / Modbus"; }
  InverterInterfaceType interface_type() { return InverterInterfaceType::Modbus; }

 protected:
  // Create a ModbusRTU server instance with 2000ms timeout
  ModbusInverterProtocol();

  static const int MB_RTU_NUM_VALUES = 13100;

  // Modbus register file
  uint16_t* mbPV;

  ModbusServer* MBserver;
};

#endif
