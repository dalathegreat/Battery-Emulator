#ifndef FOXESS_BATTERY_H
#define FOXESS_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

#define MAX_PACK_VOLTAGE_DV 4672  //467.2V for HS20.8 (used during startup, refined later)
#define MIN_PACK_VOLTAGE_DV 800   //80.V for HS5.2 (used during startup, refined later)
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 3800  //LiFePO4 Prismaticc Cell
#define MIN_CELL_VOLTAGE_MV 2700  //LiFePO4 Prismatic Cell
void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
