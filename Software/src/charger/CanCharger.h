#ifndef CAN_CHARGER_H
#define CAN_CHARGER_H

#include "src/devboard/utils/types.h"

class CanCharger {
 public:
  virtual void map_can_frame_to_variable(CAN_frame rx_frame) = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;
};

#endif
