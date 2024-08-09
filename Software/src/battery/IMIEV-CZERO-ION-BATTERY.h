#ifndef IMIEV_CZERO_ION_BATTERY_H
#define IMIEV_CZERO_ION_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 500

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
