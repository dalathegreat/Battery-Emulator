#include "../include.h"
#include "Shunt.h"

CanShunt* shunt = nullptr;

void setup_can_shunt() {
#if defined(CAN_SHUNT_SELECTED) && defined(SELECTED_SHUNT_CLASS)
  shunt = new SELECTED_SHUNT_CLASS();
#endif
}

void handle_incoming_can_frame_shunt(CAN_frame rx_frame) {
  if (shunt) {
    shunt->handle_incoming_can_frame(rx_frame);
  }
}
