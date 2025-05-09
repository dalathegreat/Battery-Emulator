#ifndef CHARGERS_H
#define CHARGERS_H
#include "../../USER_SETTINGS.h"

#ifdef CHEVYVOLT_CHARGER
#include "CHEVY-VOLT-CHARGER.h"
#endif

#ifdef NISSANLEAF_CHARGER
#include "NISSAN-LEAF-CHARGER.h"
#endif

void map_can_frame_to_variable_charger(CAN_frame rx_frame);
void transmit_can_charger(unsigned long currentMillis);
void setup_charger();

#endif
