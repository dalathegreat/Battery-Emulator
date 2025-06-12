#ifndef _RS485INVERTER_PROTOCOL_H
#define _RS485INVERTER_PROTOCOL_H

#include "InverterProtocol.h"

#include "src/communication/rs485/comm_rs485.h"

class Rs485InverterProtocol : public InverterProtocol, Rs485Receiver {
 public:
  virtual const char* interface_name() { return "RS485"; }
  InverterInterfaceType interface_type() { return InverterInterfaceType::Rs485; }
  virtual int baud_rate() = 0;
};

#endif
