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

// register_can_receiver and the fan-out now come from the real
// comm_can_dispatch.cpp, which is hardware-free and in the test build.

const std::vector<CanDevice*>& unique_can_devices() {
  static std::vector<CanDevice*> no_devices;
  return no_devices;
}

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
