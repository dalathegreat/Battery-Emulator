#ifndef BMW_I3_BATTERY_H
#define BMW_I3_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

#define MAX_CELL_VOLTAGE_60AH 4110   // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_60AH 2700   // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_VOLTAGE_94AH 4140   // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_94AH 2700   // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_VOLTAGE_120AH 4190  // Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_120AH 2790  // Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION_MV 250    // LED turns yellow on the board if mv delta exceeds this value
#define MAX_PACK_VOLTAGE_60AH 3950   // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_60AH 2590   // Discharge stops if pack voltage exceeds this value
#define MAX_PACK_VOLTAGE_94AH 3980   // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_94AH 2590   // Discharge stops if pack voltage exceeds this value
#define MAX_PACK_VOLTAGE_120AH 4030  // Charge stops if pack voltage exceeds this value
#define MIN_PACK_VOLTAGE_120AH 2680  // Discharge stops if pack voltage exceeds this value
#define NUMBER_OF_CELLS 96
void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
