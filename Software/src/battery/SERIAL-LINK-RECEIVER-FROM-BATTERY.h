// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#define BATTERY_SELECTED

#include <Arduino.h>
#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

//  https://github.com/mackelec/SerialDataLink

// These parameters need to be mapped on the battery side
extern bool batteryAllowsContactorClosing;  //Bool, 1=true, 0=false

// Parameters to send to the transmitter
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void manageSerialLinkReceiver();
void update_values_serial_link();
void setup_battery(void);

#endif
