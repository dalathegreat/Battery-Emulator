#ifndef CANINVERTER_PROTOCOL_H
#define CANINVERTER_PROTOCOL_H

#include "InverterProtocol.h"

#include "src/communication/can/CanReceiver.h"
#include "src/devboard/utils/types.h"

class CanInverterProtocol : public InverterProtocol, Transmitter, CanReceiver {
 public:
  virtual const char* interface_name() { return getCANInterfaceName(can_config.inverter); }
  InverterInterfaceType interface_type() { return InverterInterfaceType::Can; }

  virtual void transmit_can(unsigned long currentMillis) = 0;
  virtual void map_can_frame_to_variable(CAN_frame rx_frame) = 0;

  void transmit(unsigned long currentMillis) {
    if (allowed_to_send_CAN) {
      transmit_can(currentMillis);
    }
  }

  void receive_can_frame(CAN_frame* frame) { map_can_frame_to_variable(*frame); }

 protected:
  CanInverterProtocol() {
    register_transmitter(this);
    register_can_receiver(this, can_config.inverter);
  }
};

#endif
