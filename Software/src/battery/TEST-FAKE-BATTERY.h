#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED

// These parameters need to be mapped for the inverter
extern uint16_t system_scaled_SOC_pptt;      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;       //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void setup_battery(void);

#endif
