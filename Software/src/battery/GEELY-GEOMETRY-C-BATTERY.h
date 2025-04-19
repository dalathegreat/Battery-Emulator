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

#define POLL_SOC 0x4B35
#define POLL_CC2_VOLTAGE 0x4BCF
#define POLL_CELL_MAX_VOLTAGE_NUMBER 0x4B1E
#define POLL_CELL_MIN_VOLTAGE_NUMBER 0x4B20
#define POLL_AMOUNT_CELLS 0x4B07
#define POLL_SPECIFICIAL_VOLTAGE 0x4B05
#define POLL_UNKNOWN_1 0x4BDA
#define POLL_RAW_SOC_MAX 0x4BC3
#define POLL_RAW_SOC_MIN 0x4BC4
#define POLL_UNKNOWN_4 0xDF00
#define POLL_UNKNOWN_5 0x4B3D
#define POLL_UNKNOWN_6 0x4B3E
#define POLL_UNKNOWN_7 0x4B24
#define POLL_UNKNOWN_8 0x4B26

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
