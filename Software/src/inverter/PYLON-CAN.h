#ifndef PYLON_CAN_H
#define PYLON_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

class PylonCanInverter : public InverterProtocol {
 public:
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Pylontech battery over CAN bus";
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);
  virtual void update_values_can_inverter();
};

#endif
