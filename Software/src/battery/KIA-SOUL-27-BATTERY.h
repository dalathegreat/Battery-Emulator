#ifndef KIA_SOUL_27_BATTERY_H
#define KIA_SOUL_27_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4110  //TODO, OK?
#define MIN_PACK_VOLTAGE_DV 2800  //TODO, OK?
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2950  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void update_number_of_cells();
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
