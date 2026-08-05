#include <vector>

#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"
#include "../../Software/src/communication/can/comm_can_device.h"

// Records every frame transmitted by the emulated CAN interface so unit tests
// can assert what the firmware actually put on the wire (UDS requests, ISO-TP
// flow control / consecutive frames, heartbeat frames, ...).
std::vector<CAN_frame> g_emul_transmitted_frames;

void clear_transmitted_frames() {
  g_emul_transmitted_frames.clear();
}

const std::vector<CAN_frame>& get_transmitted_frames() {
  return g_emul_transmitted_frames;
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  if (tx_frame != nullptr) {
    g_emul_transmitted_frames.push_back(*tx_frame);
  }
}

// register_can_receiver, the fan-out, the device registry and the health-event
// aggregation all come from the real comm_can_dispatch.cpp now.

// The logical->physical map is no longer modelled here: comm_can_dispatch.cpp
// owns the real one and is in the test build, so tests wire a board with
// map_interface_to_device() and there is a single source of truth.

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
