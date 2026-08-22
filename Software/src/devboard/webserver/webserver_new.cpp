// webserver_new.cpp
//
// Provides the API endpoints required by the new (Preact) frontend.

#include "webserver_new.h"
#include <vector>
#include "webserver_settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../battery/MG-GEN1-BATTERY.h"
#include "../../battery/RENAULT-ZOE-GEN1-BATTERY.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/timer.h"
#include "../../devboard/webserver/frontend.h"
#include "../../inverter/INVERTERS.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"

#include "webserver_can_streaming.h"

#include <Update.h>

// True when the user has updated settings that need a reboot to take effect.
bool settingsUpdated = false;

// Declared as int to match how the rest of the codebase uses it (the State
// enum it is actually declared with is local to comm_contactorcontrol.cpp).
extern int contactorStatus;

std::string http_username = "admin";
std::string http_password;
bool webserver_auth = false;
MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

static AsyncWebServer newServer(80);

bool webserver_auth_is_ready() {
  return webserver_auth && !http_username.empty() && !http_password.empty();
}

// Wraps a handler with an authentication check.
using NewWsHandler = std::function<void(AsyncWebServerRequest*)>;
static void route(AsyncWebServer& server, const char* uri, WebRequestMethodComposite method, NewWsHandler handler) {
  server.on(uri, method, [handler](AsyncWebServerRequest* request) {
    if (webserver_auth_is_ready() && !request->authenticate(http_username.c_str(), http_password.c_str())) {
      return request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
    }
    handler(request);
  });
}

// ---------------------------------------------------------------------------
// Frontend SPA - the default route for all unhandled paths
// ---------------------------------------------------------------------------
static void send_frontend(AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response = request->beginResponse(200, "text/html", (const uint8_t*)html_data, html_data_len);
  response->addHeader("Content-Encoding", "gzip");
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}

// ---------------------------------------------------------------------------
// Events that are battery-specific
// ---------------------------------------------------------------------------
static const EVENTS_ENUM_TYPE GENERIC_BATTERY_EVENTS[] = {
    EVENT_BATTERY_OVERHEAT,
    EVENT_BATTERY_FROZEN,
    EVENT_BATTERY_OVERVOLTAGE,
    EVENT_BATTERY_UNDERVOLTAGE,
    EVENT_CELL_OVER_VOLTAGE,
    EVENT_CELL_CRITICAL_OVER_VOLTAGE,
    EVENT_CELL_UNDER_VOLTAGE,
    EVENT_CELL_CRITICAL_UNDER_VOLTAGE,
    EVENT_SOH_LOW,
    EVENT_SOC_PLAUSIBILITY_ERROR,
    EVENT_CELL_DEVIATION_HIGH,
    EVENT_SOH_DIFFERENCE,
};

static const char* get_inverter_status() {
  if (get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state == EVENT_STATE_ACTIVE ||
      get_event_pointer(EVENT_CAN_INVERTER_MISSING)->state == EVENT_STATE_ACTIVE_LATCHED) {
    return "ERROR";
  } else if (!datalayer.system.status.inverter_allows_contactor_closing) {
    return "INACTIVE";
  } else {
    return "OK";
  }
}

static const char* get_battery_status(int battery_index) {
  auto evptr = get_event_pointer(battery_index == 1   ? EVENT_CAN_BATTERY_MISSING
                                 : battery_index == 2 ? EVENT_CAN_BATTERY2_MISSING
                                                      : EVENT_CAN_BATTERY3_MISSING);
  if (evptr->state == EVENT_STATE_ACTIVE) {
    return "ERROR";
  }

  for (auto event_type : GENERIC_BATTERY_EVENTS) {
    auto ev = get_event_pointer(event_type);
    if (ev->level == EVENT_LEVEL_ERROR &&
        (ev->state == EVENT_STATE_ACTIVE || ev->state == EVENT_STATE_ACTIVE_LATCHED)) {
      return "ERROR";
    }
  }

  evptr = get_event_pointer(EVENT_CAN_CORRUPTED_WARNING);
  if (evptr->state == EVENT_STATE_ACTIVE && evptr->data == (battery_index == 1   ? can_config.battery
                                                            : battery_index == 2 ? can_config.battery_double
                                                                                 : can_config.battery_triple)) {
    return "WARNING";
  }

  for (auto event_type : GENERIC_BATTERY_EVENTS) {
    auto ev = get_event_pointer(event_type);
    if (ev->level == EVENT_LEVEL_WARNING &&
        (ev->state == EVENT_STATE_ACTIVE || ev->state == EVENT_STATE_ACTIVE_LATCHED)) {
      return "WARNING";
    }
  }

  return "OK";
}

