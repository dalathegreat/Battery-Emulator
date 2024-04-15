#ifndef SOFAR_CAN_H
#define SOFAR_CAN_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

// These parameters need to be mapped for the inverter
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

extern uint16_t min_voltage;
extern uint16_t max_voltage;

void update_values_can_sofar();
void send_can_sofar();
void receive_can_sofar(CAN_frame_t rx_frame);

#endif
