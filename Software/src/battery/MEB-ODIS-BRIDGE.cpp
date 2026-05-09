#include "MEB-ODIS-BRIDGE.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void MebOdisBridge::update_values() {
  datalayer.system.info.can_ODIS_bridge_active = true;
  //Nothing to update, passive gateway mode
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void MebOdisBridge::handle_incoming_can_frame(CAN_frame rx_frame) {
  //TODO, move handling from comm_can.cpp into here in some smart way
}

void MebOdisBridge::transmit_can(unsigned long currentMillis) {
  //No periodic sending for this protocol
}

void MebOdisBridge::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.info.can_ODIS_bridge_active = true;
}
