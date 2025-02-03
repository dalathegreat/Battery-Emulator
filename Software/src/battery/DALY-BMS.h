#ifndef DALY_BMS_H
#define DALY_BMS_H
#include <Arduino.h>
#include "../include.h"

/* Tweak these according to your battery build */
#define CELL_COUNT 14
#define DESIGN_PACK_VOLTAGE_DB 528  //528 = 52.8V
#define MAX_PACK_VOLTAGE_DV 588     //588 = 58.8V
#define MIN_PACK_VOLTAGE_DV 518     //518 = 51.8V
#define MAX_CELL_VOLTAGE_MV 4250    //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700    //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 250
#define MAX_DISCHARGE_POWER_ALLOWED_W 1800
#define MAX_CHARGE_POWER_ALLOWED_W 1800
#define MAX_CHARGE_POWER_WHEN_TOPBALANCING_W 50
#define RAMPDOWN_SOC 9000  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define RS485_BATTERY_SELECTED

void setup_battery(void);
void receive_RS485(void);

#endif
