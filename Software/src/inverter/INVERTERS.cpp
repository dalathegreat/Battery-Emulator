#include "../include.h"

InverterProtocol* inverter = nullptr;

void setup_inverter() {
  if (inverter) {
    // The inverter is setup only once.
    return;
  }

#ifdef SELECTED_INVERTER_CLASS
  inverter = new SELECTED_INVERTER_CLASS();

  if (inverter) {
    inverter->setup();
  }
#endif
}
