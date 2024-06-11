#ifndef CHARGERS_H
#define CHARGERS_H
#include "../../USER_SETTINGS.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"  // This include is annoying, consider defining a frame type in types.h

#ifdef CHEVYVOLT_CHARGER
#include "CHEVY-VOLT-CHARGER.h"
#endif

#ifdef NISSANLEAF_CHARGER
#include "NISSAN-LEAF-CHARGER.h"
#endif

#ifdef TSM_2500
#include "TSM-2500-CHARGER.h"
#endif

void receive_can_charger(CAN_frame_t rx_frame);
void send_can_charger();

#endif
