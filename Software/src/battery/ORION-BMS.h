#ifndef ORION_BMS_H
#define ORION_BMS_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

/* Change the following to suit your battery */
#define NUMBER_OF_CELLS 96
#define MAX_PACK_VOLTAGE_DV 5000  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 1500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 150

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
