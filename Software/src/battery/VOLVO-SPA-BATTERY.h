#ifndef VOLVO_SPA_BATTERY_H
#define VOLVO_SPA_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 250

void setup_battery(void);

#endif
