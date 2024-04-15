#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED

// These parameters need to be mapped for the inverter
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void setup_battery(void);

#endif
