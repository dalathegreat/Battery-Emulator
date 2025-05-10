#ifndef CAN_BATTERY_H
#define CAN_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/utils/types.h"

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
// No support for double battery yet.
class CanBattery {
 public:
  CanBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* allows_contactor_closing_ptr,
             DATALAYER_INFO_NISSAN_LEAF* extended, int targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = allows_contactor_closing_ptr;
    datalayer_nissan = extended;
    can_interface = targetCan;
    Serial.println("Constructor1 called");
  }

  CanBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_nissan = &datalayer_extended.nissanleaf;
    can_interface = can_config.battery;
    Serial.println("Constructor2 called");
  }
  virtual void setup(void) = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  virtual void update_values() = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

 protected:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_NISSAN_LEAF* datalayer_nissan;
  bool* allows_contactor_closing;
  int can_interface;
};

#endif
