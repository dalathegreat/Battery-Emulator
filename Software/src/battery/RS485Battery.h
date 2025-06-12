#ifndef RS485_BATTERY_H
#define RS485_BATTERY_H

#include "Battery.h"

#include "src/communication/Transmitter.h"
#include "src/devboard/utils/types.h"

#include "src/communication/rs485/comm_rs485.h"

// Abstract base class for batteries using the RS485 interface
class RS485Battery : public Battery, Transmitter, Rs485Receiver {
 public:
  virtual void transmit_rs485(unsigned long currentMillis) = 0;

  String interface_name() { return "RS485"; }

  void transmit(unsigned long currentMillis) { transmit_rs485(currentMillis); }

  RS485Battery() {
    register_transmitter(this);
    register_receiver(this);
  }
};

#endif
