#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"

#include <vector>

// Test hook: every frame handed to the (real) firmware TX path is captured here so tests can
// inspect what a sender actually put on the wire. Cleared by tests via emul_tx_frames().clear().
std::vector<CAN_frame>& emul_tx_frames() {
  static std::vector<CAN_frame> frames;
  return frames;
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  emul_tx_frames().push_back(*tx_frame);
}

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {}

bool change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  return true;
}

void stop_can() {}

void restart_can() {}

char const* getCANInterfaceName(CAN_Interface) {
  return "Foobar";
}

void register_transmitter(Transmitter* transmitter) {}

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {}
