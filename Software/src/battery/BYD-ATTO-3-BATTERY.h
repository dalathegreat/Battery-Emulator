#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 150
#define USE_ESTIMATED_SOC  // If enabled, SOC is estimated from pack voltage. Useful for locked packs. \
                           // Uncomment this if you know your BMS is unlocked and able to send SOC%

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
