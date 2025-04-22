#ifndef GROWATT_LV_CAN_H
#define GROWATT_LV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class GrowattLvCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Growatt Low Voltage (48V) protocol via CAN";
};

#endif
