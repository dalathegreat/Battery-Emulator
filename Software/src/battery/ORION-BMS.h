#ifndef ORION_BMS_H
#define ORION_BMS_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

/* Change the following to suit your battery */
#define NUMBER_OF_CELLS 96
#define MAX_PACK_VOLTAGE_DV 5000  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 1500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 150

class OrionBms : public CanBattery {
 public:
  OrionBms() : CanBattery(OrionBMS) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "DIY battery with Orion BMS (Victron setting)";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);

  // No transmit needed for this type
  virtual void transmit_can() {}
};

#endif
