#include "CanBattery.h"
#include "../../src/include.h"

CanBattery::CanBattery(bool halfSpeed) {
  can_interface = can_config.battery;
  register_transmitter(this);
  register_can_receiver(this, can_interface, halfSpeed);
}
