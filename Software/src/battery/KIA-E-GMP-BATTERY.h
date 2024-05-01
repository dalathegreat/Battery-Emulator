#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"

extern ACAN2517FD canfd;

#define BATTERY_SELECTED

#define MAXCHARGEPOWERALLOWED 10000
#define MAXDISCHARGEPOWERALLOWED 10000

void setup_battery(void);

#endif
