#ifndef BALANCELL_P33_H
#define BALANCELL_P33_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 9999

void setup_battery(void);

#endif
