#include "../include.h"
#ifdef MEB_GATEWAY_MODE
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MEB-GATEWAY.h"


CAN_frame Outgoing20Message = {.FD = false,.ext_ID = false, .DLC = 8,.ID = 0x123, .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};

void update_values_battery() {

  //Nothing to update, passive gateway mode
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {

}

void transmit_can_battery() {
  // No periodic sending
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "MEB Gateway mode for ODIS", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
}

#endif //MEB_GATEWAY_MODE
