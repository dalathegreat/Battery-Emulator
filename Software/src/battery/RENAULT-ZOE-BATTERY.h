#ifndef RENAULT_ZOE_BATTERY_H
#define RENAULT_ZOE_BATTERY_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define ABSOLUTE_CELL_MAX_VOLTAGE 4100
#define ABSOLUTE_CELL_MIN_VOLTAGE 3000
#define MAX_CELL_DEVIATION_MV 500

void setup_battery(void);

#endif
