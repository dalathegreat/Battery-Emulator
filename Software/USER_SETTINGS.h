#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__
#include <WiFi.h>
#include <stdint.h>

/* This file contains all the battery/inverter protocol settings Battery-Emulator software */
/* To switch between batteries/inverters, uncomment a line to enable, comment out to disable. */
/* There are also some options for battery limits and extra functionality */
/* To edit battery specific limits, see also the USER_SETTINGS.cpp file*/

/* Select battery used */
//#define BMW_I3_BATTERY
//#define BMW_IX_BATTERY
//#define BMW_PHEV_BATTERY
//#define BOLT_AMPERA_BATTERY
//#define BYD_ATTO_3_BATTERY
//#define FOXESS_BATTERY
//#define CELLPOWER_BMS
//#define CHADEMO_BATTERY	//NOTE: inherently enables CONTACTOR_CONTROL below
#define CMFA_EV_BATTERY
//#define IMIEV_CZERO_ION_BATTERY
//#define JAGUAR_IPACE_BATTERY
//#define KIA_E_GMP_BATTERY
//#define KIA_HYUNDAI_64_BATTERY
//#define KIA_HYUNDAI_HYBRID_BATTERY
//#define MEB_BATTERY
//#define MG_5_BATTERY
//#define NISSAN_LEAF_BATTERY
//#define ORION_BMS
//#define PYLON_BATTERY
//#define DALY_BMS
//#define RJXZS_BMS
//#define RANGE_ROVER_PHEV_BATTERY
//#define RENAULT_KANGOO_BATTERY
//#define RENAULT_TWIZY_BATTERY
//#define RENAULT_ZOE_GEN1_BATTERY
//#define RENAULT_ZOE_GEN2_BATTERY
//#define SONO_BATTERY
//#define SANTA_FE_PHEV_BATTERY
//#define SIMPBMS_BATTERY
//#define STELLANTIS_ECMP_BATTERY
//#define TESLA_MODEL_3Y_BATTERY
//#define TESLA_MODEL_SX_BATTERY
//#define VOLVO_SPA_BATTERY
//#define VOLVO_SPA_HYBRID_BATTERY
//#define TEST_FAKE_BATTERY
//#define DOUBLE_BATTERY  //Enable this line if you use two identical batteries at the same time (requires CAN_ADDON setup)

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
//#define AFORE_CAN        //Enable this line to emulate an "Afore battery" over CAN bus
//#define BYD_CAN  //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
//#define BYD_KOSTAL_RS485 //Enable this line to emulate a "BYD 11kWh HVM battery" over Kostal RS485
//#define BYD_MODBUS       //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define FERROAMP_CAN     //Enable this line to emulate a "Pylon 4x96V Force H2" over CAN Bus
//#define FOXESS_CAN       //Enable this line to emulate a "HV2600/ECS4100 battery" over CAN bus
//#define GROWATT_HV_CAN   //Enable this line to emulate a "Growatt High Voltage v1.10 battery" over CAN bus
//#define GROWATT_LV_CAN   //Enable this line to emulate a "48V Growatt Low Voltage battery" over CAN bus
//#define PYLON_LV_CAN     //Enable this line to emulate a "48V Pylontech battery" over CAN bus
//#define PYLON_CAN        //Enable this line to emulate a "High Voltage Pylontech battery" over CAN bus
//#define SCHNEIDER_CAN    //Enable this line to emulate a "Schneider Version 2: SE BMS" over CAN bus
//#define SMA_BYD_H_CAN    //Enable this line to emulate a "BYD Battery-Box H 8.9kWh, 7 mod" (SMA compatible) over CAN bus
//#define SMA_BYD_HVS_CAN  //Enable this line to emulate a "BYD Battery-Box HVS 10.2KW battery" (SMA compatible) over CAN bus
//#define SMA_LV_CAN       //Enable this line to emulate a "SMA Sunny Island 48V battery" over CAN bus
//#define SMA_TRIPOWER_CAN //Enable this line to emulate a "SMA Home Storage battery" over CAN bus
//#define SOFAR_CAN        //Enable this line to emulate a "Sofar Energy Storage Inverter High Voltage BMS General Protocol (Extended Frame)" over CAN bus
//#define SOLAX_CAN        //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus
//#define SUNGROW_CAN      //Enable this line to emulate a "Sungrow SBR064" over CAN bus

/* Select hardware used for Battery-Emulator */
#define HW_LILYGO
//#define HW_STARK
//#define HW_3LB
//#define HW_DEVKIT

