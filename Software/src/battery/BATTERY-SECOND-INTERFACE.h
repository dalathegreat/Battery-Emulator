#pragma once

#include "Shunt.h"

class BatterySecondInterfaceShunt : public CanShunt {
 public:
  void setup();
  void transmit_can(unsigned long currentMillis);
  void handle_incoming_can_frame(CAN_frame rx_frame);
  static constexpr const char* Name = "Battery second interface";
  void transmit_can_frame(CAN_frame* frame) { transmit_can_frame_to_interface(frame, can_interface); }
};
