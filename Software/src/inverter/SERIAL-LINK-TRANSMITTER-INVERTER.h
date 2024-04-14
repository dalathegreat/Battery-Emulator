#ifndef SERIAL_LINK_TRANSMITTER_INVERTER_H
#define SERIAL_LINK_TRANSMITTER_INVERTER_H

#include <Arduino.h>
#include "../include.h"
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

#define INVERTER_SELECTED

// These parameters need to be mapped for the inverter
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;     //V+1,  0-1000.0 (0-10000)
extern int16_t system_battery_current_dA;      //A+1, -1000 - 1000
extern uint32_t system_max_discharge_power_W;  //W,    0-200000
extern uint32_t system_max_charge_power_W;     //W,    0-200000
extern uint8_t system_bms_status;              //Enum 0-5
extern int16_t system_temperature_min_dC;      //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;      //C+1, -50.0 - 50.0
extern uint16_t system_cell_max_voltage_mV;    //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;    //mV, 0-5000, Stores the minimum cell millivolt value
extern bool batteryAllowsContactorClosing;     //Bool, true/false
extern bool system_LFP_Chemistry;              //Bool, true/false

// parameters received from receiver
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void manageSerialLinkTransmitter();

#endif
