#ifndef RJXZS_BMS_H
#define RJXZS_BMS_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef RJXZS_BMS
#define SELECTED_BATTERY_CLASS RjxzsBms
#endif

class RjxzsBms : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "RJXZS BMS, DIY battery";

 private:
  /* Tweak these according to your battery build */
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_DISCHARGE_POWER_ALLOWED_W = 5000;
  static const int MAX_CHARGE_POWER_ALLOWED_W = 5000;
  static const int MAX_CHARGE_POWER_WHEN_TOPBALANCING_W = 500;
  static const int RAMPDOWN_SOC =
      9000;  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%

  unsigned long previousMillis10s = 0;  // will store last time a 10s CAN Message was sent

  //Actual content messages
  CAN_frame RJXZS_1C = {.FD = false, .ext_ID = true, .DLC = 3, .ID = 0xF4, .data = {0x1C, 0x00, 0x02}};
  CAN_frame RJXZS_10 = {.FD = false, .ext_ID = true, .DLC = 3, .ID = 0xF4, .data = {0x10, 0x00, 0x02}};

  static const int FIVE_MINUTES = 60;

  uint8_t mux = 0;
  bool setup_completed = false;
  uint16_t total_voltage = 0;
  int16_t total_current = 0;
  uint16_t total_power = 0;
  uint16_t battery_usage_capacity = 0;
  uint16_t battery_capacity_percentage = 0;
  uint16_t charging_capacity = 0;
  uint16_t charging_recovery_voltage = 0;
  uint16_t discharging_recovery_voltage = 0;
  uint16_t remaining_capacity = 0;
  int16_t host_temperature = 0;
  uint16_t status_accounting = 0;
  uint16_t equalization_starting_voltage = 0;
  uint16_t discharge_protection_voltage = 0;
  uint16_t protective_current = 0;
  uint16_t battery_pack_capacity = 0;
  uint16_t number_of_battery_strings = 0;
  uint16_t charging_protection_voltage = 0;
  int16_t protection_temperature = 0;
  bool temperature_below_zero_mod1_4 = false;
  bool temperature_below_zero_mod5_8 = false;
  bool temperature_below_zero_mod9_12 = false;
  bool temperature_below_zero_mod13_16 = false;
  uint16_t module_1_temperature = 0;
  uint16_t module_2_temperature = 0;
  uint16_t module_3_temperature = 0;
  uint16_t module_4_temperature = 0;
  uint16_t module_5_temperature = 0;
  uint16_t module_6_temperature = 0;
  uint16_t module_7_temperature = 0;
  uint16_t module_8_temperature = 0;
  uint16_t module_9_temperature = 0;
  uint16_t module_10_temperature = 0;
  uint16_t module_11_temperature = 0;
  uint16_t module_12_temperature = 0;
  uint16_t module_13_temperature = 0;
  uint16_t module_14_temperature = 0;
  uint16_t module_15_temperature = 0;
  uint16_t module_16_temperature = 0;
  uint16_t low_voltage_power_outage_protection = 0;
  uint16_t low_voltage_power_outage_delayed = 0;
  uint16_t num_of_triggering_protection_cells = 0;
  uint16_t balanced_reference_voltage = 0;
  uint16_t minimum_cell_voltage = 0;
  uint16_t maximum_cell_voltage = 0;
  uint16_t cellvoltages[MAX_AMOUNT_CELLS];
  uint8_t populated_cellvoltages = 0;
  uint16_t accumulated_total_capacity_high = 0;
  uint16_t accumulated_total_capacity_low = 0;
  uint16_t pre_charge_delay_time = 0;
  uint16_t LCD_status = 0;
  uint16_t differential_pressure_setting_value = 0;
  uint16_t use_capacity_to_automatically_reset = 0;
  uint16_t low_temperature_protection_setting_value = 0;
  uint16_t protecting_historical_logs = 0;
  uint16_t hall_sensor_type = 0;
  uint16_t fan_start_setting_value = 0;
  uint16_t ptc_heating_start_setting_value = 0;
  uint16_t default_channel_state = 0;
  uint8_t timespent_without_soc = 0;
  bool charging_active = false;
  bool discharging_active = false;
};

/* Do not modify any rows below*/
#define NATIVECAN_250KBPS

#endif
