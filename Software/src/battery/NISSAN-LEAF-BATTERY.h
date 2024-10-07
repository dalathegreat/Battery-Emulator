#ifndef NISSAN_LEAF_BATTERY_H
#define NISSAN_LEAF_BATTERY_H

#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4040  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 2600
#define MAX_CELL_DEVIATION_MV 500
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

uint16_t Temp_fromRAW_to_F(uint16_t temperature);
bool is_message_corrupt(CAN_frame rx_frame);
void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
