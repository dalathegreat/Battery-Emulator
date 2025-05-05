#include "../include.h"

// These functions adapt the old C-style global functions battery-API to the
// object-oriented battery API.

// The instantiated class is defined by the pre-compiler define
// to support battery class selection at compile-time
#ifdef SELECTED_BATTERY_CLASS

static CanBattery* battery = nullptr;

void setup_battery() {
  // Instantiate the battery only once just in case this function gets called multiple times.
  if (battery == nullptr) {
    battery = new SELECTED_BATTERY_CLASS();
  }
  battery->setup();
}

void update_values_battery() {
  battery->update_values();
}

void transmit_can_battery(unsigned long currentMillis) {
  battery->transmit_can(currentMillis);
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  battery->handle_incoming_can_frame(rx_frame);
}

#endif
