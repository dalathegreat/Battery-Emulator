// Battery JSON endpoints:
//   GET /api/battery/status      -> BatteryStatusResponse
//   GET /api/battery/cellmonitor -> CellMonitorResponse
//   GET /api/battery/extra       -> BatteryExtraResponse
//   GET /api/battery/commands    -> BatteryCommandsResponse
// Shapes: frontend/src/types/battery.types.ts / battery-extra.types.ts

#include "../../../battery/BATTERIES.h"
#include "../../../battery/Battery.h"
#include "../../../charger/CHARGERS.h"
#include "../../../datalayer/datalayer.h"
#include "../../../devboard/safety/safety.h"
#include "../../../devboard/utils/millis64.h"
#include "../advanced_battery_html.h"
#include "../webserver.h"
#include "api_common.h"
#include "api_routes.h"

// ---------------------------------------------------------------------------
// GET /api/battery/status
// ---------------------------------------------------------------------------

static void handle_api_battery_status(AsyncWebServerRequest* request) {
  JsonDocument doc;

  api_fill_pack_status(doc["battery"].to<JsonObject>(), datalayer.battery, battery != nullptr);
  api_fill_pack_status(doc["battery2"].to<JsonObject>(), datalayer.battery2, battery2 != nullptr);
  api_fill_pack_status(doc["battery3"].to<JsonObject>(), datalayer.battery3, battery3 != nullptr);

  JsonObject system = doc["system"].to<JsonObject>();
  system["bms_status"] = api_system_status_string(datalayer.system.status.system_status);
  system["pause_status"] = String(get_emulator_pause_status().c_str());
  system["emulator_status"] = api_emulator_status_string();
  system["cpu_temp_C"] = datalayer.system.info.CPU_temperature;
  system["uptime_s"] = static_cast<uint32_t>(millis64() / 1000ULL);
  system["free_heap"] = ESP.getFreeHeap();

  JsonObject chg = doc["charger"].to<JsonObject>();
  chg["hv_enabled"] = datalayer.charger.charger_HV_enabled;
  chg["aux12v_enabled"] = datalayer.charger.charger_aux12V_enabled;
  chg["setpoint_HV_V"] = datalayer.charger.charger_setpoint_HV_VDC;
  chg["setpoint_HV_A"] = datalayer.charger.charger_setpoint_HV_IDC;
  chg["stat_HV_V"] = datalayer.charger.charger_stat_HVvol;
  chg["stat_HV_A"] = datalayer.charger.charger_stat_HVcur;

  api_send_json(request, doc);
}

// ---------------------------------------------------------------------------
// GET /api/battery/cellmonitor
// ---------------------------------------------------------------------------

static void fill_cell_group(JsonObject o, const DATALAYER_BATTERY_TYPE& b, bool present) {
  o["present"] = present;
  o["number_of_cells"] = b.info.number_of_cells;
  JsonArray cells = o["cells"].to<JsonArray>();
  if (!present) {
    return;
  }
  for (uint16_t i = 0; i < b.info.number_of_cells && i < MAX_AMOUNT_CELLS; i++) {
    // Match the legacy cellmonitor page: skip cells not yet read
    if (b.status.cell_voltages_mV[i] == 0) {
      continue;
    }
    JsonObject cell = cells.add<JsonObject>();
    cell["voltage_mV"] = b.status.cell_voltages_mV[i];
    cell["balancing"] = b.status.cell_balancing_status[i];
  }
}

static void handle_api_battery_cellmonitor(AsyncWebServerRequest* request) {
  JsonDocument doc;
  JsonArray packs = doc["packs"].to<JsonArray>();
  fill_cell_group(packs.add<JsonObject>(), datalayer.battery, battery != nullptr);
  fill_cell_group(packs.add<JsonObject>(), datalayer.battery2, battery2 != nullptr);
  fill_cell_group(packs.add<JsonObject>(), datalayer.battery3, battery3 != nullptr);
  api_send_json(request, doc);
}

// ---------------------------------------------------------------------------
// GET /api/battery/extra
// ---------------------------------------------------------------------------