static void register_status_route(AsyncWebServer& server) {
  route(server, "/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;

    doc["hardware"] = esp32hal->name();
    doc["firmware"] = String(version_number);
    doc["temp"] = datalayer.system.info.CPU_temperature;
    doc["uptime"] = millis64();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["max_alloc"] = ESP.getMaxAllocHeap();

    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["channel"] = WiFi.channel();
    doc["hostname"] = WiFi.getHostname();
    doc["ip"] = WiFi.localIP().toString();
    // Current DHCP lease, used by the settings page to hint/prefill the static IP
    // fields. Empty without a station link so the UI never suggests 0.0.0.0.
    doc["gateway"] = WiFi.isConnected() ? WiFi.gatewayIP().toString() : String("");
    doc["subnet"] = WiFi.isConnected() ? WiFi.subnetMask().toString() : String("");
    IPAddress dns_ip = WiFi.dnsIP();
    doc["dns"] = (WiFi.isConnected() && dns_ip != IPAddress(0, 0, 0, 0)) ? dns_ip.toString() : String("");
    doc["wifi_status"] = (int)WiFi.status();

    doc["status"] = getBMSStatus(datalayer.system.status.system_status);
    doc["pause_status"] = get_emulator_pause_status();
    doc["pause"] = emulator_pause_request_ON;
    doc["estop"] = datalayer.system.info.equipment_stop_active;

    JsonArray batteries = doc["battery"].to<JsonArray>();
    auto add_battery = [&](Battery* battery, auto& bat_data, int bat_index) {
      JsonObject bat = batteries.add<JsonObject>();
      bat["protocol"] = datalayer.system.info.battery_protocol;
      bat["chemistry"] = (int)bat_data.info.chemistry;
      bat["soc_scaling"] = bat_data.settings.soc_scaling_active;
      bat["real_soc"] = static_cast<float>(bat_data.status.real_soc) / 100.0f;
      bat["reported_soc"] = static_cast<float>(bat_data.status.reported_soc) / 100.0f;
      bat["soh"] = static_cast<float>(bat_data.status.soh_pptt) / 100.0f;
      bat["v"] = static_cast<float>(bat_data.status.voltage_dV) / 10.0f;
      bat["i"] = static_cast<float>(bat_data.status.current_dA) / 10.0f;
      bat["p"] = bat_data.status.active_power_W;
      bat["total_capacity"] = bat_data.info.total_capacity_Wh;
      bat["reported_total_capacity"] = bat_data.info.reported_total_capacity_Wh;
      bat["remaining_capacity"] = bat_data.status.remaining_capacity_Wh;
      bat["reported_remaining_capacity"] = bat_data.status.reported_remaining_capacity_Wh;
      bat["temp_max"] = static_cast<float>(bat_data.status.temperature_max_dC) / 10.0f;
      bat["temp_min"] = static_cast<float>(bat_data.status.temperature_min_dC) / 10.0f;
      bat["charge_i_max"] = static_cast<float>(bat_data.status.max_charge_current_dA) / 10.0f;
      bat["discharge_i_max"] = static_cast<float>(bat_data.status.max_discharge_current_dA) / 10.0f;
      bat["charge_p_max"] = bat_data.status.max_charge_power_W;
      bat["discharge_p_max"] = bat_data.status.max_discharge_power_W;
      bat["cell_mv_max"] = bat_data.status.cell_max_voltage_mV;
      bat["cell_mv_min"] = bat_data.status.cell_min_voltage_mV;
      bat["v_max"] = static_cast<float>(bat_data.info.max_design_voltage_dV) / 10.0f;
      bat["v_min"] = static_cast<float>(bat_data.info.min_design_voltage_dV) / 10.0f;
      bat["inverter_limits_discharge"] = bat_data.settings.inverter_limits_discharge;
      bat["user_settings_limit_discharge"] = bat_data.settings.user_settings_limit_discharge;
      bat["inverter_limits_charge"] = bat_data.settings.inverter_limits_charge;
      bat["user_settings_limit_charge"] = bat_data.settings.user_settings_limit_charge;
      bat["inverter_allows_contactor_closing"] = datalayer.system.status.inverter_allows_contactor_closing;

      bat["status"] = get_battery_status(bat_index);
      if (battery != nullptr && battery->supports_real_BMS_status()) {
        bat["real_bms_status"] = (int)bat_data.status.real_bms_status;
      }
    };

    if (battery != nullptr) {
      add_battery(battery, datalayer.battery, 1);
      if (battery2 != nullptr) {
        add_battery(battery2, datalayer.battery2, 2);
      }
      if (battery3 != nullptr) {
        add_battery(battery3, datalayer.battery3, 3);
      }
    }
    if (inverter != nullptr) {
      JsonObject inv = doc["inverter"].to<JsonObject>();
      inv["name"] = inverter->name();
      inv["status"] = get_inverter_status();
    }
    if (contactor_control_enabled) {
      JsonObject con = doc["contactor"].to<JsonObject>();
      con["state"] = (int)contactorStatus;
    }

    JsonArray events = doc["events"].to<JsonArray>();
    uint64_t current_timestamp = millis64();
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
      auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
      if (event_pointer->occurences > 0) {
        JsonObject ev = events.add<JsonObject>();
        ev["age"] = current_timestamp - event_pointer->timestamp;
        ev["level"] = get_event_level_string((EVENTS_ENUM_TYPE)i);
        // Also include all the other fields
        ev["type"] = get_event_enum_string((EVENTS_ENUM_TYPE)i);
        ev["count"] = event_pointer->occurences;
        ev["data"] = event_pointer->data;
        ev["message"] = get_event_message_string((EVENTS_ENUM_TYPE)i);
      }
    }

    String payload;
    serializeJson(doc, payload);
    request->send(200, "application/json", payload);
  });

  route(server, "/api/cells", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray batteryArr = doc["battery"].to<JsonArray>();
    JsonObject bat = batteryArr.add<JsonObject>();
    bat["temp_min"] = static_cast<float>(datalayer.battery.status.temperature_min_dC) / 10.0f;
    bat["temp_max"] = static_cast<float>(datalayer.battery.status.temperature_max_dC) / 10.0f;
    JsonArray data = bat["voltages"].to<JsonArray>();
    for (size_t i = 0; i < datalayer.battery.info.number_of_cells && i < MAX_AMOUNT_CELLS; i++) {
      data.add(datalayer.battery.status.cell_voltages_mV[i]);
    }
    if (battery2 != nullptr) {
      JsonObject bat2 = batteryArr.add<JsonObject>();
      bat2["temp_min"] = static_cast<float>(datalayer.battery2.status.temperature_min_dC) / 10.0f;
      bat2["temp_max"] = static_cast<float>(datalayer.battery2.status.temperature_max_dC) / 10.0f;
      JsonArray data2 = bat2["voltages"].to<JsonArray>();
      for (size_t i = 0; i < datalayer.battery2.info.number_of_cells && i < MAX_AMOUNT_CELLS; i++) {
        data2.add(datalayer.battery2.status.cell_voltages_mV[i]);
      }
    }

    String payload;
    serializeJson(doc, payload);
    request->send(200, "application/json", payload);
  });

  /*
  Redundant now
  route(server, "/api/events", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray events = doc["events"].to<JsonArray>();
    std::vector<EventData> order_events;
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
      auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
      if (event_pointer->occurences > 0) {
        order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
      }
    }
    std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampDesc);
    uint64_t current_timestamp = millis64();
    for (const auto& event : order_events) {
      JsonObject ev = events.add<JsonObject>();
      ev["type"] = get_event_enum_string(event.event_handle);
      ev["level"] = get_event_level_string(event.event_handle);
      ev["age"] = current_timestamp - event.event_pointer->timestamp;
      ev["count"] = event.event_pointer->occurences;
      ev["data"] = event.event_pointer->data;
      ev["message"] = get_event_message_string(event.event_handle);
    }

    String payload;
    serializeJson(doc, payload);
    request->send(200, "application/json", payload);
  });
  */

  route(server, "/api/events/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
    reset_all_events();
    request->send(204);
  });
}

