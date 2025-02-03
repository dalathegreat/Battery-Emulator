#ifndef DALY_BMS_H
#define DALY_BMS_H

/* Tweak these according to your battery build */
#define CELL_COUNT 14
#define DESIGN_PACK_VOLTAGE_DB 528  //528 = 52.8V
#define MAX_PACK_VOLTAGE_DV 588     //588 = 58.8V
#define MIN_PACK_VOLTAGE_DV 518     //518 = 51.8V
#define MAX_CELL_VOLTAGE_MV 4250    //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700    //Battery is put into emergency stop if one cell goes below this value
#define MAX_DISCHARGE_POWER_ALLOWED_W 1800
#define MAX_CHARGE_POWER_ALLOWED_W 1800

/* Do not modify any rows below*/
#define BATTERY_SELECTED
#define RS485_BATTERY_SELECTED

#endif
