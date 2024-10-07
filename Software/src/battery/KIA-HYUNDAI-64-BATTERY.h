#ifndef KIA_HYUNDAI_64_BATTERY_H
#define KIA_HYUNDAI_64_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_98S_DV 4070  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_98S_DV 2800
#define MAX_PACK_VOLTAGE_90S_DV 3870
#define MIN_PACK_VOLTAGE_90S_DV 2250
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2950  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void update_number_of_cells();
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
