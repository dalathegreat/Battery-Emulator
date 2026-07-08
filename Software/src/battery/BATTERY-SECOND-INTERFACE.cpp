#include "BATTERY-SECOND-INTERFACE.h"
#include "../datalayer/datalayer.h"

void BatterySecondInterfaceShunt::handle_incoming_can_frame(CAN_frame rx_frame) {}

void BatterySecondInterfaceShunt::transmit_can(unsigned long currentMillis) {}

void BatterySecondInterfaceShunt::setup() {
  strncpy(datalayer.system.info.shunt_protocol, Name, 31);
  datalayer.system.info.shunt_protocol[31] = '\0';
}
