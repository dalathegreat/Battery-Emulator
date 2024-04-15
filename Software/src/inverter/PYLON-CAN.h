#ifndef PYLON_CAN_H
#define PYLON_CAN_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

void update_values_can_pylon();
void receive_can_pylon(CAN_frame_t rx_frame);
void send_system_data();
void send_setup_info();

#endif
