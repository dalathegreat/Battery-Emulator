#ifndef CHEVYVOLT_CHARGER_H
#define CHEVYVOLT_CHARGER_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Charger hardware limits
 *
 * Relative to runtime settings, expectations are:
 * hw minimum <= setting minimum <= setting maximum <= hw max
 */
#define CHEVYVOLT_MAX_HVDC 420.0
#define CHEVYVOLT_MIN_HVDC 200.0
#define CHEVYVOLT_MAX_AMP 11.5
#define CHEVYVOLT_MAX_POWER 3300

void update_values_can_chevyvolt_charger();
void send_can_chevyvolt_charger();
void receive_can_chevyvolt_charger(CAN_frame_t rx_frame);

#endif
