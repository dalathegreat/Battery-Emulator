#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../include.h"

#define USE_ESTIMATED_SOC  // If enabled, SOC is estimated from pack voltage. Useful for locked packs. \
                           // Comment out this only if you know your BMS is unlocked and able to send SOC%
#define MAXPOWER_CHARGE_W 10000
#define MAXPOWER_DISCHARGE_W 10000

//Uncomment and configure this line, if you want to filter out a broken temperature sensor (1-10)
//Make sure you understand risks associated with disabling. Values can be read via "More Battery info"
//#define SKIP_TEMPERATURE_SENSOR_NUMBER 1

/* Do not modify the rows below */
#define BATTERY_SELECTED
#define CELLCOUNT_EXTENDED 126
#define CELLCOUNT_STANDARD 104
#define MAX_PACK_VOLTAGE_EXTENDED_DV 4410  //Extended range
#define MIN_PACK_VOLTAGE_EXTENDED_DV 3800  //Extended range
#define MAX_PACK_VOLTAGE_STANDARD_DV 3640  //Standard range
#define MIN_PACK_VOLTAGE_STANDARD_DV 3136  //Standard range
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 3800  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2800  //Battery is put into emergency stop if one cell goes below this value

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
