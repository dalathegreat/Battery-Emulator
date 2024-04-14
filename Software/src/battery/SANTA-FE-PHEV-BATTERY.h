#ifndef SANTA_FE_PHEV_BATTERY_H
#define SANTA_FE_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

// These parameters need to be mapped for the inverter
extern uint16_t system_scaled_SOC_pptt;     //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;       //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;      //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;  //Bool, true/false

uint8_t CalculateCRC8(CAN_frame_t rx_frame);
void setup_battery(void);

#endif
