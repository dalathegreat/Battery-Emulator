#ifndef _DATALAYER_H_
#define _DATALAYER_H_

#include "../include.h"

typedef struct {
  /** uint32_t */
  /** Total energy capacity in Watt-hours */
  uint32_t total_capacity_Wh = BATTERY_WH_MAX;

  /** uint16_t */
  /** The maximum intended packvoltage, in deciVolt. 4900 = 490.0 V */
  uint16_t max_design_voltage_dV = 5000;
  /** The minimum intended packvoltage, in deciVolt. 3300 = 330.0 V */
  uint16_t min_design_voltage_dV = 2500;
  /** The maximum cellvoltage before shutting down, in milliVolt. 4300 = 4.250 V */
  uint16_t max_cell_voltage_mV = 4300;
  /** The minimum cellvoltage before shutting down, in milliVolt. 2700 = 2.700 V */
  uint16_t min_cell_voltage_mV = 2700;
  /** The maxumum allowed deviation between cells, in milliVolt. 500 = 0.500 V */
  uint16_t max_cell_voltage_deviation_mV = 500;

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

  /** Other */
  /** Chemistry of the pack. NCA, NMC or LFP (so far) */
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
} DATALAYER_BATTERY_INFO_TYPE;

typedef struct {
  /** int32_t */
  /** Instantaneous battery power in Watts. Calculated based on voltage_dV and current_dA */
  /* Positive value = Battery Charging */
  /* Negative value = Battery Discharging */
  int32_t active_power_W;

  /** uint32_t */
  /** Remaining energy capacity in Watt-hours */
  uint32_t remaining_capacity_Wh;
  /** The remaining capacity reported to the inverter based on min percentage setting, in Watt-hours 
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint32_t reported_remaining_capacity_Wh;

  /** Maximum allowed battery discharge power in Watts. Set by battery */
  uint32_t max_discharge_power_W = 0;
  /** Maximum allowed battery charge power in Watts. Set by battery */
  uint32_t max_charge_power_W = 0;
  /** Maximum allowed battery discharge current in dA. Calculated based on allowed W and Voltage */
  uint16_t max_discharge_current_dA = 0;
  /** Maximum allowed battery charge current in dA. Calculated based on allowed W and Voltage  */
  uint16_t max_charge_current_dA = 0;

  /** int16_t */
  /** Maximum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_max_dC;
  /** Minimum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_min_dC;
  /** Instantaneous battery current in deciAmpere. 95 = 9.5 A */
  int16_t current_dA;

  /** uint16_t */
  /** State of health in integer-percent x 100. 9900 = 99.00% */
  uint16_t soh_pptt = 9900;
  /** Instantaneous battery voltage in deciVolts. 3700 = 370.0 V */
  uint16_t voltage_dV = 3700;
  /** Maximum cell voltage currently measured in the pack, in mV */
  uint16_t cell_max_voltage_mV = 3700;
  /** Minimum cell voltage currently measured in the pack, in mV */
  uint16_t cell_min_voltage_mV = 3700;
  /** All cell voltages currently measured in the pack, in mV.
   * Use with battery.info.number_of_cells to get valid data.
   */
  uint16_t cell_voltages_mV[MAX_AMOUNT_CELLS];
  /** The "real" SOC reported from the battery, in integer-percent x 100. 9550 = 95.50% */
  uint16_t real_soc;
  /** The SOC reported to the inverter, in integer-percent x 100. 9550 = 95.50%.
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint16_t reported_soc;
  /** A counter that increases incase a CAN CRC read error occurs */
  uint16_t CAN_error_counter;
  /** uint8_t */
  /** A counter set each time a new message comes from battery.
   * This value then gets decremented every second. Incase we reach 0
   * we report the battery as missing entirely on the CAN bus.
   */
  uint8_t CAN_battery_still_alive = CAN_STILL_ALIVE;

  /** Other */
  /** The current system status, which for now still has the name bms_status */
  bms_status_enum bms_status = ACTIVE;

  /** The current battery status, which for now has the name real_bms_status */
  real_bms_status_enum real_bms_status = BMS_DISCONNECTED;
} DATALAYER_BATTERY_STATUS_TYPE;

