#ifndef CMFA_EV_BATTERY_H
#define CMFA_EV_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 3040  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 2150
#define MAX_CELL_DEVIATION_MV 100
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

// OBD2 PID polls
/*
#define PID_POLL_UNKNOWN1 0x9001 //122 in log
#define PID_POLL_UNKNOWNX 0x9002 //5531 (Possible SOC candidate)
*/
#define PID_POLL_SOH_AVERAGE 0x9003
#define PID_POLL_AVERAGE_VOLTAGE_OF_CELLS 0x9006     //Guaranteed pack voltage
#define PID_POLL_HIGHEST_CELL_VOLTAGE 0x9007         //Best guess, not confirmed
#define PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE 0x9008  // Cell number with highest voltage
#define PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE 0x900A   // Cell number with the lowest voltage
//#define PID_POLL_UNKNOWNX 0x900D
#define PID_POLL_12V_BATTERY 0x9011  //Confirmed 12v lead acid battery
/*
#define PID_POLL_UNKNOWNX 0x9012 //749 in log
#define PID_POLL_UNKNOWN3 0x9013 //736 in log
#define PID_POLL_UNKNOWN4 0x9014 //752 in log
#define PID_POLL_UNKNOWN4 0x901B // Multi frame message, lots of values ranging between 0x30 - 0x52
#define PID_POLL_UNKNOWNX 0x912F // Multi frame message, empty
#define PID_POLL_UNKNOWNX 0x9129
#define PID_POLL_UNKNOWNX 0x9131
#define PID_POLL_UNKNOWNX 0x9132
#define PID_POLL_UNKNOWNX 0x9133
#define PID_POLL_UNKNOWNX 0x9134
#define PID_POLL_UNKNOWNX 0x9135
#define PID_POLL_UNKNOWNX 0x9136
#define PID_POLL_UNKNOWNX 0x9137
#define PID_POLL_UNKNOWNX 0x9138
#define PID_POLL_UNKNOWNX 0x9139
#define PID_POLL_UNKNOWNX 0x913A
#define PID_POLL_UNKNOWNX 0x913B
#define PID_POLL_UNKNOWNX 0x913C
#define PID_POLL_UNKNOWN5 0x912F
#define PID_POLL_UNKNOWNX 0x91B7
#define PID_POLL_SOH_AVAILABLE_POWER_CALCULATION 0x91BC // 0-100%
#define PID_POLL_SOH_GENERATED_POWER_CALCULATION 0x91BD // 0-100%
#define PID_POLL_UNKNOWNX 0x91C1
#define PID_POLL_UNKNOWNX 0x91CD
#define PID_POLL_UNKNOWNX 0x91CF
#define PID_POLL_UNKNOWNX 0x91F6
#define PID_POLL_UNKNOWNX 0x91F7
#define PID_POLL_UNKNOWNX 0x920F
#define PID_POLL_UNKNOWNx 0x9242
*/
#define PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING 0x9243
#define PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING 0x9245
#define PID_POLL_CUMULATIVE_ENERGY_IN_REGEN 0x9247
/*
#define PID_POLL_UNKNOWNx 0x9256
#define PID_POLL_UNKNOWNx 0x9261
#define PID_POLL_UNKNOWN7 0x9284
#define PID_POLL_UNKNOWNx 0xF012
#define PID_POLL_UNKNOWNx 0xF1A0
#define PID_POLL_UNKNOWNx 0xF182
#define PID_POLL_UNKNOWNx 0xF187
#define PID_POLL_UNKNOWNx 0xF188
#define PID_POLL_UNKNOWNx 0xF18A
#define PID_POLL_UNKNOWNx 0xF18C
#define PID_POLL_UNKNOWNx 0xF191
#define PID_POLL_UNKNOWNx 0xF194
#define PID_POLL_UNKNOWNx 0xF195
*/

void setup_battery(void);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

#endif
