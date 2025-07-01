#ifndef _SHUNT_H
#define _SHUNT_H

#include "src/communication/Transmitter.h"
#include "src/communication/can/CanReceiver.h"
#include "src/devboard/utils/types.h"

class CanShunt : public Transmitter, CanReceiver {
 public:
  virtual void setup() = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;

  // The name of the comm interface the shunt is using.
  virtual String interface_name() { return getCANInterfaceName(can_config.shunt); }

  void transmit(unsigned long currentMillis) {
    if (allowed_to_send_CAN) {
      transmit_can(currentMillis);
    }
  }

  void receive_can_frame(CAN_frame* frame) { handle_incoming_can_frame(*frame); }

 protected:
  CAN_Interface can_interface;

  CanShunt() {
    can_interface = can_config.battery;
    register_transmitter(this);
    register_can_receiver(this, can_interface);
  }
};

extern CanShunt* shunt;

#endif
