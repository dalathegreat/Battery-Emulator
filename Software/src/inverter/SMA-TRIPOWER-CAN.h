#ifndef SMA_CAN_TRIPOWER_H
#define SMA_CAN_TRIPOWER_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

void update_values_can_sma_tripower();
void send_can_sma_tripower();
void receive_can_sma_tripower(CAN_frame_t rx_frame);
void send_tripower_init();

#endif
