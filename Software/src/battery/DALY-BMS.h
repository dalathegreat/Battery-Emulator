#ifndef DALY_BMS_H
#define DALY_BMS_H

/* Tweak these according to your battery build */
#define CELL_COUNT 14
#define MAX_PACK_VOLTAGE_DV 588   //588 = 58.8V
#define MIN_PACK_VOLTAGE_DV 518   //518 = 51.8V
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value
#define POWER_PER_PERCENT 50  // below 20% and above 80% limit power to 50W * SOC (i.e. 150W at 3%, 500W at 10%, ...)

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define RS485_BATTERY_SELECTED
#define RS485_BAUDRATE 9600

#endif
