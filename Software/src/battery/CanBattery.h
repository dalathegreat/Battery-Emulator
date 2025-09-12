#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "Battery.h"

#include "../../src/communication/Transmitter.h"
#include "../../src/communication/can/CanReceiver.h"
#include "../../src/communication/can/comm_can.h"
#include "../../src/devboard/utils/types.h"

// Abstract base class for batteries using the CAN bus
class CanBattery : public Battery, Transmitter, CanReceiver {
 public:
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

  const char* interface_name() { return getCANInterfaceName(can_interface); }

  void transmit(unsigned long currentMillis) { transmit_can(currentMillis); }

  void receive_can_frame(CAN_frame* frame) { handle_incoming_can_frame(*frame); }

 protected:
  CAN_Interface can_interface;
  CAN_Speed initial_speed;

  CanBattery(CAN_Speed speed = CAN_Speed::CAN_SPEED_500KBPS);
  CanBattery(CAN_Interface interface, CAN_Speed speed = CAN_Speed::CAN_SPEED_500KBPS);

  bool change_can_speed(CAN_Speed speed);
  void reset_can_speed();

  void transmit_can_frame(const CAN_frame* frame) { transmit_can_frame_to_interface(frame, can_interface); }
};

#endif
