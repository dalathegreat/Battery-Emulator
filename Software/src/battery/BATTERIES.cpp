#include "BATTERIES.h"
#include "../include.h"

// These functions adapt the old C-style global functions battery-API to the
// object-oriented battery API.

#ifdef OO_BATTERY_SELECTED

static CanBattery* battery;

void setup_battery() {
  // Currently only one battery is implemented as a class.
  // TODO: Extend based on build-time or run-time selected battery.
  battery = new RenaultZoeGen1Battery();
  battery->setup();
}

void update_values_battery() {
  battery->update_values();
}

void transmit_can_battery() {
  battery->transmit_can();
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  battery->handle_incoming_can_frame(rx_frame);
}

#endif
