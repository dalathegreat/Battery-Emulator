#ifndef CHARGERS_H
#define CHARGERS_H
#include "../../USER_SETTINGS.h"

#ifdef CHEVYVOLT_CHARGER
#include "CHEVY-VOLT-CHARGER.h"
#endif

#ifdef NISSANLEAF_CHARGER
#include "NISSAN-LEAF-CHARGER.h"
#endif

void receive_can_charger(CAN_frame rx_frame);
void send_can_charger();

#endif