// ---------------------------------------------------------------------------
// Extended battery info (binary), old-style battery HTML, battery commands
// ---------------------------------------------------------------------------
static void register_battery_routes(AsyncWebServer& server) {
  route(server, "/api/batold", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (battery != nullptr) {
      String html = battery->get_status_renderer().get_status_html();
      request->send(200, "text/html", html);
    } else {
      request->send(204);
    }
  });

  route(server, "/api/batext", HTTP_GET, [](AsyncWebServerRequest* request) {
    std::vector<uint8_t> payload;
    uint32_t btype = (uint32_t)user_selected_battery_type;
    payload.insert(payload.end(), (uint8_t*)&btype, (uint8_t*)&btype + sizeof(btype));

    const uint8_t* data = nullptr;
    size_t size = 0;

    // Special case: the MG Gen1 and Renault Zoe Gen1 batteries are C++ classes,
    // not plain DATALAYER_INFO structs. Dump the raw memory of the class object
    // itself so the frontend can inspect its fields (see
    // tools/extract_datalayer_info_structures.py).
    if (battery != nullptr &&
        (user_selected_battery_type == BatteryType::MgGen1 || user_selected_battery_type == BatteryType::RenaultZoe1)) {
      data = (const uint8_t*)battery;
      size =
          (user_selected_battery_type == BatteryType::MgGen1) ? sizeof(MgGen1Battery) : sizeof(RenaultZoeGen1Battery);
    } else {
      switch (user_selected_battery_type) {
        case BatteryType::BoltAmpera:
          data = (const uint8_t*)&datalayer_extended.boltampera;
          size = sizeof(datalayer_extended.boltampera);
          break;
        case BatteryType::BmwPhev:
          data = (const uint8_t*)&datalayer_extended.bmwphev;
          size = sizeof(datalayer_extended.bmwphev);
          break;
        case BatteryType::BydAtto3:
          data = (const uint8_t*)&datalayer_extended.bydAtto3;
          size = sizeof(datalayer_extended.bydAtto3);
          break;
        case BatteryType::CellPowerBms:
          data = (const uint8_t*)&datalayer_extended.cellpower;
          size = sizeof(datalayer_extended.cellpower);
          break;
        case BatteryType::Chademo:
          data = (const uint8_t*)&datalayer_extended.chademo;
          size = sizeof(datalayer_extended.chademo);
          break;
        case BatteryType::StellantisEcmp:
          data = (const uint8_t*)&datalayer_extended.stellantisECMP;
          size = sizeof(datalayer_extended.stellantisECMP);
          break;
        case BatteryType::GeelyGeometryC:
          data = (const uint8_t*)&datalayer_extended.geometryC;
          size = sizeof(datalayer_extended.geometryC);
          break;
        case BatteryType::KiaHyundai64:
          data = (const uint8_t*)&datalayer_extended.KiaHyundai64;
          size = sizeof(datalayer_extended.KiaHyundai64);
          break;
        case BatteryType::TeslaModel3Y:
        case BatteryType::TeslaModelSX:
          data = (const uint8_t*)&datalayer_extended.tesla;
          size = sizeof(datalayer_extended.tesla);
          break;
        case BatteryType::NissanLeaf:
          data = (const uint8_t*)&datalayer_extended.nissanleaf;
          size = sizeof(datalayer_extended.nissanleaf);
          break;
        case BatteryType::Meb:
          data = (const uint8_t*)&datalayer_extended.meb;
          size = sizeof(datalayer_extended.meb);
          break;
        case BatteryType::VolvoSpa:
          data = (const uint8_t*)&datalayer_extended.VolvoPolestar;
          size = sizeof(datalayer_extended.VolvoPolestar);
          break;
        case BatteryType::VolvoSpaHybrid:
          data = (const uint8_t*)&datalayer_extended.VolvoHybrid;
          size = sizeof(datalayer_extended.VolvoHybrid);
          break;
        case BatteryType::RenaultZoe2:
          data = (const uint8_t*)&datalayer_extended.zoePH2;
          size = sizeof(datalayer_extended.zoePH2);
          break;
        default:
          break;
      }
    }
    if (data != nullptr) {
      payload.insert(payload.end(), data, data + size);
    }
    // NB: request->send(code, type, uint8_t*, len) hands the buffer pointer to an
    // AsyncProgmemResponse, which reads it asynchronously during the TCP send.
    // A local std::vector would be freed before then, so the payload is copied
    // into an AsyncResponseStream (which owns its buffer) instead.
    AsyncResponseStream* response = new AsyncResponseStream("application/octet-stream", payload.size());
    response->write(payload.data(), payload.size());
    request->send(response);
  });

  // GET /api/batteries/   - JSON list of available commands per battery
  // POST /api/batteries/<id>/<command> - run a command
  server.on("/api/batteries/*", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest* request) {
    if (webserver_auth_is_ready() && !request->authenticate(http_username.c_str(), http_password.c_str())) {
      return request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
    }
    if (request->method() == HTTP_GET) {
      JsonDocument doc;
      JsonArray batteries = doc["battery"].to<JsonArray>();
      auto add_battery_commands = [](JsonObject& commands, Battery* battery) {
        if (battery->supports_reset_SOH())
          commands["reset_soh"] = true;
        if (battery->supports_reset_crash())
          commands["reset_crash"] = true;
        if (battery->supports_clear_isolation())
          commands["clear_isolation"] = true;
        if (battery->supports_reset_BMS())
          commands["reset_bms"] = true;
        if (battery->supports_reset_SOC())
          commands["reset_soc"] = true;
        if (battery->supports_reset_NVROL())
          commands["reset_nvrol"] = true;
        if (battery->supports_reset_DTC())
          commands["reset_dtc"] = true;
        if (battery->supports_read_DTC())
          commands["read_dtc"] = true;
        if (battery->supports_reset_BECM())
          commands["reset_becm"] = true;
        if (battery->supports_calibrate_SOC())
          commands["calibrate_soc"] = true;
        if (battery->supports_contactor_close())
          commands["contactor_close"] = true;
        if (battery->supports_contactor_reset())
          commands["contactor_reset"] = true;
        if (battery->supports_real_BMS_status())
          commands["real_bms_status"] = true;
        if (battery->supports_toggle_SOC_method())
          commands["toggle_soc_method"] = true;
        if (battery->supports_energy_saving_mode_reset())
          commands["energy_saving_mode_reset"] = true;
        if (battery->supports_factory_mode_method())
          commands["factory_mode_method"] = true;
        if (battery->supports_chademo_restart())
          commands["chademo_restart"] = true;
        if (battery->supports_chademo_stop())
          commands["chademo_stop"] = true;
        if (battery->supports_balancing())
          commands["balancing"] = true;
        if (battery->is_balancing_active())
          commands["balancing_active"] = true;
      };
      if (battery != nullptr) {
        JsonObject bat = batteries.add<JsonObject>();
        bat["id"] = "1";
        JsonObject commands = bat["commands"].to<JsonObject>();
        add_battery_commands(commands, battery);
      }
      if (battery2 != nullptr) {
        JsonObject bat = batteries.add<JsonObject>();
        bat["id"] = "2";
        JsonObject commands = bat["commands"].to<JsonObject>();
        add_battery_commands(commands, battery2);
      }
      if (battery3 != nullptr) {
        JsonObject bat = batteries.add<JsonObject>();
        bat["id"] = "3";
        JsonObject commands = bat["commands"].to<JsonObject>();
        add_battery_commands(commands, battery3);
      }

      String payload;
      serializeJson(doc, payload);
      request->send(200, "application/json", payload);
      return;
    }

    // HTTP POST : run a command for a specific battery
    // url = /api/batteries/<id>/<command>
    // AsyncWebServer only lets us match /api/batteries/* without needing
    // ASYNCWEBSERVER_REGEX, so we parse the suffix out of request->url().
    String path = request->url();
    const char* rel = path.c_str() + strlen("/api/batteries");
    String wildcard = rel;
    int first_slash = wildcard.indexOf('/');
    if (first_slash < 0) {
      request->send(404, "text/plain", "Not found");
      return;
    }
    int second_slash = wildcard.indexOf('/', first_slash + 1);
    String id_part = wildcard.substring(first_slash + 1, second_slash);
    String action_part = (second_slash < 0) ? String() : wildcard.substring(second_slash + 1);

    Battery* bat = nullptr;
    if (battery != nullptr && id_part == "1")
      bat = battery;
    else if (battery2 != nullptr && id_part == "2")
      bat = battery2;
    else if (battery3 != nullptr && id_part == "3")
      bat = battery3;

    if (bat == nullptr) {
      request->send(404, "text/plain", "Not found");
      return;
    }

    if (action_part == "reset_soh" && bat->supports_reset_SOH())
      bat->reset_SOH();
    else if (action_part == "reset_crash" && bat->supports_reset_crash())
      bat->reset_crash();
    else if (action_part == "clear_isolation" && bat->supports_clear_isolation())
      bat->clear_isolation();
    else if (action_part == "reset_bms" && bat->supports_reset_BMS())
      bat->reset_BMS();
    else if (action_part == "reset_soc" && bat->supports_reset_SOC())
      bat->reset_SOC();
    else if (action_part == "reset_nvrol" && bat->supports_reset_NVROL())
      bat->reset_NVROL();
    else if (action_part == "reset_dtc" && bat->supports_reset_DTC())
      bat->reset_DTC();
    else if (action_part == "read_dtc" && bat->supports_read_DTC())
      bat->read_DTC();
    else if (action_part == "reset_becm" && bat->supports_reset_BECM())
      bat->reset_BECM();
    else if (action_part == "calibrate_soc" && bat->supports_calibrate_SOC())
      bat->calibrate_SOC();
    else if (action_part == "contactor_close" && bat->supports_contactor_close())
      bat->request_close_contactors();
    else if (action_part == "contactor_open" && bat->supports_contactor_close())
      bat->request_open_contactors();
    else if (action_part == "contactor_reset" && bat->supports_contactor_reset())
      bat->request_open_contactors();
    else if (action_part == "toggle_soc_method" && bat->supports_toggle_SOC_method())
      bat->toggle_SOC_method();
    else if (action_part == "energy_saving_mode_reset" && bat->supports_energy_saving_mode_reset())
      bat->reset_energy_saving_mode();
    else if (action_part == "factory_mode_method" && bat->supports_factory_mode_method())
      bat->set_factory_mode();
    else if (action_part == "chademo_restart" && bat->supports_chademo_restart())
      bat->chademo_restart();
    else if (action_part == "chademo_stop" && bat->supports_chademo_stop())
      bat->chademo_stop();
    else if (action_part == "start_balancing" && bat->supports_balancing())
      bat->initiate_balancing();
    else if (action_part == "stop_balancing" && bat->supports_balancing()) {
      if (bat->is_balancing_active())
        bat->end_balancing();
    } else {
      request->send(404, "text/plain", "Unknown command");
      return;
    }
    request->send(204);
  });

  // BYD Atto 3 isolation monitor commands (RoutineControl 0x2008). One
  // control, applied to every BYD battery at once - mirrors the old
  // /bydAtto3IsoEnable|Disable routes. The frontend (ext/byd_atto3.tsx) POSTs
  // to these; the current state is read from the batext blob.
  route(server, "/api/bydatto3/iso/enable", HTTP_POST, [](AsyncWebServerRequest* request) {
    datalayer_extended.bydAtto3.UserRequestIsoRoutineEnable = true;
    datalayer_extended.bydAtto3_2.UserRequestIsoRoutineEnable = true;
    request->send(204);
  });
  route(server, "/api/bydatto3/iso/disable", HTTP_POST, [](AsyncWebServerRequest* request) {
    datalayer_extended.bydAtto3.UserRequestIsoRoutineDisable = true;
    datalayer_extended.bydAtto3_2.UserRequestIsoRoutineDisable = true;
    request->send(204);
  });
}

