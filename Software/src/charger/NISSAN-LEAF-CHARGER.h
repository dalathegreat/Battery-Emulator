#ifndef NISSANLEAF_CHARGER_H
#define NISSANLEAF_CHARGER_H
#include <Arduino.h>
#include "../include.h"

#define CHARGER_SELECTED

class NissanLeafCharger : Charger {
 public:
  virtual void map_can_frame_to_variable_charger(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
