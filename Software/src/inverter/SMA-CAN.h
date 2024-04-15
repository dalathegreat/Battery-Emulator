#ifndef SMA_CAN_H
#define SMA_CAN_H
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

#define READY_STATE 0x03
#define STOP_STATE 0x02

void update_values_can_sma();
void send_can_sma();
void receive_can_sma(CAN_frame_t rx_frame);

#endif
