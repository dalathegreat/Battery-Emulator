#ifndef KIA_HYUNDAI_HYBRID_BATTERY_H
#define KIA_HYUNDAI_HYBRID_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 100

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
