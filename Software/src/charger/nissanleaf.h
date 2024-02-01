#ifndef NISSANLEAF_H
#define NISSANLEAF_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

extern uint16_t battery_voltage;             //V+1,  0-500.0 (0-5000)

void update_values_can_nissanleaf_charger();
void send_can_nissanleaf_charger();
void receive_can_nissanleaf_charger(CAN_frame_t rx_frame);
uint8_t calculate_CRC_Nissan(CAN_frame_t* frame);
uint8_t calculate_checksum_nibble(CAN_frame_t *frame);

#endif
