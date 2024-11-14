#ifndef SERIAL_LINK_TRANSMITTER_INVERTER_H
#define SERIAL_LINK_TRANSMITTER_INVERTER_H

#define MODBUS_INVERTER_SELECTED

#include <Arduino.h>
#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

void manageSerialLinkTransmitter();
void setup_inverter(void);

#endif
