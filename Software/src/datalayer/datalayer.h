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
  /** BYD CAN specific setting, max charge in deciAmpere. 300 = 30.0 A */
  uint16_t max_charge_amp_dA = BATTERY_MAX_CHARGE_AMP;
  /** BYD CAN specific setting, max discharge in deciAmpere. 300 = 30.0 A */
  uint16_t max_discharge_amp_dA = BATTERY_MAX_DISCHARGE_AMP;

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

  /** Other */
  /** Chemistry of the pack. NCA, NMC or LFP (so far) */
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
} DATALAYER_BATTERY_INFO_TYPE;

typedef struct {
  /** int32_t */
  /** Instantaneous battery power in Watts */
  /* Positive value = Battery Charging */
  /* Negative value = Battery Discharging */
  int32_t active_power_W;

  /** uint32_t */
  /** Remaining energy capacity in Watt-hours */
  uint32_t remaining_capacity_Wh;
  /** Maximum allowed battery discharge power in Watts */
  uint32_t max_discharge_power_W = 0;
  /** Maximum allowed battery charge power in Watts */
  uint32_t max_charge_power_W = 0;

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
   * This value then gets decremented each 5 seconds. Incase we reach 0
   * we report the battery as missing entirely on the CAN bus.
   */
  uint8_t CAN_battery_still_alive = CAN_STILL_ALIVE;

  /** Other */
  /** The current BMS status */
  bms_status_enum bms_status = ACTIVE;
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
} DATALAYER_BATTERY_SETTINGS_TYPE;

typedef struct {
  DATALAYER_BATTERY_INFO_TYPE info;
  DATALAYER_BATTERY_STATUS_TYPE status;
  DATALAYER_BATTERY_SETTINGS_TYPE settings;
} DATALAYER_BATTERY_TYPE;

typedef struct {
  /** measured voltage in deciVolts. 4200 = 420.0 V */
  uint16_t measured_voltage_dV = 0;
  /** measured amperage in deciAmperes. 300 = 30.0 A */
  uint16_t measured_amperage_dA = 0;
} DATALAYER_SHUNT_TYPE;

typedef struct {
  // TODO
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
  /** 5 s function measurement variable */
  int64_t time_5s_us = 0;
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
   * This will show the performance of the 5 s functionality of the core task when the total time reached a new worst case
   */
  int64_t time_snap_5s_us = 0;
  /** Function measurement snapshot variable.
   * This will show the performance of CAN TX when the total time reached a new worst case
   */
  int64_t time_snap_cantx_us = 0;
#endif
  /** True if the battery allows for the contactors to close */
  bool battery_allows_contactor_closing = false;
  /** True if the inverter allows for the contactors to close */
  bool inverter_allows_contactor_closing = true;
} DATALAYER_SYSTEM_STATUS_TYPE;

typedef struct {
} DATALAYER_SYSTEM_SETTINGS_TYPE;

typedef struct {
  DATALAYER_SYSTEM_INFO_TYPE info;
  DATALAYER_SYSTEM_STATUS_TYPE status;
  DATALAYER_SYSTEM_SETTINGS_TYPE settings;
} DATALAYER_SYSTEM_TYPE;

class DataLayer {
 public:
  DATALAYER_BATTERY_TYPE battery;
  DATALAYER_SHUNT_TYPE shunt;
  DATALAYER_SYSTEM_TYPE system;
};

extern DataLayer datalayer;

#endif
