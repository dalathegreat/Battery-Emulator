#include "VCU-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../inverter/INVERTERS.h"

void VCUInverter::update_values() {}

void VCUInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1A:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void VCUInverter::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    transmit_can_frame(&LEAF_1DC);
    transmit_can_frame(&LEAF_1DB);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&LEAF_55B);
    transmit_can_frame(&LEAF_5BC);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&LEAF_59E);
    transmit_can_frame(&LEAF_5C0);
  }
}
