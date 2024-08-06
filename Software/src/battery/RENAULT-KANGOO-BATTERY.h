#ifndef RENAULT_KANGOO_BATTERY_H
#define RENAULT_KANGOO_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define ABSOLUTE_CELL_MAX_VOLTAGE 4150  // If cellvoltage goes over this mV, we go into FAULT mode
#define ABSOLUTE_CELL_MIN_VOLTAGE 2500  // If cellvoltage goes under this mV, we go into FAULT mode
#define MAX_CELL_DEVIATION_MV 500       // If cell mV delta exceeds this, we go into WARNING mode
#define MAX_CHARGE_POWER_W 5000         // Battery can be charged with this amount of power

void setup_battery(void);
void transmit_can(CAN_frame_t* tx_frame, int interface);

#endif
