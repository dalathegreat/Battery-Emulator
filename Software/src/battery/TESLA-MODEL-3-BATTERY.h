#ifndef TESLA_MODEL_3_BATTERY_H
#define TESLA_MODEL_3_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

#define MAXCHARGEPOWERALLOWED 15000  // 15000W we use a define since the value supplied by Tesla is always 0
#define MAXDISCHARGEPOWERALLOWED \
  60000  // 60000W we need to cap this value to max 60kW, most inverters overflow otherwise

// These parameters need to be mapped for the inverter
extern uint32_t system_capacity_Wh;            //Wh,  0-150000Wh
extern uint32_t system_remaining_capacity_Wh;  //Wh,  0-150000Wh
extern int16_t system_temperature_min_dC;      //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;      //C+1, -50.0 - 50.0
extern int16_t system_active_power_W;          //W, -32000 to 32000
extern int16_t system_battery_current_dA;      //A+1, -1000 - 1000
extern uint16_t system_battery_voltage_dV;     //V+1,  0-500.0 (0-5000)
extern uint16_t system_max_design_voltage_dV;  //V+1,  0-500.0 (0-5000)
extern uint16_t system_min_design_voltage_dV;  //V+1,  0-500.0 (0-5000)
extern uint16_t system_scaled_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;          //SOC%, 0-100.00 (0-10000)
extern uint16_t system_SOH_pptt;               //SOH%, 0-100.00 (0-10000)
extern uint16_t system_max_discharge_power_W;  //W,    0-65000
extern uint16_t system_max_charge_power_W;     //W,    0-65000
extern uint16_t system_cell_max_voltage_mV;    //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;    //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[192];   //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;         //Total number of cell voltages, set by each battery
extern uint8_t system_bms_status;              //Enum 0-5
extern bool batteryAllowsContactorClosing;     //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;    //Bool, 1=true, 0=false
extern bool system_LFP_Chemistry;              //Bool, 1=true, 0=false

void printFaultCodesIfActive();
void printDebugIfActive(uint8_t symbol, const char* message);
void print_int_with_units(char* header, int value, char* units);
void print_SOC(char* header, int SOC);
void setup_battery(void);

#endif
