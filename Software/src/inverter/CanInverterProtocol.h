#ifndef CANINVERTER_PROTOCOL_H
#define CANINVERTER_PROTOCOL_H

#include "InverterProtocol.h"

#include "src/devboard/utils/types.h"

class CanInverterProtocol : public InverterProtocol, Transmitter {
 public:
  virtual const char* interface_name() { return getCANInterfaceName(can_config.inverter); }
  virtual void transmit_can(unsigned long currentMillis) = 0;
  virtual void map_can_frame_to_variable(CAN_frame rx_frame) = 0;

  void transmit(unsigned long currentMillis) {
    if (allowed_to_send_CAN) {
      transmit_can(currentMillis);
    }
  }

 protected:
  CanInverterProtocol() { register_transmitter(this); }
};

#endif
