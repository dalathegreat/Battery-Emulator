#ifndef BYD_CAN_H
#define BYD_CAN_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

void update_values_can_byd();
void send_can_byd();
void receive_can_byd(CAN_frame_t rx_frame);
void send_intial_data();

#endif
