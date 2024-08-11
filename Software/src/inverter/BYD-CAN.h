#ifndef BYD_CAN_H
#define BYD_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

void send_intial_data();
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
