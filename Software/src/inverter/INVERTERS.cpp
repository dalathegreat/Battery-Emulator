#include "../include.h"

InverterProtocol* inverter = nullptr;

void setup_inverter() {
  if (inverter) {
    // The inverter is setup only once.
    return;
  }

  inverter = new SELECTED_INVERTER_CLASS();

  if (inverter) {
    inverter->setup();
  }
}
