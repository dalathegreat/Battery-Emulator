#include "../include.h"

// These functions adapt the old C-style global functions inverter-API to the
// object-oriented inverter protocol API.

InverterProtocol* inverter = nullptr;

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

#ifdef RS485_INVERTER_SELECTED
  inverter = new SELECTED_INVERTER_CLASS();
#endif

  if (inverter) {
    inverter->setup();
  }
}

#ifdef CAN_INVERTER_SELECTED

void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {
  can_inverter->map_can_frame_to_variable(rx_frame);
}
#endif

#ifdef RS485_INVERTER_SELECTED
void receive_RS485() {
  ((Rs485InverterProtocol*)inverter)->receive_RS485();
}
#endif
