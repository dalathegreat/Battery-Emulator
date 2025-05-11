#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "../datalayer/datalayer.h"
#include "src/devboard/utils/types.h"

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
class CanBattery {
 public:
  CanBattery(int batteryNumber, int targetCan) {
    battery_number = batteryNumber;
    can_interface = targetCan;

    switch (batteryNumber) {
      case 2:
        datalayer_battery = &datalayer.battery2;
        allows_contactor_closing = &datalayer.system.status.battery2_allows_contactor_closing;
        break;
      default:
        datalayer_battery = &datalayer.battery;
        allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
        break;
    }
  }

  virtual void setup(void) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void update_values() = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

 protected:
  int battery_number;
  int can_interface;
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  bool* allows_contactor_closing;
};

#endif
