#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 150

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
