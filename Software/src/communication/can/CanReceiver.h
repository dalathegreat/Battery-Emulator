#ifndef _CANRECEIVER_H
#define _CANRECEIVER_H

#include "src/devboard/utils/types.h"

class CanReceiver {
 public:
  virtual void receive_can_frame(CAN_frame* rx_frame) = 0;
};

// Register a receiver object for a given CAN interface
void register_can_receiver(CanReceiver* receiver, CAN_Interface interface);

#endif
