#ifndef _DATALAYER_H_
#define _DATALAYER_H_

#include "../include.h"

typedef struct {
  /** uint32_t */
  uint32_t total_capacity_Wh = BATTERY_WH_MAX;

  /** uint16_t */
  uint16_t max_design_voltage_dV;
  uint16_t min_design_voltage_dV;
  uint16_t max_charge_amp_dA = BATTERY_MAX_CHARGE_AMP;
  uint16_t max_discharge_amp_dA = BATTERY_MAX_DISCHARGE_AMP;

  /** uint8_t */
  uint8_t number_of_cells;

  /** Other */
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
} DATALAYER_BATTERY_INFO_TYPE;

typedef struct {
  /** int32_t */
  int32_t active_power_W;

  /** uint32_t */
  uint32_t remaining_capacity_Wh;
  uint32_t max_discharge_power_W = 0;
  uint32_t max_charge_power_W = 0;

  /** int16_t */
  int16_t temperature_max_dC;
  int16_t temperature_min_dC;
  int16_t current_dA;

  /** uint16_t */
  uint16_t soh_pptt = 9900;
  uint16_t voltage_dV;
  uint16_t cell_max_voltage_mV = 3700;
  uint16_t cell_min_voltage_mV = 3700;
  uint16_t cell_voltages_mV[MAX_AMOUNT_CELLS];
  uint16_t real_soc;
  uint16_t scaled_soc;
  uint16_t reported_soc;

  /** Other */
  bms_status_enum bms_status = ACTIVE;
} DATALAYER_BATTERY_STATUS_TYPE;

typedef struct {
  bool soc_scaling_active = BATTERY_USE_SCALED_SOC;
  uint16_t min_percentage = BATTERY_MINPERCENTAGE;
  uint16_t max_percentage = BATTERY_MAXPERCENTAGE;
} DATALAYER_BATTERY_SETTINGS_TYPE;

typedef struct {
  DATALAYER_BATTERY_INFO_TYPE info;
  DATALAYER_BATTERY_STATUS_TYPE status;
  DATALAYER_BATTERY_SETTINGS_TYPE settings;
} DATALAYER_BATTERY_TYPE;

typedef struct {
  // TODO
} DATALAYER_SYSTEM_INFO_TYPE;

typedef struct {
#ifdef FUNCTION_TIME_MEASUREMENT
  int64_t core_task_max_us = 0;
  int64_t core_task_10s_max_us = 0;
  int64_t mqtt_task_10s_max_us = 0;
  int64_t loop_task_10s_max_us = 0;
  int64_t time_wifi_us = 0;
  int64_t time_comm_us = 0;
  int64_t time_10ms_us = 0;
  int64_t time_5s_us = 0;
  int64_t time_cantx_us = 0;

  int64_t time_snap_wifi_us = 0;
  int64_t time_snap_comm_us = 0;
  int64_t time_snap_10ms_us = 0;
  int64_t time_snap_5s_us = 0;
  int64_t time_snap_cantx_us = 0;
#endif
  bool battery_allows_contactor_closing = false;
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
  DATALAYER_SYSTEM_TYPE system;
};

extern DataLayer datalayer;

#endif
