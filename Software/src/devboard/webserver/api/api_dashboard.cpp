// GET /api/dashboard - JSON mirror of the legacy dashboard page (processor()
// in webserver.cpp). Shape: DashboardResponse in frontend/src/types/dashboard.types.ts

#include "../../../battery/BATTERIES.h"
#include "../../../battery/Battery.h"
#include "../../../battery/Shunt.h"
#include "../../../charger/CHARGERS.h"
#include "../../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../../datalayer/datalayer.h"
#include "../../../devboard/safety/safety.h"
#include "../../../devboard/utils/millis64.h"
#include "../../../inverter/INVERTERS.h"
#include "../../hal/hal.h"
#include "../../wifi/wifi.h"
#include "../webserver.h"
#include "api_common.h"
#include "api_routes.h"

// Defined in webserver.cpp
extern String getConnectResultString(wl_status_t status);

static String api_uptime_human(uint64_t ms) {
  uint16_t days = ms / (1000ULL * 60 * 60 * 24);
  uint32_t secs_in_day = (ms / 1000ULL) % (60 * 60 * 24);
  uint32_t hours = secs_in_day / 3600;
  uint32_t minutes = (secs_in_day % 3600) / 60;
  uint32_t seconds = secs_in_day % 60;
  return String(days) + " days, " + String(hours) + " hours, " + String(minutes) + " minutes, " + String(seconds) +
         " seconds";
}

static const char* flash_mode_string(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO:
      return "QIO";
    case FM_QOUT:
      return "QOUT";
    case FM_DIO:
      return "DIO";
    case FM_DOUT:
      return "DOUT";
    default:
      return "Unknown";
  }
}

static void fill_dashboard_pack(JsonObject o, const DATALAYER_BATTERY_TYPE& b, Battery* batt, bool present) {
  api_fill_pack_status(o, b, present);
  o["reported_total_Wh"] = b.info.reported_total_capacity_Wh;
  o["max_charge_current_A"] = static_cast<float>(b.status.max_charge_current_dA) / 10.0f;
  o["max_discharge_current_A"] = static_cast<float>(b.status.max_discharge_current_dA) / 10.0f;
  o["soc_scaling_active"] = b.settings.soc_scaling_active;
  if (b.status.current_dA == 0) {
    o["flow"] = "idle";
  } else if (b.status.current_dA < 0) {
    o["flow"] = "discharging";
  } else {
    o["flow"] = "charging";
  }
  o["remote_limit_discharge"] = b.settings.remote_settings_limit_discharge;
  o["remote_limit_charge"] = b.settings.remote_settings_limit_charge;
  o["user_limit_discharge"] = b.settings.user_settings_limit_discharge;
  o["user_limit_charge"] = b.settings.user_settings_limit_charge;
  o["inverter_limit_discharge"] = b.settings.inverter_limits_discharge;
  o["inverter_limit_charge"] = b.settings.inverter_limits_charge;
  if (batt && batt->supports_real_BMS_status()) {
    o["real_bms_status"] = static_cast<int>(b.status.real_bms_status);
  }
}

