#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Preferences.h>
#include <WiFi.h>
#include "../../include.h"
#include "../../lib/ayushsharma82-ElegantOTA/src/ElegantOTA.h"
#ifdef MQTT
#include "../../lib/knolleary-pubsubclient/PubSubClient.h"
#endif
#include "../../lib/me-no-dev-AsyncTCP/src/AsyncTCP.h"
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#ifdef MQTT
#include "../mqtt/mqtt.h"
#endif

extern const char* version_number;                         // The current software version, shown on webserver
extern uint32_t system_capacity_Wh;                        //Wh,  0-500000Wh
extern uint32_t system_remaining_capacity_Wh;              //Wh,  0-500000Wh
extern int16_t system_battery_current_dA;                  //A+1, -1000 - 1000
extern int16_t system_temperature_min_dC;                  //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;                  //C+1, -50.0 - 50.0
extern int32_t system_active_power_W;                      //W, -200000 to 200000
extern uint16_t system_max_design_voltage_dV;              //V+1,  0-1000.0 (0-10000)
extern uint16_t system_min_design_voltage_dV;              //V+1,  0-1000.0 (0-10000)
extern uint16_t system_scaled_SOC_pptt;                    //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;                      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_SOH_pptt;                           //SOH%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;                 //V+1,  0-1000.0 (0-10000)
extern uint32_t system_max_discharge_power_W;              //W,    0-200000
extern uint32_t system_max_charge_power_W;                 //W,    0-200000
extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000 , Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;                     //Total number of cell voltages, set by each battery
extern uint8_t system_bms_status;                          //Enum 0-5
extern bool batteryAllowsContactorClosing;                 //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;                //Bool, 1=true, 0=false

extern const char* ssid;
extern const char* password;
extern const uint8_t wifi_channel;
extern const char* ssidAP;
extern const char* passwordAP;

// Common charger parameters
extern float charger_stat_HVcur;
extern float charger_stat_HVvol;
extern float charger_stat_ACcur;
extern float charger_stat_ACvol;
extern float charger_stat_LVcur;
extern float charger_stat_LVvol;

//LEAF charger
extern uint16_t OBC_Charge_Power;

/**
 * @brief Initialization function for the webserver.
 *
 * @param[in] void
 *
 * @return void
 */
void init_webserver();

/**
 * @brief Monitoring loop for WiFi. Will attempt to reconnect to access point if the connection goes down.
 *
 * @param[in] void
 * 
 * @return void
 */
void wifi_monitor();

/**
 * @brief Initialization function that creates a WiFi Access Point.
 *
 * @param[in] void
 * 
 * @return void
 */
void init_WiFi_AP();

/**
 * @brief Initialization function that connects to an existing network.
 *
 * @param[in] ssid WiFi network name
 * @param[in] password WiFi network password
 * 
 * @return void
 */
void init_WiFi_STA(const char* ssid, const char* password, const uint8_t channel);

// /**
//  * @brief Function to handle WiFi reconnection.
//  *
//  * @param[in] void
//  *
//  * @return void
//  */
// void handle_WiFi_reconnection(unsigned long currentMillis);

/**
 * @brief Initialization function for ElegantOTA.
 *
 * @param[in] void
 * 
 * @return void
 */
void init_ElegantOTA();

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String processor(const String& var);

/**
 * @brief Executes on OTA start 
 *
 * @param[in] void
 *
 * @return void
 */
void onOTAStart();

/**
 * @brief Executes on OTA progress 
 * 
 * @param[in] current Current bytes
 * @param[in] final Final bytes
 *
 * @return void
 */
void onOTAProgress(size_t current, size_t final);

/**
 * @brief Executes on OTA end 
 *
 * @param[in] void
 * 
 * @return bool success: success = true, failed = false
 */
void onOTAEnd(bool success);

/**
 * @brief Formats power values 
 *
 * @param[in] float or uint16_t 
 * 
 * @return string: values
 */
template <typename T>
String formatPowerValue(String label, T value, String unit, int precision);

extern void storeSettings();

#endif
