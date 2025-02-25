#ifndef RANGE_ROVER_PHEV_BATTERY_H
#define RANGE_ROVER_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

/* Change the following to suit your battery */
#define MAX_PACK_VOLTAGE_DV 5000  //TODO: Configure
#define MIN_PACK_VOLTAGE_DV 0     //TODO: Configure
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 150

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
