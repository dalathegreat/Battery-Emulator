// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#define BATTERY_SELECTED

#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

void manageSerialLinkReceiver();
void update_values_serial_link();
void setup_battery(void);

#endif
