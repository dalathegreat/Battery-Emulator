#include "../include.h"

// These functions adapt the old C-style global functions inverter-API to the
// object-oriented inverter protocol API.

#ifdef SELECTED_INVERTER_CLASS

InverterProtocol* inverter;

#ifdef CAN_INVERTER_SELECTED
CanInverterProtocol* can_inverter;
#endif

#ifdef MODBUS_INVERTER_SELECTED
ModbusInverterProtocol* modbus_inverter;
#endif

void setup_inverter() {
#ifdef MODBUS_INVERTER_SELECTED
  modbus_inverter = new SELECTED_INVERTER_CLASS();
  inverter = modbus_inverter;
#endif

#ifdef CAN_INVERTER_SELECTED
  can_inverter = new SELECTED_INVERTER_CLASS();
  inverter = can_inverter;
#endif

  inverter->setup();
}

#ifdef MODBUS_INVERTER_SELECTED
void update_modbus_registers_inverter() {
  modbus_inverter->update_modbus_registers();
}

void handle_static_data_modbus() {
  modbus_inverter->handle_static_data();
}
#endif

#ifdef CAN_INVERTER_SELECTED
void update_values_can_inverter() {
  can_inverter->update_values();
}

void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {
  can_inverter->map_can_frame_to_variable(rx_frame);
}

void transmit_can_inverter(unsigned long currentMillis) {
  can_inverter->transmit_can(currentMillis);
}
#endif

#endif
