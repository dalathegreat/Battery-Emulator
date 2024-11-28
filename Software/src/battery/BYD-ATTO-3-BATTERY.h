#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../include.h"

#define USE_ESTIMATED_SOC  // If enabled, SOC is estimated from pack voltage. Useful for locked packs. \
                           // Uncomment this only if you know your BMS is unlocked and able to send SOC%
#define MAXPOWER_CHARGE_W 10000
#define MAXPOWER_DISCHARGE_W 10000

/* Do not modify the rows below */
#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4410  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 3800
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 3800  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2800  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
