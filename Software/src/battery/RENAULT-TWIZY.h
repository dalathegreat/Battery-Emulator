#ifndef RENAULT_TWIZY_BATTERY_H
#define RENAULT_TWIZY_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 579  // 57.9V at 100% SOC (with 70% SOH, new one might be higher)
#define MIN_PACK_VOLTAGE_DV 480  // 48.4V at 13.76% SOC
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4200  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 3400  //Battery is put into emergency stop if one cell goes below this value

#endif
