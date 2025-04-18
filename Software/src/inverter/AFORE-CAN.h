#ifndef AFORE_CAN_H
#define AFORE_CAN_H
//#include "../include.h"
#include "Inverter.h"

#define CAN_INVERTER_SELECTED

class AforeCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Afore battery over CAN";
};

#endif
