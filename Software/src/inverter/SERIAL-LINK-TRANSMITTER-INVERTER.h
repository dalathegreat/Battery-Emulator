#ifndef SERIAL_LINK_TRANSMITTER_INVERTER_H
#define SERIAL_LINK_TRANSMITTER_INVERTER_H

#include <Arduino.h>
#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

#define INVERTER_SELECTED

// These parameters need to be mapped for the inverter
extern bool batteryAllowsContactorClosing;  //Bool, true/false
extern bool system_LFP_Chemistry;           //Bool, true/false

// parameters received from receiver
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void manageSerialLinkTransmitter();

#endif
