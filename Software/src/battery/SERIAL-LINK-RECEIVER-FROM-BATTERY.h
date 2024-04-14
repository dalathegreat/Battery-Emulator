// SERIAL-LINK-RECEIVER-FROM-BATTERY.h

#ifndef SERIAL_LINK_RECEIVER_FROM_BATTERY_H
#define SERIAL_LINK_RECEIVER_FROM_BATTERY_H

#define BATTERY_SELECTED

#include <Arduino.h>
#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

//  https://github.com/mackelec/SerialDataLink

// These parameters need to be mapped on the battery side
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint32_t system_max_discharge_power_W;  //W,    0-200000
extern uint32_t system_max_charge_power_W;     //W,    0-200000
extern uint8_t system_bms_status;              //Enum 0-5
extern uint16_t system_cell_max_voltage_mV;    //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;    //mV, 0-5000, Stores the minimum cell millivolt value
extern bool system_LFP_Chemistry;              //Set to true or false depending on cell chemistry
extern bool batteryAllowsContactorClosing;     //Bool, 1=true, 0=false

// Parameters to send to the transmitter
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void manageSerialLinkReceiver();
void update_values_serial_link();
void setup_battery(void);

#endif
