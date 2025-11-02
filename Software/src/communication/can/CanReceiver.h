#ifndef _CANRECEIVER_H
#define _CANRECEIVER_H

#include "../../devboard/utils/types.h"

class CanReceiver {
 public:
  virtual void receive_can_frame(CAN_frame* rx_frame) = 0;
};

#endif
