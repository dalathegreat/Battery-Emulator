#ifndef RS485CANINVERTER_PROTOCOL_H
#define RS485INVERTER_PROTOCOL_H

#include "InverterProtocol.h"

class Rs485InverterProtocol : public InverterProtocol {
 public:
  virtual const char* interface_name() { return "RS485"; }
  virtual void receive_RS485() = 0;
  virtual int baud_rate() = 0;
};

#endif
