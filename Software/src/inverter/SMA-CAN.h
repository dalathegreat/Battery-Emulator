#ifndef SMA_CAN_H
#define SMA_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

#define READY_STATE 0x03
#define STOP_STATE 0x02

void transmit_can(CAN_frame* tx_frame, int interface);

#endif