typedef struct {
  /** SOC scaling setting. Set to true to use SOC scaling */
  bool soc_scaling_active = BATTERY_USE_SCALED_SOC;
  /** Minimum percentage setting. Set this value to the lowest real SOC
   * you want the inverter to be able to use. At this real SOC, the inverter
   * will "see" 0% */
  uint16_t min_percentage = BATTERY_MINPERCENTAGE;
  /** Maximum percentage setting. Set this value to the highest real SOC
   * you want the inverter to be able to use. At this real SOC, the inverter
   * will "see" 100% */
  uint16_t max_percentage = BATTERY_MAXPERCENTAGE;

  /** The user specified maximum allowed charge rate, in deciAmpere. 300 = 30.0 A */
  uint16_t max_user_set_charge_dA = BATTERY_MAX_CHARGE_AMP;
  /** The user specified maximum allowed discharge rate, in deciAmpere. 300 = 30.0 A */
  uint16_t max_user_set_discharge_dA = BATTERY_MAX_DISCHARGE_AMP;

  /** User specified discharge/charge voltages in use. Set to true to use user specified values */
  /** Some inverters like to see a specific target voltage for charge/discharge. Use these values to override automatic voltage limits*/
  bool user_set_voltage_limits_active = BATTERY_USE_VOLTAGE_LIMITS;
  /** The user specified maximum allowed charge voltage, in deciVolt. 4000 = 400.0 V */
  uint16_t max_user_set_charge_voltage_dV = BATTERY_MAX_CHARGE_VOLTAGE;
  /** The user specified maximum allowed discharge voltage, in deciVolt. 3000 = 300.0 V */
  uint16_t max_user_set_discharge_voltage_dV = BATTERY_MAX_DISCHARGE_VOLTAGE;

  /** Tesla specific settings that are edited on the fly when manually forcing a balance charge for LFP chemistry */
  /* Bool for specifying if user has requested manual function */
  bool user_requests_balancing = false;
  bool user_requests_isolation_clear = false;
  /* Forced balancing max time & start timestamp */
  uint32_t balancing_time_ms = 3600000;  //1h default, (60min*60sec*1000ms)
  uint32_t balancing_start_time_ms = 0;  //For keeping track when balancing started
  /* Max cell voltage during forced balancing */
  uint16_t balancing_max_cell_voltage_mV = 3650;
  /* Max cell deviation allowed during forced balancing */
  uint16_t balancing_max_deviation_cell_voltage_mV = 400;
  /* Float max power during forced balancing */
  uint16_t balancing_float_power_W = 1000;
  /* Maximum voltage for entire battery pack during forced balancing */
  uint16_t balancing_max_pack_voltage_dV = 3940;

} DATALAYER_BATTERY_SETTINGS_TYPE;

typedef struct {
  DATALAYER_BATTERY_INFO_TYPE info;
  DATALAYER_BATTERY_STATUS_TYPE status;
  DATALAYER_BATTERY_SETTINGS_TYPE settings;
} DATALAYER_BATTERY_TYPE;

typedef struct {
  /** Charger setpoint voltage */
  float charger_setpoint_HV_VDC = 0;
  /** Charger setpoint current */
  float charger_setpoint_HV_IDC = 0;
  /** Charger setpoint current at end of charge **/
  float charger_setpoint_HV_IDC_END = 0;
  /** Measured current from charger */
  float charger_stat_HVcur = 0;
  /** Measured HV from charger */
  float charger_stat_HVvol = 0;
  /** Measured AC current from charger **/
  float charger_stat_ACcur = 0;
  /** Measured AC voltage from charger **/
  float charger_stat_ACvol = 0;
  /** Measured LV current from charger **/
  float charger_stat_LVcur = 0;
  /** Measured LV voltage from charger **/
  float charger_stat_LVvol = 0;
  /** True if charger is enabled */
  bool charger_HV_enabled = false;
  /** True if the 12V DC/DC output is enabled */
  bool charger_aux12V_enabled = false;
  /** uint8_t */
  /** A counter set each time a new message comes from charger.
   * This value then gets decremented every second. Incase we reach 0
   * we report the battery as missing entirely on the CAN bus.
   */
  uint8_t CAN_charger_still_alive = CAN_STILL_ALIVE;
} DATALAYER_CHARGER_TYPE;

typedef struct {
  /** measured voltage in deciVolts. 4200 = 420.0 V */
  uint16_t measured_voltage_dV = 0;
  /** measured amperage in deciAmperes. 300 = 30.0 A */
  uint16_t measured_amperage_dA = 0;
  /** measured battery voltage in mV (S-BOX) **/
  uint32_t measured_voltage_mV = 0;
  /** measured output voltage in mV (eg. S-BOX) **/
  uint32_t measured_outvoltage_mV = 0;
  /** measured amperage in mA (eg. S-BOX) **/
  int32_t measured_amperage_mA = 0;
  /** Average current from last 1s **/
  int32_t measured_avg1S_amperage_mA = 0;
  /** True if contactors are precharging state */
  bool precharging = false;
  /** True if the contactor controlled by battery-emulator is closed */
  bool contactors_engaged = false;
  /** True if shunt communication ok **/
  bool available = false;
} DATALAYER_SHUNT_TYPE;

