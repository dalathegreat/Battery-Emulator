#include "api_common.h"
#include "../../utils/events.h"

const char* api_bms_status_string(real_bms_status_enum status) {
  switch (status) {
    case BMS_ACTIVE:
      return "ACTIVE";
    case BMS_FAULT:
      return "FAULT";
    case BMS_STANDBY:
      return "STANDBY";
    case BMS_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

const char* api_balancing_status_string(balancing_status_enum status) {
  switch (status) {
    case BALANCING_STATUS_ERROR:
      return "Error";
    case BALANCING_STATUS_READY:
      return "Ready";
    case BALANCING_STATUS_ACTIVE:
      return "Active";
    case BALANCING_STATUS_BLOCKED:
      return "Blocked";
    case BALANCING_STATUS_UNKNOWN:
    default:
      return "Unknown";
  }
}

const char* api_system_status_string(system_status_enum status) {
  switch (status) {
    case ACTIVE:
      return "OK";
    case UPDATING:
      return "UPDATING";
    case FAULT:
      return "FAULT";
    case INACTIVE:
      return "INACTIVE";
    case STANDBY:
      return "STANDBY";
    default:
      return "UNKNOWN";
  }
}

const char* api_emulator_status_string() {
  switch (get_emulator_status()) {
    case EMULATOR_STATUS::STATUS_OK:
      return "OK";
    case EMULATOR_STATUS::STATUS_WARNING:
      return "WARNING";
    case EMULATOR_STATUS::STATUS_ERROR:
      return "ERROR";
    case EMULATOR_STATUS::STATUS_UPDATING:
      return "UPDATING";
    default:
      return "UNKNOWN";
  }
}

uint16_t api_count_balancing_cells(const DATALAYER_BATTERY_TYPE& b) {
  uint16_t count = 0;
  for (uint16_t i = 0; i < b.info.number_of_cells && i < MAX_AMOUNT_CELLS; i++) {
    if (b.status.cell_balancing_status[i]) {
      count++;
    }
  }
  return count;
}

void api_fill_pack_status(JsonObject o, const DATALAYER_BATTERY_TYPE& b, bool present) {
  o["present"] = present;
  o["voltage_V"] = static_cast<float>(b.status.voltage_dV) / 10.0f;
  o["current_A"] = static_cast<float>(b.status.current_dA) / 10.0f;
  o["reported_current_A"] = static_cast<float>(b.status.reported_current_dA) / 10.0f;
  o["power_W"] = b.status.active_power_W;
  o["soc_reported_pct"] = static_cast<float>(b.status.reported_soc) / 100.0f;
  o["soc_real_pct"] = static_cast<float>(b.status.real_soc) / 100.0f;
  o["soh_pct"] = static_cast<float>(b.status.soh_pptt) / 100.0f;
  o["temp_min_C"] = static_cast<float>(b.status.temperature_min_dC) / 10.0f;
  o["temp_max_C"] = static_cast<float>(b.status.temperature_max_dC) / 10.0f;
  o["cell_max_mV"] = b.status.cell_max_voltage_mV;
  o["cell_min_mV"] = b.status.cell_min_voltage_mV;
  o["cell_delta_mV"] = static_cast<uint16_t>(b.status.cell_max_voltage_mV - b.status.cell_min_voltage_mV);
  o["remaining_Wh"] = b.status.remaining_capacity_Wh;
  o["reported_remaining_Wh"] = b.status.reported_remaining_capacity_Wh;
  o["total_Wh"] = b.info.total_capacity_Wh;
  o["max_charge_power_W"] = b.status.max_charge_power_W;
  o["max_discharge_power_W"] = b.status.max_discharge_power_W;
  o["balancing_status"] = api_balancing_status_string(b.status.balancing_status);
  o["balancing_active_cells"] = api_count_balancing_cells(b);
  o["bms_status"] = api_bms_status_string(b.status.real_bms_status);
  o["can_alive"] = b.status.CAN_battery_still_alive > 0;
  o["number_of_cells"] = b.info.number_of_cells;
}

void api_send_json(AsyncWebServerRequest* request, const JsonDocument& doc) {
  String content;
  content.reserve(measureJson(doc) + 1);
  serializeJson(doc, content);
  request->send(200, "application/json", content);
}
