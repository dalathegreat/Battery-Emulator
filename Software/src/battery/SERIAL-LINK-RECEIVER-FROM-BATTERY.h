// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#define BATTERY_SELECTED
#ifndef MAX_CELL_DEVIATION_MV
#define MAX_CELL_DEVIATION_MV 9999
#endif

#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

void manageSerialLinkReceiver();
void update_values_serial_link();
void setup_battery(void);

#endif
