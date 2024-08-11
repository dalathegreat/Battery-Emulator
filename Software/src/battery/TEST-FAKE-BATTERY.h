#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 9999

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
