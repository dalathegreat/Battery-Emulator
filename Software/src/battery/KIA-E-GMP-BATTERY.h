#ifndef KIA_E_GMP_BATTERY_H
#define KIA_E_GMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"

extern ACAN2517FD canfd;

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 150
#define MAXCHARGEPOWERALLOWED 10000
#define MAXDISCHARGEPOWERALLOWED 10000

void setup_battery(void);
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
