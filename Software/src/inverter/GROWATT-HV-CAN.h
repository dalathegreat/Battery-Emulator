#ifndef GROWATT_HV_CAN_H
#define GROWATT_HV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

void transmit_can_frame(CAN_frame* tx_frame, int interface);
void setup_inverter(void);

#endif
