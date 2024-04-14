#ifndef TESLA_MODEL_3_BATTERY_H
#define TESLA_MODEL_3_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define BATTERY_SELECTED

//#define LFP_CHEMISTRY // Enable this line to startup in LFP mode

#define RAMPDOWN_SOC 900             // 90.0 SOC% to start ramping down from max charge power towards 0 at 100.00%
#define FLOAT_MAX_POWER_W 200        // W, what power to allow for top balancing battery
#define FLOAT_START_MV 20            // mV, how many mV under overvoltage to start float charging
#define MAXCHARGEPOWERALLOWED 15000  // 15000W we use a define since the value supplied by Tesla is always 0
#define MAXDISCHARGEPOWERALLOWED \
  60000                             // 60000W we need to cap this value to max 60kW, most inverters overflow otherwise
#define MAX_PACK_VOLTAGE_NCMA 4030  // V+1, if pack voltage goes over this, charge stops
#define MIN_PACK_VOLTAGE_NCMA 3100  // V+1, if pack voltage goes below this, discharge stops
#define MAX_PACK_VOLTAGE_LFP 3880   // V+1, if pack voltage goes over this, charge stops
#define MIN_PACK_VOLTAGE_LFP 2968   // V+1, if pack voltage goes below this, discharge stops

// These parameters need to be mapped for the inverter
extern uint16_t system_scaled_SOC_pptt;                    //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;                      //SOC%, 0-100.00 (0-10000)
extern uint32_t system_max_discharge_power_W;              //W,    0-200000
extern uint32_t system_max_charge_power_W;                 //W,    0-200000
extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;                     //Total number of cell voltages, set by each battery
extern uint8_t system_bms_status;                          //Enum 0-5
extern bool batteryAllowsContactorClosing;                 //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;                //Bool, 1=true, 0=false
extern bool system_LFP_Chemistry;                          //Bool, 1=true, 0=false

void printFaultCodesIfActive();
void printDebugIfActive(uint8_t symbol, const char* message);
void print_int_with_units(char* header, int value, char* units);
void print_SOC(char* header, int SOC);
void setup_battery(void);

#endif
