#ifndef CANINVERTER_PROTOCOL_H
#define CANINVERTER_PROTOCOL_H

#include "InverterProtocol.h"
#include "USER_SETTINGS.h"

#include "src/communication/Transmitter.h"
#include "src/communication/can/CanReceiver.h"
#include "src/communication/can/comm_can.h"
#include "src/devboard/safety/safety.h"
#include "src/devboard/utils/types.h"

class CanInverterProtocol : public InverterProtocol, Transmitter, CanReceiver {
 public:
  virtual const char* interface_name() { return getCANInterfaceName(can_interface); }
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
  CAN_Interface can_interface;

  CanInverterProtocol() {
    can_interface = can_config.inverter;
    register_transmitter(this);
    register_can_receiver(this, can_interface);
  }

  void transmit_can_frame(CAN_frame* frame) { transmit_can_frame_to_interface(frame, can_interface); }
};

#endif
