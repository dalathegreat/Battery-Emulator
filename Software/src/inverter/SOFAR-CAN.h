#ifndef SOFAR_CAN_H
#define SOFAR_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class SofarCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Sofar BMS (Extended Frame) over CAN bus";
};

#endif
