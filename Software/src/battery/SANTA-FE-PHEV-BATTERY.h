#ifndef SANTA_FE_PHEV_BATTERY_H
#define SANTA_FE_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 250

uint8_t CalculateCRC8(CAN_frame_t rx_frame);
void setup_battery(void);

#endif
