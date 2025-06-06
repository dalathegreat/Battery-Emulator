#ifndef _SHUNT_H
#define _SHUNT_H

#include "../communication/can/comm_can.h"
#include "src/communication/Transmitter.h"
#include "src/devboard/utils/types.h"

class CanShunt : public Transmitter {
 public:
  virtual void setup() = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;

  void transmit(unsigned long currentMillis) {
    if (allowed_to_send_CAN) {
      transmit_can(currentMillis);
    }
  }

 protected:
  CAN_Interface can_interface;

  CanShunt() {
    can_interface = can_config.battery;
    register_transmitter(this);
  }
};

extern CanShunt* shunt;

#endif
