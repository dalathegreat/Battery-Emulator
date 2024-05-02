#ifndef RENAULT_KANGOO_BATTERY_H
#define RENAULT_KANGOO_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define ABSOLUTE_CELL_MAX_VOLTAGE \
  4100  // Max Cell Voltage mV! if voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_CELL_MIN_VOLTAGE \
  3000                             // Min Cell Voltage mV! if voltage goes under this, discharging further is disabled
#define MAX_CELL_DEVIATION_MV 500  //LED turns yellow on the board if mv delta exceeds this value
#define MAX_CHARGE_POWER_W 5000    // Battery can be charged with this amount of power

void setup_battery(void);

#endif
