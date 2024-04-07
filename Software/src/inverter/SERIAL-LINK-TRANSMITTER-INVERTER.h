#ifndef SERIAL_LINK_TRANSMITTER_INVERTER_H
#define SERIAL_LINK_TRANSMITTER_INVERTER_H

#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/mackelec-SerialDataLink/SerialDataLink.h"

// These parameters need to be mapped for the inverter
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint16_t system_SOH_pptt;               //SOH%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;     //V+1,  0-1000.0 (0-10000)
extern int16_t system_battery_current_dA;      //A+1, -1000 - 1000
extern uint32_t system_capacity_Wh;            //Wh,  0-250000Wh
extern uint32_t system_remaining_capacity_Wh;  //Wh,  0-250000Wh
extern uint32_t system_max_discharge_power_W;  //W,    0-100000
extern uint32_t system_max_charge_power_W;     //W,    0-100000
extern uint8_t system_bms_status;              //Enum 0-5
extern int32_t system_active_power_W;          //W, -200000 to 200000
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
