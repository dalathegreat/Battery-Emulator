#ifndef MEB_GATEWAY_H
#define MEB_GATEWAY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
