#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "Battery.h"

#include "src/devboard/utils/types.h"

// Abstract base class for batteries using the CAN bus
class CanBattery : public Battery {
 public:
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

  String interface_name() { return getCANInterfaceName(can_interface); }

 protected:
  CAN_Interface can_interface;

  CanBattery() { can_interface = can_config.battery; }

  CanBattery(CAN_Interface interface) { can_interface = interface; }
};

#endif
