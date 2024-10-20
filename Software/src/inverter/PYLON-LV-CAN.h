#ifndef PYLON_LV_CAN_H
#define PYLON_LV_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

#define MANUFACTURER_NAME "BatEmuLV"
#define PACK_NUMBER 0x01
// 80 means after reaching 80% of a nominal value a warning is produced (e.g. 80% of max current)
#define WARNINGS_PERCENT 80

void send_system_data();
void send_setup_info();
void transmit_can(CAN_frame* tx_frame, int interface);

#endif
