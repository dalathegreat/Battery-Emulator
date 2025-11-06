#include "CanBattery.h"

CanBattery::CanBattery(CAN_Speed speed) : CanBattery(can_config.battery, speed) {}

CanBattery::CanBattery(CAN_Interface interface, CAN_Speed speed) {
  can_interface = interface;
  initial_speed = speed;
  register_transmitter(this);
  register_can_receiver(this, can_interface, speed);
}

bool CanBattery::change_can_speed(CAN_Speed speed) {
  return ::change_can_speed(can_interface, speed);
}

void CanBattery::reset_can_speed() {
  ::change_can_speed(can_interface, initial_speed);
}