/* Contactor settings. If you have a battery that does not activate contactors via CAN, configure this section */
#define PRECHARGE_TIME_MS 500  //Precharge time in milliseconds. Modify to suit your inverter (See wiki for more info)
//#define CONTACTOR_CONTROL     //Enable this line to have the emulator handle automatic precharge/contactor+/contactor- closing sequence (See wiki for pins)
//#define CONTACTOR_CONTROL_DOUBLE_BATTERY //Enable this line to have the emulator hardware control secondary set of contactors for double battery setups (See wiki for pins)
//#define PWM_CONTACTOR_CONTROL //Enable this line to use PWM for CONTACTOR_CONTROL, which lowers power consumption and heat generation. CONTACTOR_CONTROL must be enabled.
//#define NC_CONTACTORS         //Enable this line to control normally closed contactors. CONTACTOR_CONTROL must be enabled for this option. Extremely rare setting!
//#define PERIODIC_BMS_RESET    //Enable to have the emulator powercycle the connected battery every 24hours via GPIO. Useful for some batteries like Nissan LEAF
//#define REMOTE_BMS_RESET      //Enable to allow the emulator to remotely trigger a powercycle of the battery via MQTT. Useful for some batteries like Nissan LEAF
// PERIODIC_BMS_RESET_AT Uses NTP server, internet required. In 24 Hour format WITHOUT leading 0. e.g 0230 should be 230. Time Zone is set in USER_SETTINGS.cpp
//#define PERIODIC_BMS_RESET_AT 525

/* Shunt/Contactor settings (Optional) */
//#define BMW_SBOX  // SBOX relay control & battery current/voltage measurement

/* Select charger used (Optional) */
//#define CHEVYVOLT_CHARGER  //Enable this line to control a Chevrolet Volt charger connected to battery - for example, when generator charging or using an inverter without a charging function.
//#define NISSANLEAF_CHARGER  //Enable this line to control a Nissan LEAF PDM connected to battery - for example, when generator charging

/* Automatic Precharge settings (Optional) If you have a battery that expects an external voltage applied before opening contactors (within the battery), configure this section */
//#define PRECHARGE_CONTROL      //Enable this line to control a modified HIA4V1 (see wiki) by PWM on the PRECHARGE_PIN.

/* Other options */
//#define EQUIPMENT_STOP_BUTTON    // Enable this to allow an equipment stop button connected to the Battery-Emulator to disengage the battery
//#define LFP_CHEMISTRY          //Tesla specific setting, enable this line to startup in LFP mode
//#define INTERLOCK_REQUIRED     //Nissan LEAF specific setting, if enabled requires both high voltage conenctors to be seated before starting
//#define LOG_TO_SD              //Enable this line to log diagnostic data to SD card (WARNING, raises CPU load, do not use for production)
//#define LOG_CAN_TO_SD          //Enable this line to log incoming/outgoing CAN & CAN-FD messages to SD card (WARNING, raises CPU load, do not use for production)
//#define DEBUG_VIA_USB          //Enable this line to have the USB port output serial diagnostic data while program runs (WARNING, raises CPU load, do not use for production)
//#define DEBUG_VIA_WEB          //Enable this line to log diagnostic data while program runs, which can be viewed via webpage (WARNING, slightly raises CPU load, do not use for production)
//#define DEBUG_CAN_DATA  //Enable this line to print incoming/outgoing CAN & CAN-FD messages to USB serial (WARNING, raises CPU load, do not use for production)

/* CAN options */
//#define CAN_ADDON              //Enable this line to activate an isolated secondary CAN Bus using add-on MCP2515 chip (Needed for some inverters / double battery)
#define CRYSTAL_FREQUENCY_MHZ 8  //CAN_ADDON option, what is your MCP2515 add-on boards crystal frequency?
//#define CANFD_ADDON           //Enable this line to activate an isolated secondary CAN-FD bus using add-on MCP2518FD chip / Native CANFD on Stark board
#define CANFD_ADDON_CRYSTAL_FREQUENCY_MHZ \
  ACAN2517FDSettings::OSC_40MHz  //CANFD_ADDON option, what is your MCP2518 add-on boards crystal frequency?
//#define USE_CANFD_INTERFACE_AS_CLASSIC_CAN // Enable this line if you intend to use the CANFD as normal CAN

/* Connectivity options */
#define WIFI
//#define WIFICONFIG  //Enable this line to set a static IP address / gateway /subnet mask for the device. see USER_SETTINGS.cpp for the settings
#define WEBSERVER  //Enable this line to enable WiFi, and to run the webserver. See USER_SETTINGS.cpp for the Wifi settings.
#define WIFIAP  //When enabled, the emulator will broadcast its own access point Wifi. Can be used at the same time as a normal Wifi connection to a router.
#define MDNSRESPONDER  //Enable this line to enable MDNS, allows battery monitor te be found by .local address. Requires WEBSERVER to be enabled.
#define LOAD_SAVED_SETTINGS_ON_BOOT  // Enable this line to read settings stored via the webserver on boot (overrides Wifi credentials set here)
//#define FUNCTION_TIME_MEASUREMENT  // Enable this to record execution times and present them in the web UI (WARNING, raises CPU load, do not use for production)