// ---------------------------------------------------------------------------
// Control: pause / e-stop / reboot / stats / log
// ---------------------------------------------------------------------------
static void register_control_routes(AsyncWebServer& server) {
  route(server, "/api/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
    // We return the current uptime, to allow the frontend to easily tell when
    // the reboot has completed.
    String payload = "{\"uptime\": ";
    payload += millis64();
    payload += "}";
    request->send(200, "application/json", payload);
    hold_pins_across_reset();
    graceful_restart();
  });

  auto handle_battery_set = [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total,
                               bool is_estop) {
    if (webserver_auth_is_ready() && index == 0 &&
        !request->authenticate(http_username.c_str(), http_password.c_str())) {
      request->requestAuthentication(AsyncAuthType::AUTH_BASIC, WEB_AUTH_REALM);
      return;
    }
    // The frontend POSTs a raw body of "1" or "0".
    if (len >= 1 && data[0] == '1') {
      setBatteryPause(true, false, is_estop ? EquipmentStop::STOP : EquipmentStop::UNCHANGED);
    } else if (len >= 1 && data[0] == '0') {
      // TODO - should a stop-estop always unpause?
      setBatteryPause(false, false, is_estop ? EquipmentStop::RESUME : EquipmentStop::UNCHANGED);
    }
    if (index + len >= total) {
      request->send(204);
    }
  };

  server.on(
      "/api/pause", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
      [&, handle_battery_set](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        handle_battery_set(request, data, len, index, total, false);
      });

  server.on(
      "/api/estop", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
      [&, handle_battery_set](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        handle_battery_set(request, data, len, index, total, true);
      });

  route(server, "/api/stats", HTTP_GET, [](AsyncWebServerRequest* request) {
    String payload;
    payload.reserve(5000);
    char output[3000] = {0};
    vTaskList(output);
    payload += output;
    payload += "\n";
    vTaskGetRunTimeStats(output);
    payload += output;
    request->send(200, "text/plain", payload);
  });

  // GET /api/log - incremental web-log fetch.
  //
  // The web log is a ring buffer - each poll, the frontend keeps the byte
  // position it has received up to, and asks for the small amount of new data
  // since then:
  //   GET /api/log            -> everything currently in the ring (first load)
  //   GET /api/log?pos=<uint> -> only bytes after <uint>
  //
  // Response (application/octet-stream):
  //   bytes 0..7 : uint64 LE - byte position to request next time
  //   bytes 8..  : raw log bytes
  //
  // The position only ever increases, so a wrap around implies that the device has
  // rebooted.
  route(server, "/api/log", HTTP_GET, [](AsyncWebServerRequest* request) {
    constexpr size_t LOG_CHUNK_MAX = 4096;  // steady-state polls stay small

    // Parse ?pos= as a plain decimal uint64.
    uint64_t from = 0;
    if (request->hasParam("pos")) {
      const char* s = request->getParam("pos")->value().c_str();
      while (*s >= '0' && *s <= '9') {
        from = from * 10 + (uint64_t)(*s - '0');
        s++;
      }
    }

    // Allocate a buffer to hold the chunk whilst we send it out (it is too big
    // to fit on the stack).
    char* chunk = (char*)malloc(LOG_CHUNK_MAX);
    if (chunk == nullptr) {
      request->send(503, "text/plain", "Out of memory");
      return;
    }

    uint64_t pos = 0;
    size_t len = web_log_fetch(from, chunk, LOG_CHUNK_MAX, &pos);

    AsyncResponseStream* response = new AsyncResponseStream("application/octet-stream", len + sizeof(pos));
    // Write the position directly (little-endian)
    response->write((const uint8_t*)&pos, sizeof(pos));
    if (len > 0) {
      response->write((const uint8_t*)chunk, len);
    }
    free(chunk);
    request->send(response);
  });
}

