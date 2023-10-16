#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__

/* This file contains all the user configurable settings for the Battery-Emulator software */
/* To switch between batteries/inverters, uncomment a line to enable, comment out to disable. */
/* There are also some options for battery limits and extra functionality */

/* Select battery used */
#define BATTERY_TYPE_LEAF
//#define TESLA_MODEL_3_BATTERY
//#define RENAULT_ZOE_BATTERY
//#define BMW_I3_BATTERY
//#define IMIEV_ION_CZERO_BATTERY
//#define KIA_HYUNDAI_64_BATTERY
//#define CHADEMO

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
#define MODBUS_BYD     //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define CAN_BYD      //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
//#define SOLAX_CAN    //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus
//#define PYLON_CAN		 //Enable this line to emulate a "Pylontech battery" over CAN bus
//#define SMA_CAN		//Enable this line to emulate a "BYD Battery-Box H 8.9kWh, 7 mod" over CAN bus

/* Battery settings */
#define BATTERY_WH_MAX 30000 //Battery size in Wh (Maximum value for most inverters is 60000 [60kWh], you can use larger batteries but do set value over 60000!
#define MAXPERCENTAGE 800 //80.0% , Max percentage the battery will charge to (App will show 100% once this value is reached)
#define MINPERCENTAGE 200 //20.0% , Min percentage the battery will discharge to (App will show 0% once this value is reached)
//define INTERLOCK_REQUIRED //Nissan LEAF specific setting, if enabled requires both high voltage conenctors to be seated before starting

/* Other options */
#define DEBUG_VIA_USB           //Enable this line to have the USB port output serial diagnostic data while program runs
//#define CONTACTOR_CONTROL     //Enable this line to have pins 25,32,33 handle automatic precharge/contactor+/contactor- closing sequence
//#define PWM_CONTACTOR_CONTROL //Enable this line to use PWM logic for contactors, which lower power consumption and heat generation
//#define DUAL_CAN              //Enable this line to activate an isolated secondary CAN Bus using add-on MCP2515 controller (Needed for FoxESS inverters)

#endif