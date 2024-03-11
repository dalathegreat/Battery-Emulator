#ifndef SUMMARY_A
#define SUMMARY_A

#include <Arduino.h>
#include <stdint.h>
#include "../config.h"  // Needed for defines
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/AsyncJson.h"

extern const char* version_number;                         // The current software version, shown on webserver
extern uint32_t system_capacity_Wh;                        //Wh,  0-150000Wh
extern uint32_t system_remaining_capacity_Wh;              //Wh,  0-150000Wh
extern int16_t system_battery_current_dA;                  //A+1, -1000 - 1000
extern int16_t system_temperature_min_dC;                  //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;                  //C+1, -50.0 - 50.0
extern int16_t system_active_power_W;                      //W, -32000 to 32000
extern uint16_t system_max_design_voltage_dV;              //V+1,  0-500.0 (0-5000)
extern uint16_t system_min_design_voltage_dV;              //V+1,  0-500.0 (0-5000)
extern uint16_t system_scaled_SOC_pptt;                    //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;                      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_SOH_pptt;                           //SOH%, 0-100.00 (0-10000)
extern uint16_t system_battery_voltage_dV;                 //V+1,  0-500.0 (0-5000)
extern uint16_t system_max_discharge_power_W;              //W,    0-65000
extern uint16_t system_max_charge_power_W;                 //W,    0-65000
extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000 , Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;                     //Total number of cell voltages, set by each battery
extern uint8_t system_bms_status;                          //Enum 0-5
extern uint8_t LEDcolor;                                   //Enum, 0-10
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
 * @brief Pepares and returns an AsyncJsonResponse configured with the summary json data
 *
 * @param[in] void
 *
 * @return AsyncJsonResponse
 */
AsyncJsonResponse* summaryAPI_GET();

/**
 * @brief Returns a description of the user defined battery protocol
 *
 * @param[in] void
 *
 * @return String: Protocol Description
 */
String batteryProtocolDescription();

/**
 * @brief Returns a description of the user defined charger protocol
 *
 * @param[in] void
 *
 * @return String: Protocol Description
 */
String chargerProtocolDescription();

/**
 * @brief Returns a description of the user defined inverter protocol
 *
 * @param[in] void
 *
 * @return String: Protocol Description
 */
String inverterProtocolDescription();

/**
 * @brief Returns the current LED colour (which itself is based on the current state of the system)
 *
 * @param[in] void
 *
 * @return String: Colour
 */
String getLEDColour();

/**
 * @brief Returns a description of the current state of the wireless connection
 *
 * @param[in] void
 *
 * @return String; WiFi connection description
 */
String wirelessStatusDescription(wl_status_t status);

/**
 * @brief Scales power values
 *
 * @param[in] float or uint16_t
 *
 * @return string: values
 */
template <typename S>
String scalePowerValue(S value, String unit, int precision);
#endif
