#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "Battery.h"

#include "src/communication/Transmitter.h"
#include "src/communication/can/CanReceiver.h"
#include "src/devboard/utils/types.h"

// Abstract base class for batteries using the CAN bus
class CanBattery : public Battery, Transmitter, CanReceiver {
 public:
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

  String interface_name() { return getCANInterfaceName(can_interface); }

  void transmit(unsigned long currentMillis) { transmit_can(currentMillis); }

  void receive_can_frame(CAN_frame* frame) { handle_incoming_can_frame(*frame); }

 protected:
  CAN_Interface can_interface;

  CanBattery();

  CanBattery(CAN_Interface interface) {
    can_interface = interface;
    register_transmitter(this);
    register_can_receiver(this, can_interface);
  }
};

#endif
