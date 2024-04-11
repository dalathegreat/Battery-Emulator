#ifndef NISSANLEAF_CHARGER_H
#define NISSANLEAF_CHARGER_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

extern uint16_t system_battery_voltage_dV;  //V+1,  0-1000.0 (0-10000)

void send_can_nissanleaf_charger();
void receive_can_nissanleaf_charger(CAN_frame_t rx_frame);

#endif
