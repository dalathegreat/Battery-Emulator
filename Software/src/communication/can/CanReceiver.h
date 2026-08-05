#ifndef _CANRECEIVER_H
#define _CANRECEIVER_H

#include "../../devboard/utils/types.h"

class CanReceiver {
 protected:
  // Never deleted through the base - see Battery.h for the pattern rationale
  ~CanReceiver() = default;

 public:
  virtual void receive_can_frame(CAN_frame* rx_frame) = 0;
};

#endif
