#ifndef GROWATT_LV_CAN_H
#define GROWATT_LV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class GrowattLvCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Growatt Low Voltage (48V) protocol via CAN";
};

#endif
