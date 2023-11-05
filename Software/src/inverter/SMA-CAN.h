#ifndef SMA_CAN_H
#define SMA_CAN_H
#include <Arduino.h>
#include "../../ESP32CAN.h"
#include "../../USER_SETTINGS.h"

extern uint16_t SOC;						            //SOC%, 0-100.00 (0-10000)
extern uint16_t StateOfHealth; 				      //SOH%, 0-100.00 (0-10000)
extern uint16_t battery_voltage; 			      //V+1,  0-500.0 (0-5000)
extern uint16_t battery_current; 			        //A+1,  Goes thru convert2unsignedint16 function (5.0A = 50, -5.0A = 65485)
extern uint16_t capacity_Wh;				//Wh,   0-60000
extern uint16_t remaining_capacity_Wh;		//Wh,   0-60000
extern uint16_t max_target_discharge_power; //W,    0-60000
extern uint16_t max_target_charge_power; 	//W,    0-60000
extern uint16_t bms_status; 				//Enum, 0-5
extern uint16_t bms_char_dis_status; 		//Enum, 0-2
extern uint16_t stat_batt_power; 			//W,    Goes thru convert2unsignedint16 function (5W = 5, -5W = 65530)
extern uint16_t temperature_min; 			//C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max; 			//C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;           //mV,   0-4350
extern uint16_t cell_min_voltage;			//mV,   0-4350
extern uint16_t min_volt_sma_can;
extern uint16_t max_volt_sma_can;
extern uint8_t LEDcolor;                 	//Enum, 0-2
// Definitions for BMS status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

void update_values_can_sma();
void send_can_sma();
void receive_can_sma(CAN_frame_t rx_frame);

#endif