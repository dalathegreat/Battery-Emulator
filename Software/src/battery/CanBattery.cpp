#include "CanBattery.h"
#include "../../src/include.h"

CanBattery::CanBattery(bool low_speed) {
  can_interface = can_config.battery;
  register_transmitter(this);
  register_can_receiver(this, can_interface, low_speed);
}

void CanBattery::slow_down_can() {
  ::slow_down_can(can_interface);
}

void CanBattery::resume_full_speed() {
  ::resume_full_speed(can_interface);
}
