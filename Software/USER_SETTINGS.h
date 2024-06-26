#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__
#include <stdint.h>

/* This file contains all the battery/inverter protocol settings Battery-Emulator software */
/* To switch between batteries/inverters, uncomment a line to enable, comment out to disable. */
/* There are also some options for battery limits and extra functionality */
/* To edit battery specific limits, see also the USER_SETTINGS.cpp file*/

/* Select battery used */
//#define BMW_I3_BATTERY
//#define BYD_ATTO_3_BATTERY
//#define CHADEMO_BATTERY	//NOTE: inherently enables CONTACTOR_CONTROL below
//#define IMIEV_CZERO_ION_BATTERY
//#define KIA_HYUNDAI_64_BATTERY
//#define KIA_E_GMP_BATTERY
//#define MG_5_BATTERY
//#define NISSAN_LEAF_BATTERY
//#define PYLON_BATTERY
//#define RENAULT_KANGOO_BATTERY
//#define RENAULT_ZOE_GEN1_BATTERY
//#define RENAULT_ZOE_GEN2_BATTERY
//#define SANTA_FE_PHEV_BATTERY
//#define TESLA_MODEL_3_BATTERY
//#define VOLVO_SPA_BATTERY
//#define TEST_FAKE_BATTERY

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
//#define BYD_CAN          //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
//#define BYD_MODBUS  //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define LUNA2000_MODBUS  //Enable this line to emulate a "Luna2000 battery" over Modbus RTU
//#define PYLON_CAN        //Enable this line to emulate a "Pylontech battery" over CAN bus
//#define SMA_CAN          //Enable this line to emulate a "BYD Battery-Box H 8.9kWh, 7 mod" over CAN bus
//#define SMA_TRIPOWER_CAN //Enable this line to emulate a "SMA Home Storage battery" over CAN bus
//#define SOFAR_CAN        //Enable this line to emulate a "Sofar Energy Storage Inverter High Voltage BMS General Protocol (Extended Frame)" over CAN bus
//#define SOLAX_CAN        //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus

/* Select hardware used for Battery-Emulator */
#define HW_LILYGO
//#define HW_STARK

/* Other options */
//#define DEBUG_VIA_USB  //Enable this line to have the USB port output serial diagnostic data while program runs (WARNING, raises CPU load, do not use for production)
//#define DEBUG_CANFD_DATA    //Enable this line to have the USB port output CAN-FD data while program runs (WARNING, raises CPU load, do not use for production)
//#define INTERLOCK_REQUIRED  //Nissan LEAF specific setting, if enabled requires both high voltage conenctors to be seated before starting
//#define CONTACTOR_CONTROL     //Enable this line to have pins 25,32,33 handle automatic precharge/contactor+/contactor- closing sequence
//#define PWM_CONTACTOR_CONTROL //Enable this line to use PWM logic for contactors, which lower power consumption and heat generation
//#define DUAL_CAN              //Enable this line to activate an isolated secondary CAN Bus using add-on MCP2515 controller (Needed for FoxESS inverters)
//#define CAN_FD  //Enable this line to activate an isolated secondary CAN-FD bus using add-on MCP2517FD controller (Needed for some batteries)
//#define SERIAL_LINK_RECEIVER  //Enable this line to receive battery data over RS485 pins from another Lilygo (This LilyGo interfaces with inverter)
//#define SERIAL_LINK_TRANSMITTER  //Enable this line to send battery data over RS485 pins to another Lilygo (This LilyGo interfaces with battery)
#define WEBSERVER  //Enable this line to enable WiFi, and to run the webserver. See USER_SETTINGS.cpp for the Wifi settings.
//#define LOAD_SAVED_SETTINGS_ON_BOOT  //Enable this line to read settings stored via the webserver on boot (overrides any battery settings set in USER_SETTINGS.cpp)
//#define FUNCTION_TIME_MEASUREMENT  // Enable this to record execution times and present them in the web UI (WARNING, raises CPU load, do not use for production)
//#define ISA_SHUNT  //Enable this line to build support for ISA IVT shunts

/* MQTT options */
// #define MQTT  // Enable this line to enable MQTT
#define MQTT_SERVER "192.168.xxx.yyy"
#define MQTT_PORT 1883

/* Event options*/
#define DUMMY_EVENT_ENABLED false  //Enable this line to have a dummy event that gets logged to test the interface

/* Select charger used (Optional) */
//#define CHEVYVOLT_CHARGER //Enable this line to control a Chevrolet Volt charger connected to battery - for example, when generator charging or using an inverter without a charging function.
//#define NISSANLEAF_CHARGER //Enable this line to control a Nissan LEAF PDM connected to battery - for example, when generator charging

/* Battery settings */

// Predefined total energy capacity of the battery in Watt-hours
#define BATTERY_WH_MAX 30000
// Increases battery life. If true will rescale SOC between the configured min/max-percentage
#define BATTERY_USE_SCALED_SOC true
// 8000 = 80.0% , Max percentage the battery will charge to (Inverter gets 100% when reached)
#define BATTERY_MAXPERCENTAGE 8000
// 2000 = 20.0% , Min percentage the battery will discharge to (Inverter gets 0% when reached)
#define BATTERY_MINPERCENTAGE 2000
// 300 = 30.0A , BYD CAN specific setting, Max charge in Amp (Some inverters needs to be limited)
#define BATTERY_MAX_CHARGE_AMP 300
// 300 = 30.0A , BYD CAN specific setting, Max discharge in Amp (Some inverters needs to be limited)
#define BATTERY_MAX_DISCHARGE_AMP 300

extern volatile uint8_t AccessPointEnabled;
extern const uint8_t wifi_channel;

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

#endif
