#ifndef _ESPNOW_H_
#define _ESPNOW_H_

#include "../../system_settings.h"
#include "../utils/types.h"

void init_espnow();
void update_espnow();

struct BATTERY_INFO_TYPE {
  /** uint32_t */
  /** Total energy capacity in Watt-hours 
   * Automatically updates depending on battery integration OR from settings page
  */
  uint32_t total_capacity_Wh = 30000;
  uint32_t reported_total_capacity_Wh = 30000;

  /** uint16_t */
  /** The maximum intended packvoltage, in deciVolt. 4900 = 490.0 V */
  uint16_t max_design_voltage_dV = 5000;
  /** The minimum intended packvoltage, in deciVolt. 3300 = 330.0 V */
  uint16_t min_design_voltage_dV = 2500;
  /** The maximum cellvoltage before shutting down, in milliVolt. 4300 = 4.300 V */
  uint16_t max_cell_voltage_mV = 4300;
  /** The minimum cellvoltage before shutting down, in milliVolt. 2700 = 2.700 V */
  uint16_t min_cell_voltage_mV = 2700;
  /** The maxumum allowed deviation between cells, in milliVolt. 500 = 0.500 V */
  uint16_t max_cell_voltage_deviation_mV = 500;

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

  /** Other */
  /** Chemistry of the pack. Autodetect, or force specific chemistry */
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
};  // 24 bytes

struct BATTERY_STATUS_TYPE {
  /** uint32_t */
  /** Remaining energy capacity in Watt-hours */
  uint32_t remaining_capacity_Wh = 0;
  /** The remaining capacity reported to the inverter based on min percentage setting, in Watt-hours 
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint32_t reported_remaining_capacity_Wh;
  /** Maximum allowed battery discharge power in Watts. Set by battery */
  uint32_t max_discharge_power_W = 0;
  /** Maximum allowed battery charge power in Watts. Set by battery */
  uint32_t max_charge_power_W = 0;
  /* Some early integrations do not support reading allowed charge power from battery
  On these integrations we need to have the user specify what limits the battery can take */
  /** Overriden allowed battery discharge power in Watts. Set by user */
  uint32_t override_discharge_power_W = 0;
  /** Overriden allowed battery charge power in Watts. Set by user */
  uint32_t override_charge_power_W = 0;

  /** int32_t */
  /** Instantaneous battery power in Watts. Calculated based on voltage_dV and current_dA */
  /* Positive value = Battery Charging */
  /* Negative value = Battery Discharging */
  int32_t active_power_W = 0;
  int32_t total_charged_battery_Wh = 0;
  int32_t total_discharged_battery_Wh = 0;

  /** uint16_t */
  /** Maximum allowed battery discharge current in dA. Calculated based on allowed W and Voltage */
  uint16_t max_discharge_current_dA = 0;
  /** Maximum allowed battery charge current in dA. Calculated based on allowed W and Voltage  */
  uint16_t max_charge_current_dA = 0;
  /** State of health in integer-percent x 100. 9900 = 99.00% */
  uint16_t soh_pptt = 9900;
  /** Instantaneous battery voltage in deciVolts. 3700 = 370.0 V */
  uint16_t voltage_dV = 3700;
  /** Maximum cell voltage currently measured in the pack, in mV */
  uint16_t cell_max_voltage_mV = 3700;
  /** Minimum cell voltage currently measured in the pack, in mV */
  uint16_t cell_min_voltage_mV = 3700;
  /** The "real" SOC reported from the battery, in integer-percent x 100. 9550 = 95.50% */
  uint16_t real_soc;
  /** The SOC reported to the inverter, in integer-percent x 100. 9550 = 95.50%.
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint16_t reported_soc;
  /** A counter that increases incase a CAN CRC read error occurs */
  uint16_t CAN_error_counter;

  /** int16_t */
  /** Maximum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_max_dC;
  /** Minimum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_min_dC;
  /** Instantaneous battery current in deciAmpere. 95 = 9.5 A */
  int16_t current_dA;
  /** Instantaneous battery current in deciAmpere. Sum of all batteries in the system 95 = 9.5 A */
  int16_t reported_current_dA;

  /** uint8_t */
  /** A counter set each time a new message comes from battery.
   * This value then gets decremented every second. Incase we reach 0
   * we report the battery as missing entirely on the CAN bus.
   */
  uint8_t CAN_battery_still_alive = CAN_STILL_ALIVE;
  /** The current system status, which for now still has the name bms_status */
  bms_status_enum bms_status = ACTIVE;
  /** The current battery status, which for now has the name real_bms_status */
  real_bms_status_enum real_bms_status = BMS_DISCONNECTED;
  /** LED mode, customizable by user */
  led_mode_enum led_mode = CLASSIC;
  /** Balancing status */
  balancing_status_enum balancing_status = BALANCING_STATUS_UNKNOWN;
};  // 80 bytes

struct BATTERY_BALANCING_STATUS_TYPE {
  /** All balancing resistors status inside the pack, either on(1) or off(0).
   * Use with battery.info.number_of_cells to get valid data.
   * Not available for all battery manufacturers.
   */
  bool cell_balancing_status[MAX_AMOUNT_CELLS];

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

};  // 193 bytes

struct BATTERY_CELL_STATUS_TYPE {

  /** All cell voltages currently measured in the pack, in mV. 212 * 20 = 4240 mV
   * Use with battery.info.number_of_cells to get valid data.
   */
  uint8_t cell_voltages_mV[MAX_AMOUNT_CELLS];

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;
};  // 193 bytes

enum espnow_message_enum { BAT_INFO = 1, BAT_STATUS = 2, BAT_BALANCE = 3, BAT_CELL_STATUS = 4 };

struct ESPNOW_BATTERY_MESSAGE {
  uint16_t emulator_id;
  uint8_t battery_id;
  uint8_t esp_message_type;
  uint8_t esp_message[];
} __packed;

#endif  // _ESPNOW_H_
