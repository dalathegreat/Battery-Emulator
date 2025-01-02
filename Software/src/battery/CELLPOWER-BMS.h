#ifndef CELLPOWER_BMS_H
#define CELLPOWER_BMS_H
#include <Arduino.h>
#include "../include.h"

/* Tweak these according to your battery build */
#define MAX_PACK_VOLTAGE_DV 5000  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 1500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define NATIVECAN_250KBPS

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
