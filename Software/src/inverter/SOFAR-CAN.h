#ifndef SOFAR_CAN_H
#define SOFAR_CAN_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

void update_values_can_sofar();
void send_can_sofar();
void receive_can_sofar(CAN_frame_t rx_frame);

#endif
