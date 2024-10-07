#ifndef VOLVO_SPA_BATTERY_H
#define VOLVO_SPA_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4540  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 2938
#define MAX_CELL_DEVIATION_MV 250
#define MAX_CELL_VOLTAGE_MV 4210  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
