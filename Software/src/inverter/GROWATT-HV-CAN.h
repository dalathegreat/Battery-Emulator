#ifndef GROWATT_HV_CAN_H
#define GROWATT_HV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class GrowattHvInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Growatt High Voltage protocol via CAN";
};

#endif
