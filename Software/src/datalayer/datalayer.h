#ifndef _DATALAYER_H_
#define _DATALAYER_H_

#include "../devboard/utils/soc_scaling.h"
#include "../include.h"

typedef struct {
  /** float - FUTURE */
  float max_design_voltage_V;
  float min_design_voltage_V;

  /** uint32_t */
  uint32_t total_capacity_Wh;

  /** uint16_t */
  uint16_t number_of_cells;

  /** Other */
  battery_chemistry_enum chemistry;
} DATALAYER_BATTERY_DATA_TYPE;

typedef struct {
  /** float - FUTURE */
  // float temperature_max_C;
  // float temperature_min_C;
  // float current_A;
  // float voltage_V;
  // float soh_pct = 99.0f;

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
  uint16_t soh_pptt = 9900;  //SOH%, 0-100.00 (0-10000)
  uint16_t voltage_dV;
  uint16_t cell_max_voltage_mV;
  uint16_t cell_min_voltage_mV;
  uint16_t cell_voltages_mV[MAX_AMOUNT_CELLS];

  /** Other */
  // bms_status_enum bms_status; // Not ready yet
  ScaledSoc soc;
} DATALAYER_BATTERY_STATUS_TYPE;

typedef struct {
  DATALAYER_BATTERY_DATA_TYPE info;
  DATALAYER_BATTERY_STATUS_TYPE status;
} DATALAYER_BATTERY_TYPE;

typedef struct {
#ifdef FUNCTION_TIME_MEASUREMENT
  int64_t main_task_max_us = 0;
  int64_t main_task_10s_max_us = 0;
  int64_t time_wifi_us = 0;
  int64_t time_mqtt_us = 0;
  int64_t time_comm_us = 0;
  int64_t time_10ms_us = 0;
  int64_t time_5s_us = 0;
  int64_t time_cantx_us = 0;
  int64_t time_events_us = 0;
#endif
} DATALAYER_SYSTEM_STATUS_TYPE;

typedef struct {
  DATALAYER_SYSTEM_STATUS_TYPE status;
} DATALAYER_SYSTEM_TYPE;

typedef struct {
  bool batteryAllowsContactorClosing = false;
  bool inverterAllowsContactorClosing = true;
} DATALAYER_SETTINGS_TYPE;

class DataLayer {
 public:
  DATALAYER_BATTERY_TYPE battery;
  DATALAYER_SYSTEM_TYPE system;
  DATALAYER_SETTINGS_TYPE settings;
};

extern DataLayer datalayer;

#endif
