#ifndef SIMPBMS_BATTERY_H
#define SIMPBMS_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

/* DEFAULT VALUES BMS will send configured */
#define MAX_PACK_VOLTAGE_DV 5000  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 1500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 500
#define CELL_COUNT 96

class SimpBmsBattery : public CanBattery {
 public:
  SimpBmsBattery() : CanBattery(SimpBms) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "SIMPBMS battery";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can() {}
};

#endif