static void handle_api_dashboard(AsyncWebServerRequest* request) {
  JsonDocument doc;

  doc["version"] = String(version_number);
  doc["hardware"] = esp32hal->name();
  uint64_t up_ms = millis64();
  doc["uptime_human"] = api_uptime_human(up_ms);
  doc["uptime_s"] = static_cast<uint32_t>(up_ms / 1000ULL);
  doc["cpu_temp_C"] = datalayer.system.info.CPU_temperature;

  // WiFi
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wl_status_t status = WiFi.status();
  wifi["ssid"] = String(ssid.c_str());
  wifi["connected"] = (status == WL_CONNECTED);
  if (status == WL_CONNECTED) {
    wifi["rssi"] = WiFi.RSSI();
    wifi["channel"] = WiFi.channel();
    wifi["hostname"] = String(WiFi.getHostname());
    wifi["ip"] = WiFi.localIP().toString();
  }
  wifi["state_text"] = getConnectResultString(status);
  if (ap_active) {
    wifi["ap_ip"] = WiFi.softAPIP().toString();
  }

  // Performance metrics (only when profiling is enabled, like the legacy page)
  if (datalayer.system.info.performance_measurement_active) {
    JsonObject perf = doc["performance"].to<JsonObject>();
    perf["free_heap"] = ESP.getFreeHeap();
    perf["max_alloc_heap"] = ESP.getMaxAllocHeap();
    perf["flash_mode"] = flash_mode_string(ESP.getFlashChipMode());
    perf["flash_mb"] = ESP.getFlashChipSize() / (1024 * 1024);
    perf["core_task_max_us"] = datalayer.system.status.core_task_max_us;
    perf["core_task_10s_max_us"] = datalayer.system.status.core_task_10s_max_us;
    perf["mqtt_task_10s_max_us"] = datalayer.system.status.mqtt_task_10s_max_us;
    perf["wifi_task_10s_max_us"] = datalayer.system.status.wifi_task_10s_max_us;
    perf["time_snap_10ms_us"] = datalayer.system.status.time_snap_10ms_us;
    perf["time_snap_values_us"] = datalayer.system.status.time_snap_values_us;
    perf["time_snap_comm_us"] = datalayer.system.status.time_snap_comm_us;
    perf["time_snap_cantx_us"] = datalayer.system.status.time_snap_cantx_us;
  }

  // Connected components
  JsonObject components = doc["components"].to<JsonObject>();
  if (inverter) {
    components["inverter"] = inverter->name();
    components["inverter_brand"] = datalayer.system.info.inverter_brand;
  }
  if (battery) {
    components["battery_protocol"] = datalayer.system.info.battery_protocol;
    components["double_battery"] = (battery2 != nullptr);
    components["triple_battery"] = (battery3 != nullptr);
    components["battery_chemistry_lfp"] = (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP);
  }
  if (user_selected_shunt_type != ShuntType::None) {
    components["shunt_protocol"] = datalayer.system.info.shunt_protocol;
  }
  if (charger) {
    components["charger"] = charger->name();
  }

  // Battery packs (always 3 entries; "present" marks the active ones)
  JsonArray packs = doc["packs"].to<JsonArray>();
  fill_dashboard_pack(packs.add<JsonObject>(), datalayer.battery, battery, battery != nullptr);
  fill_dashboard_pack(packs.add<JsonObject>(), datalayer.battery2, battery2, battery2 != nullptr);
  fill_dashboard_pack(packs.add<JsonObject>(), datalayer.battery3, battery3, battery3 != nullptr);

  // Contactor / power status
  JsonObject contactor = doc["contactor"].to<JsonObject>();
  contactor["pause_status_text"] = String(get_emulator_pause_status().c_str());
  contactor["emulator_pause_request_on"] = emulator_pause_request_ON;
  contactor["equipment_stop_active"] = datalayer.system.info.equipment_stop_active;
  contactor["emulator_allows_contactor_close"] = (datalayer.system.status.system_status != FAULT);
  contactor["inverter_allows_contactor_closing"] = datalayer.system.status.inverter_allows_contactor_closing;
  contactor["battery2_allowed_contactor_closing"] = datalayer.system.status.battery2_allowed_contactor_closing;
  contactor["battery3_allowed_contactor_closing"] = datalayer.system.status.battery3_allowed_contactor_closing;
  contactor["contactor_control_enabled"] = contactor_control_enabled;
  contactor["contactor_control_double_battery"] = contactor_control_enabled_double_battery;
  contactor["contactor_control_triple_battery"] = contactor_control_enabled_triple_battery;
  contactor["contactors_engaged"] = (datalayer.system.status.contactors_engaged == 1);
  contactor["contactors_state"] = datalayer.system.status.contactors_engaged;
  contactor["contactors_battery2_engaged"] = datalayer.system.status.contactors_battery2_engaged;
  contactor["contactors_battery3_engaged"] = datalayer.system.status.contactors_battery3_engaged;
  contactor["precharge_status"] = static_cast<int>(datalayer.system.status.precharge_status);
  contactor["pwm_contactor_control"] = pwm_contactor_control;

  // Charger details
  if (charger) {
    JsonObject chg = doc["charger_detail"].to<JsonObject>();
    chg["hv_enabled"] = datalayer.charger.charger_HV_enabled;
    chg["aux12v_enabled"] = datalayer.charger.charger_aux12V_enabled;
    chg["output_power_W"] = charger->outputPowerDC();
    chg["efficiency_supported"] = charger->efficiencySupported();
    if (charger->efficiencySupported()) {
      chg["efficiency_pct"] = charger->efficiency();
    }
    chg["hvdc_V"] = charger->HVDC_output_voltage();
    chg["hvdc_A"] = charger->HVDC_output_current();
    chg["lvdc_V"] = charger->LVDC_output_voltage();
    chg["lvdc_A"] = charger->LVDC_output_current();
    chg["ac_V"] = charger->AC_input_voltage();
    chg["ac_A"] = charger->AC_input_current();
  }

  // UI hints
  JsonObject ui = doc["ui"].to<JsonObject>();
  ui["web_logging_active"] = datalayer.system.info.web_logging_active;
  ui["sd_logging_active"] = datalayer.system.info.SD_logging_active;
  ui["webserver_auth"] = webserver_auth;
  ui["emulator_status"] = api_emulator_status_string();
  ui["system_status"] = api_system_status_string(datalayer.system.status.system_status);
  ui["new_webui_enabled"] = new_webui_enabled();

  api_send_json(request, doc);
}

void init_api_dashboard_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/dashboard", server, HTTP_GET, handle_api_dashboard);
}
