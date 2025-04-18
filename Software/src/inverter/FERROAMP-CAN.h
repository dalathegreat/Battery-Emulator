#ifndef FERROAMP_CAN_H
#define FERROAMP_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class FerroampCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Ferroamp Pylon battery over CAN bus";
};

#endif
