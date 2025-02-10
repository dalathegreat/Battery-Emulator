#ifndef FERROAMP_CAN_H
#define FERROAMP_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

void send_system_data();
void send_setup_info();
void transmit_can_frame(CAN_frame* tx_frame, int interface);
void setup_inverter(void);

#endif
