#include "metrics_html.h"
#ifdef PROMETHEUS_METRICS
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../utils/led_handler.h"

// External declarations
extern const char* version_number;

// Initialize default value for the device name
String PROMETHEUS_DEVICE_NAME = "battery-emulator-01";

String metrics_html_processor() {
  String output = "";
  String deviceLabel = "device=\"" + PROMETHEUS_DEVICE_NAME + "\"";

  // System metrics
  output += "be_system_version{value=\"" + String(version_number) + "\"," + deviceLabel + "} 1\n";

  // Calculate uptime in seconds
  unsigned long uptime_seconds = millis() / 1000;
  output += "be_system_uptime_seconds{" + deviceLabel + "} " + String(uptime_seconds) + "\n";

  output += "be_system_inverter_protocol{value=\"" + String(datalayer.system.info.inverter_protocol) + "\"," +
            deviceLabel + "} 1\n";
  output += "be_system_inverter_brand{value=\"" + String(datalayer.system.info.inverter_brand) + "\"," + deviceLabel +
            "} 1\n";
  output += "be_system_battery_protocol{value=\"" + String(datalayer.system.info.battery_protocol) + "\"," +
            deviceLabel + "} 1\n";
  output += "be_system_temperature{" + deviceLabel + "} " + String(datalayer.system.info.CPU_temperature) + "\n";
  output += "be_system_emulator_status{status=\"" + String(get_emulator_pause_status().c_str()) + "\"," + deviceLabel +
            "} 1\n";
  output += "be_system_equipment_stop{" + deviceLabel + "} " +
            String(datalayer.system.settings.equipment_stop_active ? 1 : 0) + "\n";

  // LED status
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
  output += "be_system_led_status{status=\"" + led_status + "\"," + deviceLabel + "} 1\n";

  // Battery metrics
  output += "be_battery_soc_real{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.real_soc) / 100.0) + "\n";
  output += "be_battery_soc_reported{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.reported_soc) / 100.0) + "\n";
  output += "be_battery_soc_scaling_active{" + deviceLabel + "} " +
            String(datalayer.battery.settings.soc_scaling_active ? 1 : 0) + "\n";
  output += "be_battery_soh{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.soh_pptt) / 100.0) + "\n";
  output += "be_battery_voltage{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0) + "\n";
  output += "be_battery_current{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.current_dA) / 10.0) + "\n";
  output += "be_battery_power{" + deviceLabel + "} " + String(datalayer.battery.status.active_power_W) + "\n";
  output +=
      "be_battery_capacity_total_wh{" + deviceLabel + "} " + String(datalayer.battery.info.total_capacity_Wh) + "\n";
  output += "be_battery_capacity_reported_wh{" + deviceLabel + "} " +
            String(datalayer.battery.info.reported_total_capacity_Wh) + "\n";
  output += "be_battery_capacity_remaining_wh{" + deviceLabel + "} " +
            String(datalayer.battery.status.remaining_capacity_Wh) + "\n";
  output += "be_battery_capacity_reported_remaining_wh{" + deviceLabel + "} " +
            String(datalayer.battery.status.reported_remaining_capacity_Wh) + "\n";

  // Power limits
  output += "be_battery_max_discharge_power{" + deviceLabel + "} " +
            String(datalayer.battery.status.max_discharge_power_W) + "\n";
  output +=
      "be_battery_max_charge_power{" + deviceLabel + "} " + String(datalayer.battery.status.max_charge_power_W) + "\n";
  output += "be_battery_max_discharge_current{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.max_discharge_current_dA) / 10.0) + "\n";
  output += "be_battery_max_charge_current{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.max_charge_current_dA) / 10.0) + "\n";
  output += "be_battery_user_settings_limit_discharge{" + deviceLabel + "} " +
            String(datalayer.battery.settings.user_settings_limit_discharge ? 1 : 0) + "\n";
  output += "be_battery_user_settings_limit_charge{" + deviceLabel + "} " +
            String(datalayer.battery.settings.user_settings_limit_charge ? 1 : 0) + "\n";
  output += "be_battery_inverter_limits_discharge{" + deviceLabel + "} " +
            String(datalayer.battery.settings.inverter_limits_discharge ? 1 : 0) + "\n";
  output += "be_battery_inverter_limits_charge{" + deviceLabel + "} " +
            String(datalayer.battery.settings.inverter_limits_charge ? 1 : 0) + "\n";

  // Cell information
  output +=
      "be_battery_cell_min_voltage{" + deviceLabel + "} " + String(datalayer.battery.status.cell_min_voltage_mV) + "\n";
  output +=
      "be_battery_cell_max_voltage{" + deviceLabel + "} " + String(datalayer.battery.status.cell_max_voltage_mV) + "\n";
  output += "be_battery_cell_delta{" + deviceLabel + "} " +
            String(datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV) + "\n";
  output += "be_battery_cell_delta_limit{" + deviceLabel + "} " +
            String(datalayer.battery.info.max_cell_voltage_deviation_mV) + "\n";

  // Temperature
  output += "be_battery_temp_min{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0) + "\n";
  output += "be_battery_temp_max{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0) + "\n";

  // Status
  output += "be_battery_bms_status{" + deviceLabel + "} " + String(datalayer.battery.status.bms_status) + "\n";

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
  output += "be_battery_bms_status_info{status=\"" + bms_status_text + "\"," + deviceLabel + "} 1\n";

  // Contactor information - fix for the contactors_engaged issue
#ifdef CONTACTOR_CONTROL
  output += "be_battery_contactor_status{" + deviceLabel + "} " +
            String(datalayer.system.settings.relays_engaged ? 1 : 0) + "\n";
#else
  // If there's no contactor control, we'll assume default true value
  output += "be_battery_contactor_status{" + deviceLabel + "} 1\n";
#endif
  
  output += "be_battery_allows_contactor_closing{" + deviceLabel + "} " +
            String(datalayer.system.status.battery_allows_contactor_closing ? 1 : 0) + "\n";
  output += "be_battery_inverter_allows_contactor_closing{" + deviceLabel + "} " +
            String(datalayer.system.status.inverter_allows_contactor_closing ? 1 : 0) + "\n";

  // Add cell voltage metrics
  int cellCount = datalayer.battery.info.number_of_cells;
  for (int i = 0; i < cellCount; i++) {
    int cellVoltage = datalayer.battery.status.cell_voltages_mV[i];
    output +=
        "be_battery_cell_voltage{index=\"" + String(i + 1) + "\"," + deviceLabel + "} " + String(cellVoltage) + "\n";
  }

#ifdef DOUBLE_BATTERY
  // Second battery metrics
  output += "be_battery2_soc_real{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery2.status.real_soc) / 100.0) + "\n";
  output += "be_battery2_soc_reported{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery2.status.reported_soc) / 100.0) + "\n";
  output += "be_battery2_soh{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery2.status.soh_pptt) / 100.0) + "\n";
  output += "be_battery2_voltage{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery2.status.voltage_dV) / 10.0) + "\n";
  output += "be_battery2_current{" + deviceLabel + "} " +
            String(static_cast<float>(datalayer.battery2.status.current_dA) / 10.0) + "\n";
  output += "be_battery2_power{" + deviceLabel + "} " + String(datalayer.battery2.status.active_power_W) + "\n";
  output +=
      "be_battery2_capacity_total_wh{" + deviceLabel + "} " + String(datalayer.battery2.info.total_capacity_Wh) + "\n";
  output += "be_battery2_capacity_reported_wh{" + deviceLabel + "} " +
            String(datalayer.battery2.info.reported_total_capacity_Wh) + "\n";
  output += "be_battery2_capacity_remaining_wh{" + deviceLabel + "} " +
            String(datalayer.battery2.status.remaining_capacity_Wh) + "\n";
  output += "be_battery2_capacity_reported_remaining_wh{" + deviceLabel + "} " +
            String(datalayer.battery2.status.reported_remaining_capacity_Wh) + "\n";
  output += "be_battery2_max_discharge_power{" + deviceLabel + "} " +
            String(datalayer.battery2.status.max_discharge_power_W) + "\n";
  output += "be_battery2_max_charge_power{" + deviceLabel + "} " +
            String(datalayer.battery2.status.max_charge_power_W) + "\n";
  output += "be_battery2_cell_min_voltage{" + deviceLabel + "} " +
            String(datalayer.battery2.status.cell_min_voltage_mV) + "\n";
  output += "be_battery2_cell_max_voltage{" + deviceLabel + "} " +
            String(datalayer.battery2.status.cell_max_voltage_mV) + "\n";
  output += "be_battery2_cell_delta{" + deviceLabel + "} " + 
            String(datalayer.battery2.status.cell_max_voltage_mV - datalayer.battery2.status.cell_min_voltage_mV) + "\n";

  // Fix for second battery contactors_battery2_engaged
#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
  output += "be_battery2_contactor_status{" + deviceLabel + "} " +
            String(datalayer.system.settings.relays_battery2_engaged ? 1 : 0) + "\n";
#else
  // If there's no contactor control, we'll assume default true value
  output += "be_battery2_contactor_status{" + deviceLabel + "} 1\n";
#endif

  output += "be_battery2_allows_contactor_closing{" + deviceLabel + "} " +
            String(datalayer.system.status.battery2_allows_contactor_closing ? 1 : 0) + "\n";

  // Add cell voltage metrics for battery 2
  int cellCount2 = datalayer.battery2.info.number_of_cells;
  for (int i = 0; i < cellCount2; i++) {
    int cellVoltage = datalayer.battery2.status.cell_voltages_mV[i];
    output +=
        "be_battery2_cell_voltage{index=\"" + String(i + 1) + "\"," + deviceLabel + "} " + String(cellVoltage) + "\n";
  }
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
  // Charger metrics
  output += "be_charger_enabled_hv{" + deviceLabel + "} " + String(datalayer.charger.charger_HV_enabled ? 1 : 0) + "\n";
  output += "be_charger_enabled_aux12v{" + deviceLabel + "} " + String(datalayer.charger.charger_aux12V_enabled ? 1 : 0) + "\n";

#ifdef CHEVYVOLT_CHARGER
  float chgPwrDC = static_cast<float>(datalayer.charger.charger_stat_HVcur * datalayer.charger.charger_stat_HVvol);
  float chgPwrAC = static_cast<float>(datalayer.charger.charger_stat_ACcur * datalayer.charger.charger_stat_ACvol);
  float chgEff = (chgPwrAC > 0) ? (chgPwrDC / chgPwrAC * 100) : 0;
  
  output += "be_charger_output_power{" + deviceLabel + "} " + String(chgPwrDC) + "\n";
  output += "be_charger_efficiency{" + deviceLabel + "} " + String(chgEff) + "\n";
  output += "be_charger_hvdc_voltage{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_HVvol) + "\n";
  output += "be_charger_hvdc_current{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_HVcur) + "\n";
  output += "be_charger_lvdc_voltage{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_LVvol) + "\n";
  output += "be_charger_lvdc_current{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_LVcur) + "\n";
  output += "be_charger_ac_voltage{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_ACvol) + "\n";
  output += "be_charger_ac_current{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_ACcur) + "\n";
#endif

#ifdef NISSANLEAF_CHARGER
  float chgPwrDC = static_cast<float>(datalayer.charger.charger_stat_HVcur * 100);
  float hvdc_voltage = static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0;
  float hvdc_current = (hvdc_voltage > 0) ? (chgPwrDC / hvdc_voltage) : 0;
  
  output += "be_charger_output_power{" + deviceLabel + "} " + String(chgPwrDC) + "\n";
  output += "be_charger_hvdc_voltage{" + deviceLabel + "} " + String(hvdc_voltage) + "\n";
  output += "be_charger_hvdc_current{" + deviceLabel + "} " + String(hvdc_current) + "\n";
  output += "be_charger_ac_voltage{" + deviceLabel + "} " + String(datalayer.charger.charger_stat_ACvol) + "\n";
#endif
#endif

  // Add event metrics
  int active_events = 0;
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* eventPtr = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (eventPtr->occurences > 0) {
      bool isActive = (eventPtr->state == EVENT_STATE_ACTIVE || eventPtr->state == EVENT_STATE_ACTIVE_LATCHED);
      const char* eventDesc = get_event_message_string((EVENTS_ENUM_TYPE)i);
      const char* eventLevel = get_event_level_string((EVENTS_ENUM_TYPE)i);

      output += "be_event{id=\"" + String(i) + "\",description=\"" + String(eventDesc) + "\",level=\"" +
                String(eventLevel) + "\"," + deviceLabel + "} " + String(isActive ? 1 : 0) + "\n";

      if (isActive)
        active_events++;
    }
  }
  output += "be_events_active_count{" + deviceLabel + "} " + String(active_events) + "\n";

  return output;
}
#endif