#include "api_status_html.h"
#include <Arduino.h>
#include "../../../datalayer/datalayer.h"
#include "../../../datalayer/datalayer_extended.h"
#include "../../utils/led_handler.h"

// External declarations
extern const char* version_number;

// Helper function to add a field to the JSON string
void addJsonField(String& json, const String& key, const String& value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":\"";
  json += value;
  json += "\"";
  if (addComma)
    json += ",";
}

// Helper function to add a numeric field to the JSON string
template <typename T>
void addJsonNumber(String& json, const String& key, T value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += String(value);
  if (addComma)
    json += ",";
}

// Helper function to add a boolean field to the JSON string
void addJsonBool(String& json, const String& key, bool value, bool addComma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (addComma)
    json += ",";
}

String api_status_processor() {
  String json = "{";

  // System information
  json += "\"system\":{";
  addJsonField(json, "version", version_number);

  // Calculate uptime
  unsigned long uptime_seconds = millis() / 1000;
  unsigned long days = uptime_seconds / 86400;
  uptime_seconds %= 86400;
  unsigned long hours = uptime_seconds / 3600;
  uptime_seconds %= 3600;
  unsigned long minutes = uptime_seconds / 60;
  uptime_seconds %= 60;
  char uptime_buffer[32];
  sprintf(uptime_buffer, "%lud %luh %lum %lus", days, hours, minutes, uptime_seconds);
  addJsonField(json, "uptime", uptime_buffer);

  addJsonField(json, "inverter_protocol", datalayer.system.info.inverter_protocol);
  addJsonField(json, "inverter_brand", datalayer.system.info.inverter_brand);
  addJsonField(json, "battery_protocol", datalayer.system.info.battery_protocol);
  addJsonNumber(json, "temperature", datalayer.system.info.CPU_temperature);
  addJsonField(json, "emulator_status", String(get_emulator_pause_status().c_str()));
  addJsonBool(json, "equipment_stop", datalayer.system.settings.equipment_stop_active);

  String led_status;
  switch (led_get_color()) {
    case led_color::GREEN:
      led_status = "GREEN";
      break;
    case led_color::YELLOW:
      led_status = "YELLOW";
      break;
    case led_color::BLUE:
      led_status = "BLUE";
      break;
    case led_color::RED:
      led_status = "RED";
      break;
    default:
      led_status = "UNKNOWN";
      break;
  }
  addJsonField(json, "led_status", led_status, false);

  json += "},";

  // Battery information
  json += "\"battery\":{";
  addJsonNumber(json, "soc_real", static_cast<float>(datalayer.battery.status.real_soc) / 100.0);
  addJsonNumber(json, "soc_reported", static_cast<float>(datalayer.battery.status.reported_soc) / 100.0);
  addJsonBool(json, "soc_scaling_active", datalayer.battery.settings.soc_scaling_active);
  addJsonNumber(json, "soh", static_cast<float>(datalayer.battery.status.soh_pptt) / 100.0);
  addJsonNumber(json, "voltage", static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0);
  addJsonNumber(json, "current", static_cast<float>(datalayer.battery.status.current_dA) / 10.0);
  addJsonNumber(json, "power", datalayer.battery.status.active_power_W);
  addJsonNumber(json, "capacity_total_wh", datalayer.battery.info.total_capacity_Wh);
  addJsonNumber(json, "capacity_reported_wh", datalayer.battery.info.reported_total_capacity_Wh);
  addJsonNumber(json, "capacity_remaining_wh", datalayer.battery.status.remaining_capacity_Wh);
  addJsonNumber(json, "capacity_reported_remaining_wh", datalayer.battery.status.reported_remaining_capacity_Wh);

  // Power limits
  addJsonNumber(json, "max_discharge_power", datalayer.battery.status.max_discharge_power_W);
  addJsonNumber(json, "max_charge_power", datalayer.battery.status.max_charge_power_W);
  addJsonNumber(json, "max_discharge_current",
                static_cast<float>(datalayer.battery.status.max_discharge_current_dA) / 10.0);
  addJsonNumber(json, "max_charge_current", static_cast<float>(datalayer.battery.status.max_charge_current_dA) / 10.0);
  addJsonBool(json, "user_settings_limit_discharge", datalayer.battery.settings.user_settings_limit_discharge);
  addJsonBool(json, "user_settings_limit_charge", datalayer.battery.settings.user_settings_limit_charge);
  addJsonBool(json, "inverter_limits_discharge", datalayer.battery.settings.inverter_limits_discharge);
  addJsonBool(json, "inverter_limits_charge", datalayer.battery.settings.inverter_limits_charge);

  // Cell information
  addJsonNumber(json, "cell_min_voltage", datalayer.battery.status.cell_min_voltage_mV);
  addJsonNumber(json, "cell_max_voltage", datalayer.battery.status.cell_max_voltage_mV);
  addJsonNumber(json, "cell_delta",
                datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV);
  addJsonNumber(json, "cell_delta_limit", datalayer.battery.info.max_cell_voltage_deviation_mV);

  // Temperature
  addJsonNumber(json, "temp_min", static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0);
  addJsonNumber(json, "temp_max", static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0);

  // Status
  addJsonNumber(json, "bms_status", datalayer.battery.status.bms_status);

  String bms_status_text;
  switch (datalayer.battery.status.bms_status) {
    case ACTIVE:
      bms_status_text = "ACTIVE";
      break;
    case UPDATING:
      bms_status_text = "UPDATING";
      break;
    case FAULT:
      bms_status_text = "FAULT";
      break;
    case INACTIVE:
      bms_status_text = "INACTIVE";
      break;
    case STANDBY:
      bms_status_text = "STANDBY";
      break;
    default:
      bms_status_text = "UNKNOWN";
      break;
  }
  addJsonField(json, "bms_status_text", bms_status_text);

  // Contactor information
  addJsonBool(json, "contactor_status", datalayer.system.status.contactors_engaged);
  addJsonBool(json, "battery_allows_contactor_closing", datalayer.system.status.battery_allows_contactor_closing);
  addJsonBool(json, "inverter_allows_contactor_closing", datalayer.system.status.inverter_allows_contactor_closing,
              false);

  json += "}";

#ifdef DOUBLE_BATTERY
  // Second battery information
  json += ",\"battery2\":{";
  addJsonNumber(json, "soc_real", static_cast<float>(datalayer.battery2.status.real_soc) / 100.0);
  addJsonNumber(json, "soc_reported", static_cast<float>(datalayer.battery2.status.reported_soc) / 100.0);
  addJsonNumber(json, "soh", static_cast<float>(datalayer.battery2.status.soh_pptt) / 100.0);
  addJsonNumber(json, "voltage", static_cast<float>(datalayer.battery2.status.voltage_dV) / 10.0);
  addJsonNumber(json, "current", static_cast<float>(datalayer.battery2.status.current_dA) / 10.0);
  addJsonNumber(json, "power", datalayer.battery2.status.active_power_W);
  addJsonNumber(json, "capacity_total_wh", datalayer.battery2.info.total_capacity_Wh);
  addJsonNumber(json, "capacity_reported_wh", datalayer.battery2.info.reported_total_capacity_Wh);
  addJsonNumber(json, "capacity_remaining_wh", datalayer.battery2.status.remaining_capacity_Wh);
  addJsonNumber(json, "capacity_reported_remaining_wh", datalayer.battery2.status.reported_remaining_capacity_Wh);
  addJsonNumber(json, "max_discharge_power", datalayer.battery2.status.max_discharge_power_W);
  addJsonNumber(json, "max_charge_power", datalayer.battery2.status.max_charge_power_W);
  addJsonNumber(json, "cell_min_voltage", datalayer.battery2.status.cell_min_voltage_mV);
  addJsonNumber(json, "cell_max_voltage", datalayer.battery2.status.cell_max_voltage_mV);
  addJsonNumber(json, "cell_delta",
                datalayer.battery2.status.cell_max_voltage_mV - datalayer.battery2.status.cell_min_voltage_mV);
  addJsonNumber(json, "temp_min", static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0);
  addJsonNumber(json, "temp_max", static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0);
  addJsonBool(json, "contactor_status", datalayer.system.status.contactors_battery2_engaged);
  addJsonBool(json, "battery_allows_contactor_closing", datalayer.system.status.battery2_allows_contactor_closing,
              false);
  json += "}";
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
  // Charger information
  json += ",\"charger\":{";
  addJsonBool(json, "enabled_hv", datalayer.charger.charger_HV_enabled);
  addJsonBool(json, "enabled_aux12v", datalayer.charger.charger_aux12V_enabled);

#ifdef CHEVYVOLT_CHARGER
  float chgPwrDC = static_cast<float>(datalayer.charger.charger_stat_HVcur * datalayer.charger.charger_stat_HVvol);
  float chgPwrAC = static_cast<float>(datalayer.charger.charger_stat_ACcur * datalayer.charger.charger_stat_ACvol);
  float chgEff = (chgPwrAC > 0) ? (chgPwrDC / chgPwrAC * 100) : 0;

  addJsonNumber(json, "output_power", chgPwrDC);
  addJsonNumber(json, "efficiency", chgEff);
  addJsonNumber(json, "hvdc_voltage", datalayer.charger.charger_stat_HVvol);
  addJsonNumber(json, "hvdc_current", datalayer.charger.charger_stat_HVcur);
  addJsonNumber(json, "lvdc_voltage", datalayer.charger.charger_stat_LVvol);
  addJsonNumber(json, "lvdc_current", datalayer.charger.charger_stat_LVcur);
  addJsonNumber(json, "ac_voltage", datalayer.charger.charger_stat_ACvol);
  addJsonNumber(json, "ac_current", datalayer.charger.charger_stat_ACcur, false);
#endif

#ifdef NISSANLEAF_CHARGER
  float chgPwrDC = static_cast<float>(datalayer.charger.charger_stat_HVcur * 100);
  float hvdc_voltage = static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0;
  float hvdc_current = (hvdc_voltage > 0) ? (chgPwrDC / hvdc_voltage) : 0;

  addJsonNumber(json, "output_power", chgPwrDC);
  addJsonNumber(json, "hvdc_voltage", hvdc_voltage);
  addJsonNumber(json, "hvdc_current", hvdc_current);
  addJsonNumber(json, "ac_voltage", datalayer.charger.charger_stat_ACvol, false);
#endif

  json += "}";
#endif

  json += "}";
  return json;
}
