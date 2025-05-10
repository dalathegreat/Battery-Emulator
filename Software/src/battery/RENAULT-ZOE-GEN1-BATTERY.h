#ifndef RENAULT_ZOE_GEN1_BATTERY_H
#define RENAULT_ZOE_GEN1_BATTERY_H

#include "CanBattery.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS RenaultZoeGen1Battery

#define MAX_PACK_VOLTAGE_DV 4200  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 3000
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

class RenaultZoeGen1Battery : public CanBattery {
 public:
  RenaultZoeGen1Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* allows_contactor_closing_ptr,
                        DATALAYER_INFO_NISSAN_LEAF* extended, int targetCan)
      : CanBattery(datalayer_ptr, allows_contactor_closing_ptr, extended, targetCan) {}
  RenaultZoeGen1Battery() : CanBattery() {}
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
};

#endif