// ---------------------------------------------------------------------------
// CAN sender (POST binary frames to /api/cansend?if=<interface>)
// ---------------------------------------------------------------------------
struct __attribute__((packed)) NewCanFrame {
  uint32_t timestamp;
  uint32_t id;
  uint8_t len;
  uint8_t bus;
  uint8_t data[64];
};

static void register_can_sender_route(AsyncWebServer& server) {
  server.on(
      "/api/cansend", HTTP_POST, [](AsyncWebServerRequest* request) { request->send(204); }, nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (!request->hasParam("if")) {
          request->send(400, "text/plain", "Missing interface parameter.");
          request->abort();
          return;
        }
        int can_interface = request->getParam("if")->value().toInt();

        const size_t header_len = 10;  // timestamp (4) + id (4) + len (1) + bus (1)
        uint8_t* ptr = data;
        size_t remaining = len;
        while (remaining >= header_len) {
          NewCanFrame frame;
          memcpy(&frame, ptr, header_len);
          size_t frame_length = header_len + frame.len;
          if (remaining < frame_length)
            break;
          if (frame.len > 64) {
            request->send(400, "text/plain", "Invalid CAN frame length.");
            request->abort();
            return;
          }
          memcpy(frame.data, ptr + header_len, frame.len);

          CAN_Interface iface = can_interface == 15 ? (CAN_Interface)(frame.bus / 2) : (CAN_Interface)can_interface;

          CAN_frame send_frame = {
              .FD = false, .ext_ID = frame.id > 0x7FF, .DLC = frame.len, .ID = frame.id, .data = {}};
          memcpy(send_frame.data.u8, frame.data, frame.len);
          transmit_can_frame_to_interface(&send_frame, iface);

          ptr += frame_length;
          remaining -= frame_length;
        }
        if (index + len >= total) {
          request->send(204);
        }
      });
}

