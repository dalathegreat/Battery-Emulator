#ifndef GEELY_GEOMETRY_C_BATTERY_H
#define GEELY_GEOMETRY_C_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_70_DV 4420  //70kWh
#define MIN_PACK_VOLTAGE_70_DV 2860
#define MAX_PACK_VOLTAGE_53_DV 4160  //53kWh
#define MIN_PACK_VOLTAGE_53_DV 2700
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
