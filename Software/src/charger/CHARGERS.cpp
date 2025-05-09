#include "../include.h"

#ifdef SELECTED_CHARGER_CLASS

static CanCharger* charger;

void map_can_frame_to_variable_charger(CAN_frame rx_frame) {
  charger->map_can_frame_to_variable(rx_frame);
}

void transmit_can_charger(unsigned long currentMillis) {
  charger->transmit_can(currentMillis);
}

void setup_charger() {
  charger = new SELECTED_CHARGER_CLASS();
}

#endif