typedef struct {
  /** array with type of battery used, for displaying on webserver */
  char battery_protocol[64] = {0};
  /** array with type of inverter used, for displaying on webserver */
  char inverter_protocol[64] = {0};
  /** array with type of battery used, for displaying on webserver */
  char shunt_protocol[64] = {0};
  /** array with incoming CAN messages, for displaying on webserver */
  char logged_can_messages[15000] = {0};
  size_t logged_can_messages_offset = 0;
  /** bool, determines if CAN messages should be logged for webserver */
  bool can_logging_active = false;

} DATALAYER_SYSTEM_INFO_TYPE;

typedef struct {
#ifdef FUNCTION_TIME_MEASUREMENT
  /** Core task measurement variable */
  int64_t core_task_max_us = 0;
  /** Core task measurement variable, reset each 10 seconds */
  int64_t core_task_10s_max_us = 0;
  /** MQTT sub-task measurement variable, reset each 10 seconds */
  int64_t mqtt_task_10s_max_us = 0;
  /** Wifi sub-task measurement variable, reset each 10 seconds */
  int64_t wifi_task_10s_max_us = 0;
  /** loop() task measurement variable, reset each 10 seconds */
  int64_t loop_task_10s_max_us = 0;

  /** OTA handling function measurement variable */
  int64_t time_ota_us = 0;
  /** CAN RX or serial link function measurement variable */
  int64_t time_comm_us = 0;
  /** 10 ms function measurement variable */
  int64_t time_10ms_us = 0;
  /** Value update function measurement variable */
  int64_t time_values_us = 0;
  /** CAN TX function measurement variable */
  int64_t time_cantx_us = 0;

  /** Function measurement snapshot variable.
   * This will show the performance of OTA handling when the total time reached a new worst case
   */
  int64_t time_snap_ota_us = 0;
  /** Function measurement snapshot variable.
   * This will show the performance of CAN RX or serial link when the total time reached a new worst case
   */
  int64_t time_snap_comm_us = 0;
  /** Function measurement snapshot variable.
   * This will show the performance of the 10 ms functionality of the core task when the total time reached a new worst case
   */
  int64_t time_snap_10ms_us = 0;
  /** Function measurement snapshot variable.
   * This will show the performance of the values functionality of the core task when the total time reached a new worst case
   */
  int64_t time_snap_values_us = 0;
  /** Function measurement snapshot variable.
   * This will show the performance of CAN TX when the total time reached a new worst case
   */
  int64_t time_snap_cantx_us = 0;
#endif
  /** uint8_t */
  /** A counter set each time a new message comes from inverter.
   * This value then gets decremented every second. Incase we reach 0
   * we report the inverter as missing entirely on the CAN bus.
   */
  uint8_t CAN_inverter_still_alive = CAN_STILL_ALIVE;
  /** True if the battery allows for the contactors to close */
  bool battery_allows_contactor_closing = false;
  /** True if the second battery allows for the contactors to close */
  bool battery2_allows_contactor_closing = false;
  /** True if the inverter allows for the contactors to close */
  bool inverter_allows_contactor_closing = true;
#ifdef CONTACTOR_CONTROL
  /** True if the contactor controlled by battery-emulator is closed */
  bool contactors_engaged = false;
  /** True if the contactor controlled by battery-emulator is closed. Determined by check_interconnect_available(); if voltage is OK */
  bool contactors_battery2_engaged = false;
#endif
  /** True if the BMS is being reset, by cutting power towards it */
  bool BMS_reset_in_progress = false;
#ifdef PRECHARGE_CONTROL
  /** State of automatic precharge sequence */
  PrechargeState precharge_status = AUTO_PRECHARGE_IDLE;
#endif
} DATALAYER_SYSTEM_STATUS_TYPE;

typedef struct {
  bool equipment_stop_active = false;
#ifdef PRECHARGE_CONTROL
  bool start_precharging = false;
#endif
} DATALAYER_SYSTEM_SETTINGS_TYPE;

typedef struct {
  DATALAYER_SYSTEM_INFO_TYPE info;
  DATALAYER_SYSTEM_STATUS_TYPE status;
  DATALAYER_SYSTEM_SETTINGS_TYPE settings;
} DATALAYER_SYSTEM_TYPE;

class DataLayer {
 public:
  DATALAYER_BATTERY_TYPE battery;
  DATALAYER_BATTERY_TYPE battery2;
  DATALAYER_SHUNT_TYPE shunt;
  DATALAYER_CHARGER_TYPE charger;
  DATALAYER_SYSTEM_TYPE system;
};

extern DataLayer datalayer;

#endif
