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
extern bool batteryAllowsContactorClosing;   //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false
extern bool system_LFP_Chemistry;            //Bool, 1=true, 0=false

void printFaultCodesIfActive();
void printDebugIfActive(uint8_t symbol, const char* message);
void print_int_with_units(char* header, int value, char* units);
void print_SOC(char* header, int SOC);
void setup_battery(void);

#endif
