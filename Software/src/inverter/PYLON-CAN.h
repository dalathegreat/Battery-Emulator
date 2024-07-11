#ifndef PYLON_CAN_H
#define PYLON_CAN_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define CAN_INVERTER_SELECTED

void send_system_data();
void send_setup_info();

#endif
