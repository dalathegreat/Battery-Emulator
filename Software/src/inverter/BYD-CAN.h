#ifndef BYD_CAN_H
#define BYD_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED
#define FW_MAJOR_VERSION 0x03
#define FW_MINOR_VERSION 0x29

void send_intial_data();
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
