#ifndef BMW_IX_BATTERY_H
#define BMW_IX_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

#define MAX_PACK_VOLTAGE_DV 4650  //4650 = 465.0V
#define MIN_PACK_VOLTAGE_DV 3000
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 4300  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2800  //Battery is put into emergency stop if one cell goes below this value
#define MAX_DISCHARGE_POWER_ALLOWED_W 10000
#define MAX_CHARGE_POWER_ALLOWED_W 10000
#define MAX_CHARGE_POWER_WHEN_TOPBALANCING_W 500
#define RAMPDOWN_SOC 9000  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%
#define STALE_PERIOD_CONFIG \
  900000;  //Number of milliseconds before critical values are classed as stale/stuck 900000 = 900 seconds
void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