static void on_ota_failure() {
  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);
  // Unpause battery (preserving equipment stop if set)
  setBatteryPause(false, false, EquipmentStop::UNCHANGED, false);
}

// ---------------------------------------------------------------------------
// OTA (the new frontend flashes via /ota/start + /ota/upload)
// ---------------------------------------------------------------------------
static void register_ota_routes(AsyncWebServer& server) {
  route(server, "/ota/start", HTTP_GET, [](AsyncWebServerRequest* request) {
    int mode = U_FLASH;
    if (request->hasParam("mode") && request->getParam("mode")->value() == "fs") {
      mode = U_SPIFFS;
    }
    if (request->hasParam("hash")) {
      String md5_hash = request->getParam("hash")->value();
      if (md5_hash.length() >= 32 && !Update.setMD5(md5_hash.c_str())) {
        request->send(400, "text/plain", "MD5 parameter invalid.");
        return;
      }
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, mode)) {
      request->send(400, "text/plain", "Not ok");
      return;
    }
    setBatteryPause(true, false, EquipmentStop::UNCHANGED, false);
    set_event(EVENT_OTA_UPDATE, 0);
    clear_event(EVENT_OTA_UPDATE_TIMEOUT);
    ota_timeout_timer.reset();
    ota_active = true;
    request->send(200, "text/plain", "OK");
  });

  server.on(
      "/ota/upload", HTTP_POST, [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "OK"); }, nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        ota_timeout_timer.reset();
        if (Update.write(data, len) != len) {
          on_ota_failure();
          request->send(400, "text/plain", "Update write failed.");
          request->abort();
          return;
        }
        if (index + len >= total) {
          if (Update.end(true)) {
            hold_pins_across_reset();
            graceful_restart();
            request->send(200, "text/plain", "OK");
          } else {
            on_ota_failure();
            request->send(500, "text/plain", "Update failed.");
          }
        }
      });
}

