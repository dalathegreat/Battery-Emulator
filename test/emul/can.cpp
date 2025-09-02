#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, int interface) {}

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {}

CAN_Speed change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  return CAN_Speed::CAN_SPEED_500KBPS;
}

void stop_can() {}

void restart_can() {}

char const* getCANInterfaceName(CAN_Interface) {
  return "Foobar";
}

void register_transmitter(Transmitter* transmitter) {}

void dump_can_frame(CAN_frame& frame, frameDirection msgDir) {}
