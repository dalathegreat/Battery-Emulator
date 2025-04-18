#ifndef SUNGROW_CAN_H
#define SUNGROW_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class SungrowCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Sungrow SBR064 battery over CAN bus";
};

#endif
