#ifndef RJXZS_BMS_H
#define RJXZS_BMS_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"

/* Tweak these according to your battery build */
#define MAX_PACK_VOLTAGE_DV 5000  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 1500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 250
#define MAX_DISCHARGE_POWER_ALLOWED_W 5000
#define MAX_CHARGE_POWER_ALLOWED_W 5000
#define MAX_CHARGE_POWER_WHEN_TOPBALANCING_W 500
#define RAMPDOWN_SOC 9000  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define NATIVECAN_250KBPS

class RjxzsBms : public CanBattery {
 public:
  RjxzsBms() : CanBattery(RjxzsBMS) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "RJXZS BMS, DIY battery";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
