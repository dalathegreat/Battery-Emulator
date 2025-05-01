#include "../include.h"

// These functions adapt the old C-style global functions inverter-API to the
// object-oriented inverter protocol API.

#ifdef OO_INVERTER_PROTOCOL_SELECTED

ModbusInverterProtocol* inverter;

void setup_inverter() {
  // Currently only a single inverter protocol is implemented as a class
  inverter = new BydModbusInverter();
  inverter->setup();
}

void update_modbus_registers_inverter() {
  inverter->update_modbus_registers();
}

void handle_static_data_modbus() {
  inverter->handle_static_data();
}

#endif
