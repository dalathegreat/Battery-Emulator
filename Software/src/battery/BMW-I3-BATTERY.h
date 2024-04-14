#ifndef BMW_I3_BATTERY_H
#define BMW_I3_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define WUP_PIN 25

// These parameters need to be mapped for the inverter
extern uint16_t system_scaled_SOC_pptt;     //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;       //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;      //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;  //Bool, true/false

void setup_battery(void);

#endif
