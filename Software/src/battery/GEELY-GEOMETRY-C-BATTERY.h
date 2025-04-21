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
#define POLL_UNKNOWN_1 0x4BDA  //245 on two batteries
#define POLL_RAW_SOC_MAX 0x4BC3
#define POLL_RAW_SOC_MIN 0x4BC4
#define POLL_UNKNOWN_4 0xDF00  //144 (the other battery 143)
#define POLL_CAPACITY_MODULE_MAX 0x4B3D
#define POLL_CAPACITY_MODULE_MIN 0x4B3E
#define POLL_UNKNOWN_7 0x4B24  //1 (the other battery 23)
#define POLL_UNKNOWN_8 0x4B26  //16 (the other battery 33)
#define POLL_MULTI_TEMPS 0x4B0F
#define POLL_MULTI_UNKNOWN_2 0x4B10
#define POLL_MULTI_UNKNOWN_3 0x4B53
#define POLL_MULTI_UNKNOWN_4 0x4B54
#define POLL_MULTI_UNKNOWN_5 0x4B6B
#define POLL_MULTI_UNKNOWN_6 0x4B6C

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
