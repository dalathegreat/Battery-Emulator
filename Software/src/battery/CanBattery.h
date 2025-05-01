#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "src/devboard/utils/types.h"

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
// No support for double battery yet.
class CanBattery {
 public:
  virtual void setup(void) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void update_values() = 0;
  virtual void transmit_can() = 0;
};

#endif
