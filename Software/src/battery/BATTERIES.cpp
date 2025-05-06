#include "../include.h"

// These functions adapt the old C-style global functions battery-API to the
// object-oriented battery API.

// The instantiated class is defined by the pre-compiler define
// to support battery class selection at compile-time
#ifdef SELECTED_BATTERY_CLASS

static CanBattery* battery = nullptr;

#ifdef DOUBLE_BATTERY
static CanBattery* battery2 = nullptr;
#endif

void setup_battery() {
  // Instantiate the battery only once just in case this function gets called multiple times.
  if (battery == nullptr) {
    battery = new SELECTED_BATTERY_CLASS();
  }
  battery->setup();

#ifdef DOUBLE_BATTERY
  if (battery2 == nullptr) {
    battery2 =
        new SELECTED_BATTERY_CLASS(&datalayer.battery2, &datalayer.system.status.battery2_allows_contactor_closing,
                                   nullptr, can_config.battery_double);
  }
  battery2->setup();
#endif
}

void update_values_battery() {
  battery->update_values();
}

// transmit_can_battery is called once and we need to
// call both batteries.
void transmit_can_battery(unsigned long currentMillis) {
  battery->transmit_can(currentMillis);

#ifdef DOUBLE_BATTERY
  if (battery2) {
    battery2->transmit_can(currentMillis);
  }
#endif
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  battery->handle_incoming_can_frame(rx_frame);
}

#ifdef DOUBLE_BATTERY
void update_values_battery2() {
  battery2->update_values();
}

void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  battery2->handle_incoming_can_frame(rx_frame);
}
#endif

#endif
