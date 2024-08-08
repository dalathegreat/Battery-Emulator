#ifndef SMA_CAN_TRIPOWER_H
#define SMA_CAN_TRIPOWER_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

void send_tripower_init();
void transmit_can(CAN_frame_t* tx_frame, int interface);

#endif
