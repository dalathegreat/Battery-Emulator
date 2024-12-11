#ifndef BMW_SBOX_CONTROL_H
#define BMW_SBOX_CONTROL_H
#include "../include.h"
#define CAN_SHUNT_SELECTED
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
