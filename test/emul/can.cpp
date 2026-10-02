#include <vector>

#include "../../Software/src/communication/Transmitter.h"
#include "../../Software/src/communication/can/comm_can.h"

// Records every frame transmitted by the emulated CAN interface so unit tests
// can assert what the firmware actually put on the wire (UDS requests, ISO-TP
// flow control / consecutive frames, heartbeat frames, ...).
std::vector<CAN_frame> g_emul_transmitted_frames;

// Transmit hold, as in comm_can.cpp: held interfaces drop what is sent to them.
static uint8_t g_emul_held_mask = 0;
static int g_emul_abort_count = 0;
static bool g_emul_interface_shared = false;

// A fresh recording also starts from an unheld, unshared bus, so a test that ends with a hold set
// cannot silence the next one.
void clear_transmitted_frames() {
  g_emul_transmitted_frames.clear();
  g_emul_held_mask = 0;
  g_emul_abort_count = 0;
  g_emul_interface_shared = false;
}

bool emul_can_transmissions_held(CAN_Interface interface) {
  return (g_emul_held_mask & (1u << interface)) != 0;
}

// How many times a hold dropped pending transmissions.
int emul_can_abort_count() {
  return g_emul_abort_count;
}

void emul_set_can_interface_shared(bool shared) {
  g_emul_interface_shared = shared;
}

void hold_can_transmissions(CAN_Interface interface, bool hold) {
  if (interface >= NO_CAN_INTERFACE) {
    return;
  }
  const uint8_t bit = (uint8_t)(1u << interface);
  if (hold && (g_emul_held_mask & bit) == 0) {
    g_emul_held_mask |= bit;
    g_emul_abort_count++;
  } else if (!hold) {
    g_emul_held_mask &= (uint8_t)~bit;
  }
}

bool can_interface_shared(CAN_Interface interface) {
  return g_emul_interface_shared;
}

const std::vector<CAN_frame>& get_transmitted_frames() {
  return g_emul_transmitted_frames;
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  if (interface < NO_CAN_INTERFACE && (g_emul_held_mask & (1u << interface)) != 0) {
    return;
  }
  if (tx_frame != nullptr) {
    g_emul_transmitted_frames.push_back(*tx_frame);
  }
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
