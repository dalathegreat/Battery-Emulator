#ifndef BMW_I3_BATTERY_H
#define BMW_I3_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-acan2515/ACAN2515.h"

extern ACAN2515 can;

#define BATTERY_SELECTED

#define WUP_PIN 25
void setup_battery(void);

#endif
