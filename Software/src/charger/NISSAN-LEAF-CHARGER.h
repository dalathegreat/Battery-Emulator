#ifndef NISSANLEAF_CHARGER_H
#define NISSANLEAF_CHARGER_H
#include <Arduino.h>
#include "../include.h"

#define CHARGER_SELECTED

class NissanLeafCharger : public Charger {
 public:
  NissanLeafCharger() : Charger(ChargerType::NissanLeaf) {}

  virtual void map_can_frame_to_variable_charger(CAN_frame rx_frame);
  virtual void transmit_can();
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Nissan LEAF 2013-2024 PDM charger";
};

#endif