// Strips HTML tags from a string, converting a few common entities.
static String strip_html_tags(const String& in) {
  String out;
  out.reserve(in.length());
  bool in_tag = false;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '<') {
      in_tag = true;
    } else if (c == '>') {
      in_tag = false;
    } else if (!in_tag) {
      if (c == '&') {
        // Translate the entities used by the battery HTML renderers
        if (in.startsWith("&deg;", i)) {
          out += "\xC2\xB0";  // °
          i += 4;
          continue;
        }
        if (in.startsWith("&nbsp;", i)) {
          out += ' ';
          i += 5;
          continue;
        }
        if (in.startsWith("&percnt;", i)) {
          out += '%';
          i += 7;
          continue;
        }
        if (in.startsWith("&amp;", i)) {
          out += '&';
          i += 4;
          continue;
        }
      }
      out += c;
    }
  }
  out.trim();
  return out;
}

// Converts the battery-specific HTML from BatteryHtmlRenderer::get_status_html()
// into label/value rows. Each <h4>...</h4> becomes one row: "Label: value" is
// split at the first colon; heading-like lines (no colon) become "§ " section
// headers, which the frontend renders as section titles.
static void html_to_rows(const String& html, JsonArray rows) {
  int pos = 0;
  while (true) {
    int start = html.indexOf("<h4", pos);
    if (start < 0) {
      break;
    }
    int content_start = html.indexOf('>', start);
    if (content_start < 0) {
      break;
    }
    int end = html.indexOf("</h4>", content_start);
    if (end < 0) {
      break;
    }
    String text = strip_html_tags(html.substring(content_start + 1, end));
    pos = end + 5;
    if (text.length() == 0) {
      continue;
    }
    JsonObject row = rows.add<JsonObject>();
    int colon = text.indexOf(':');
    if (colon > 0 && colon < static_cast<int>(text.length()) - 1) {
      String label = text.substring(0, colon);
      String value = text.substring(colon + 1);
      label.trim();
      value.trim();
      row["label"] = label;
      row["value"] = value;
    } else if (colon == static_cast<int>(text.length()) - 1) {
      String label = text.substring(0, colon);
      label.trim();
      row["label"] = label;
      row["value"] = "";
    } else {
      row["label"] = "\xC2\xA7 " + text;  // "§ " section header
      row["value"] = "";
    }
  }
}

static void handle_api_battery_extra(AsyncWebServerRequest* request) {
  JsonDocument doc;
  JsonArray batteries = doc["batteries"].to<JsonArray>();

  Battery* batts[] = {battery, battery2};
  for (int i = 0; i < 2; i++) {
    if (!batts[i]) {
      continue;
    }
    JsonObject entry = batteries.add<JsonObject>();
    entry["index"] = i;
    JsonArray rows = entry["rows"].to<JsonArray>();
    html_to_rows(batts[i]->get_status_renderer().get_status_html(), rows);
  }
  if (battery3) {
    JsonObject entry = batteries.add<JsonObject>();
    entry["index"] = 2;
    JsonArray rows = entry["rows"].to<JsonArray>();
    JsonObject row = rows.add<JsonObject>();
    row["label"] = "Note";
    row["value"] = "Advanced detailed info is currently limited to the Main Battery.";
  }

  api_send_json(request, doc);
}

// ---------------------------------------------------------------------------
// GET /api/battery/commands
// ---------------------------------------------------------------------------

static void handle_api_battery_commands(AsyncWebServerRequest* request) {
  JsonDocument doc;
  JsonArray commands = doc["commands"].to<JsonArray>();

  Battery* batts[] = {battery, battery2, battery3};
  for (const auto& cmd : battery_commands) {
    JsonArray applicable;
    JsonObject entry;
    bool added = false;
    for (int i = 0; i < 3; i++) {
      if (batts[i] && cmd.condition(batts[i])) {
        if (!added) {
          entry = commands.add<JsonObject>();
          entry["id"] = cmd.identifier;
          entry["label"] = cmd.title;
          if (cmd.prompt) {
            // Match the legacy confirm phrasing built in advanced_battery_html.cpp
            entry["confirm"] = String("Are you sure you want to ") + cmd.prompt;
          } else {
            entry["confirm"] = nullptr;
          }
          applicable = entry["batteries"].to<JsonArray>();
          added = true;
        }
        applicable.add(i);
      }
    }
  }

  api_send_json(request, doc);
}

void init_api_battery_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/battery/status", server, HTTP_GET, handle_api_battery_status);
  def_route_with_auth("/api/battery/cellmonitor", server, HTTP_GET, handle_api_battery_cellmonitor);
  def_route_with_auth("/api/battery/extra", server, HTTP_GET, handle_api_battery_extra);
  def_route_with_auth("/api/battery/commands", server, HTTP_GET, handle_api_battery_commands);
}
