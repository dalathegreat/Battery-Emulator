#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"

extern ACAN2517FD canfd;

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 8064  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 4320
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2950  //Battery is put into emergency stop if one cell goes below this value
#define MAXCHARGEPOWERALLOWED 10000
#define MAXDISCHARGEPOWERALLOWED 10000

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