/* MQTT options */
// #define MQTT     // Enable this line to enable MQTT
#define MQTT_QOS 0                  // MQTT Quality of Service (0, 1, or 2)
#define MQTT_PUBLISH_CELL_VOLTAGES  // Enable this line to publish cell voltages to MQTT
#define MQTT_TIMEOUT 2000           // MQTT timeout in milliseconds
#define MQTT_MANUAL_TOPIC_OBJECT_NAME
// Enable MQTT_MANUAL_TOPIC_OBJECT_NAME to use custom MQTT topic, object ID prefix, and device name.
// WARNING: If this is not defined, the previous default naming format 'battery-emulator_esp32-XXXXXX' (based on hardware ID) will be used.
// This naming convention was in place until version 7.5.0. Users should check the version from which they are updating, as this change
// may break compatibility with previous versions of MQTT naming. Please refer to USER_SETTINGS.cpp for configuration options.

/* Home Assistant options */
#define HA_AUTODISCOVERY  // Enable this line to send Home Assistant autodiscovery messages. If not enabled manual configuration of Home Assitant is required

/* Battery settings */
// Predefined total energy capacity of the battery in Watt-hours
#define BATTERY_WH_MAX 30000
// Increases battery life. If true will rescale SOC between the configured min/max-percentage
#define BATTERY_USE_SCALED_SOC true
// 8000 = 80.0% , Max percentage the battery will charge to (Inverter gets 100% when reached)
#define BATTERY_MAXPERCENTAGE 8000
// 2000 = 20.0% , Min percentage the battery will discharge to (Inverter gets 0% when reached)
#define BATTERY_MINPERCENTAGE 2000
// 500 = 50.0 °C , Max temperature (Will produce a battery overheat event if above)
#define BATTERY_MAXTEMPERATURE 500
// -250 = -25.0 °C , Min temperature (Will produce a battery frozen event if below)
#define BATTERY_MINTEMPERATURE -250
// 150 = 15.0 °C , Max difference between min and max temperature (Will produce a battery temperature deviation event if greater)
#define BATTERY_MAX_TEMPERATURE_DEVIATION 150
// 300 = 30.0A , Max charge in Amp (Some inverters needs to be limited)
#define BATTERY_MAX_CHARGE_AMP 300
// 300 = 30.0A , Max discharge in Amp (Some inverters needs to be limited)
#define BATTERY_MAX_DISCHARGE_AMP 300
// Enable this to manually set voltage limits on how much battery can be discharged/charged. Normally not used.
#define BATTERY_USE_VOLTAGE_LIMITS false
// 5000 = 500.0V , Target charge voltage (Value can be tuned on the fly via webserver). Not used unless BATTERY_USE_VOLTAGE_LIMITS = true
#define BATTERY_MAX_CHARGE_VOLTAGE 5000
// 3000 = 300.0V, Target discharge voltage (Value can be tuned on the fly via webserver). Not used unless BATTERY_USE_VOLTAGE_LIMITS = true
#define BATTERY_MAX_DISCHARGE_VOLTAGE 3000

/* LED settings. Optional customization for how the blinking pattern on the LED should behave.
* CLASSIC   - Slow up/down ramp. If CLASSIC, then a ramp up and ramp down will finish in LED_PERIOD_MS milliseconds
* FLOW      - Ramp up/down depending on flow of energy
* HEARTBEAT - Heartbeat-like LED pattern that reacts to the system state with color and BPM
*/
#define LED_MODE CLASSIC
#define LED_PERIOD_MS 3000

/* Do not change any code below this line */
/* Only change battery specific settings above and in "USER_SETTINGS.cpp" */
typedef enum { CAN_NATIVE = 0, CANFD_NATIVE = 1, CAN_ADDON_MCP2515 = 2, CANFD_ADDON_MCP2518 = 3 } CAN_Interface;
typedef struct {
  CAN_Interface battery;
  CAN_Interface inverter;
  CAN_Interface battery_double;
  CAN_Interface charger;
  CAN_Interface shunt;
} CAN_Configuration;
extern volatile CAN_Configuration can_config;
extern volatile uint8_t AccessPointEnabled;
extern const uint8_t wifi_channel;
extern volatile float CHARGER_SET_HV;
extern volatile float CHARGER_MAX_HV;
extern volatile float CHARGER_MIN_HV;
extern volatile float CHARGER_MAX_POWER;
extern volatile float CHARGER_MAX_A;
extern volatile float CHARGER_END_A;

extern volatile unsigned long long bmsResetTimeOffset;

#ifdef EQUIPMENT_STOP_BUTTON
typedef enum { LATCHING_SWITCH = 0, MOMENTARY_SWITCH = 1 } STOP_BUTTON_BEHAVIOR;
extern volatile STOP_BUTTON_BEHAVIOR equipment_stop_behavior;
#endif

#ifdef WIFICONFIG
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
#endif

#if defined(DEBUG_VIA_USB) || defined(DEBUG_VIA_WEB) || defined(LOG_TO_SD)
#define DEBUG_LOG
#endif

#if defined(MEB_BATTERY)
#define PRECHARGE_CONTROL
#endif

#endif  // __USER_SETTINGS_H__