void init_webserver() {
  newServer.reset();

  register_status_route(newServer);
  register_battery_routes(newServer);
  register_settings_route(newServer);
  register_control_routes(newServer);
  register_can_sender_route(newServer);
  register_dump_can_route(newServer);
  register_ota_routes(newServer);

  // Every other path serves the (gzipped) frontend
  newServer.onNotFound([](AsyncWebServerRequest* request) { send_frontend(request); });

  newServer.begin();
}

/* Functions copied from old webserver */

const char* getCANInterfaceName(CAN_Interface interface) {
  switch (interface) {
    case CAN_NATIVE:
      return "CAN";
    case CANFD_NATIVE:
      return "CAN-FD Native";
    case CAN_ADDON_MCP2515:
      return "Add-on CAN via GPIO MCP2515";
    case CANFD_ADDON_MCP2518:
      return "Add-on CAN-FD via GPIO MCP2518";
    case CANFD_ADDON_MCP2518_2:
      return "Add-on CAN-FD #2 via GPIO MCP2518";
    default:
      return "UNKNOWN";
  }
}

void webserver_tick() {
  can_dump_drain_tick();
  if (ota_active && ota_timeout_timer.elapsed()) {
    // OTA timeout
    set_event(EVENT_OTA_UPDATE_TIMEOUT, 0);
    on_ota_failure();
  }
}
