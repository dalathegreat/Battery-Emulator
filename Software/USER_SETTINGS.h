#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__
#include <stdint.h>

/* This file contains all the battery/inverter protocol settings Battery-Emulator software */
/* To switch between batteries/inverters, uncomment a line to enable, comment out to disable. */
/* There are also some options for battery limits and extra functionality */
/* To edit battery specific limits, see also the USER_SETTINGS.cpp file*/

/* Select battery used */
//#define BMW_I3_BATTERY
//#define CHADEMO_BATTERY
//#define IMIEV_CZERO_ION_BATTERY
//#define KIA_HYUNDAI_64_BATTERY
//#define NISSAN_LEAF_BATTERY
//#define RENAULT_KANGOO_BATTERY
//#define RENAULT_ZOE_BATTERY
//#define SANTA_FE_PHEV_BATTERY
//#define TESLA_MODEL_3_BATTERY
//#define TEST_FAKE_BATTERY

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
//#define BYD_CAN          //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
//#define BYD_MODBUS  //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define LUNA2000_MODBUS  //Enable this line to emulate a "Luna2000 battery" over Modbus RTU
//#define PYLON_CAN        //Enable this line to emulate a "Pylontech battery" over CAN bus
//#define SMA_CAN          //Enable this line to emulate a "BYD Battery-Box H 8.9kWh, 7 mod" over CAN bus
//#define SOFAR_CAN        //Enable this line to emulate a "Sofar Energy Storage Inverter High Voltage BMS General Protocol (Extended Frame)" over CAN bus
//#define SOLAX_CAN        //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus

/* Other options */
#define DEBUG_VIA_USB  //Enable this line to have the USB port output serial diagnostic data while program runs
//#define INTERLOCK_REQUIRED  //Nissan LEAF specific setting, if enabled requires both high voltage conenctors to be seated before starting
//#define CONTACTOR_CONTROL     //Enable this line to have pins 25,32,33 handle automatic precharge/contactor+/contactor- closing sequence
//#define PWM_CONTACTOR_CONTROL //Enable this line to use PWM logic for contactors, which lower power consumption and heat generation
//#define DUAL_CAN              //Enable this line to activate an isolated secondary CAN Bus using add-on MCP2515 controller (Needed for FoxESS inverters)
//#define SERIAL_LINK_RECEIVER  //Enable this line to receive battery data over RS485 pins from another Lilygo (This LilyGo interfaces with inverter)
//#define SERIAL_LINK_TRANSMITTER  //Enable this line to send battery data over RS485 pins to another Lilygo (This LilyGo interfaces with battery)
#define WEBSERVER  //Enable this line to enable WiFi, and to run the webserver. See USER_SETTINGS.cpp for the Wifi settings.
//#define LOAD_SAVED_SETTINGS_ON_BOOT  //Enable this line to read settings stored via the webserver on boot

/* MQTT options */
//#define MQTT  // Enable this line to enable MQTT
#define MQTT_SUBSCRIPTIONS \
  { "my/topic/abc", "my/other/topic" }
#define MQTT_SERVER "192.168.xxx.yyy"
#define MQTT_PORT 1883

/* Event options*/
#define EVENTLOGGING               //Enable this line to log events to the event log
#define DUMMY_EVENT_ENABLED false  //Enable this line to have a dummy event that gets logged to test the interface

/* Select charger used (Optional) */
//#define CHEVYVOLT_CHARGER //Enable this line to control a Chevrolet Volt charger connected to battery - for example, when generator charging or using an inverter without a charging function.
//#define NISSANLEAF_CHARGER //Enable this line to control a Nissan LEAF PDM connected to battery - for example, when generator charging

/* Battery limits: These are set in the USER_SETTINGS.cpp file, or later on via the Webserver */
extern volatile uint16_t BATTERY_WH_MAX;
extern volatile uint16_t MAXPERCENTAGE;
extern volatile uint16_t MINPERCENTAGE;
extern volatile uint16_t MAXCHARGEAMP;
extern volatile uint16_t MAXDISCHARGEAMP;
extern volatile uint8_t AccessPointEnabled;

/* Charger limits (Optional): Set in the USER_SETTINGS.cpp or later in the webserver */
extern volatile float charger_setpoint_HV_VDC;
extern volatile float charger_setpoint_HV_IDC;
extern volatile float charger_setpoint_HV_IDC_END;
extern volatile float CHARGER_SET_HV;
extern volatile float CHARGER_MAX_HV;
extern volatile float CHARGER_MIN_HV;
extern volatile float CHARGER_MAX_POWER;
extern volatile float CHARGER_MAX_A;
extern volatile float CHARGER_END_A;
extern bool charger_HV_enabled;
extern bool charger_aux12V_enabled;

extern const uint8_t wifi_channel;

#endif
