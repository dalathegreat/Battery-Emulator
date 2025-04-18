#ifndef PYLON_CAN_H
#define PYLON_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class PylonCanInverter : public InverterProtocol {
 public:
  virtual void setup();
  virtual void transmit_can();
};

#endif
