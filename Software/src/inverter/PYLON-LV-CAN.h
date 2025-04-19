#ifndef PYLON_LV_CAN_H
#define PYLON_LV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

#define MANUFACTURER_NAME "BatEmuLV"
#define PACK_NUMBER 0x01
// 80 means after reaching 80% of a nominal value a warning is produced (e.g. 80% of max current)
#define WARNINGS_PERCENT 80

class PylonLvCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Pylontech LV battery over CAN bus";
};

#endif
