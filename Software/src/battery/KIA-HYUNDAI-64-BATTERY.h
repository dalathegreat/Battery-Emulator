#ifndef KIA_HYUNDAI_64_BATTERY_H
#define KIA_HYUNDAI_64_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define MAXCHARGEPOWERALLOWED 10000
#define MAXDISCHARGEPOWERALLOWED 10000

void setup_battery(void);

#endif
