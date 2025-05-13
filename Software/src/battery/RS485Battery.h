#ifndef RS485_BATTERY_H
#define RS485_BATTERY_H

#include "Battery.h"

#include "src/devboard/utils/types.h"

// Abstract base class for batteries using the RS485 interface
class RS485Battery : public Battery {
 public:
  virtual void receive_RS485() = 0;
  virtual void transmit_rs485() = 0;
};

#endif
