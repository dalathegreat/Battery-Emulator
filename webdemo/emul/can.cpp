#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"

void dump_can_frame2(const CAN_frame& frame, CAN_Interface interface, frameDirection msgDir);
void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  dump_can_frame2(*tx_frame, interface, frameDirection(MSG_TX));
}
