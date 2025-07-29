#include "CanBattery.h"

CanBattery::CanBattery(CAN_Speed speed) {
  can_interface = can_config.battery;
  register_transmitter(this);
  register_can_receiver(this, can_interface, speed);
}

CAN_Speed CanBattery::change_can_speed(CAN_Speed speed) {
  return ::change_can_speed(can_interface, speed);
}
