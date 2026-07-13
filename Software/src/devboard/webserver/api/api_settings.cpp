// GET /api/settings - JSON mirror of the legacy settings page data
// (settings_html.cpp). Shape: SettingsResponse in frontend/src/types/settings.types.ts
// Saving still goes through the legacy POST /saveSettings handler.

#include <type_traits>
#include <utility>
#include <vector>
#include "../../../battery/BATTERIES.h"
#include "../../../battery/Battery.h"
#include "../../../battery/Shunt.h"
#include "../../../charger/CHARGERS.h"
#include "../../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../../communication/nvm/comm_nvm.h"
#include "../../../datalayer/datalayer.h"
#include "../../../inverter/INVERTERS.h"
#include "../../wifi/wifi.h"
#include "api_common.h"
#include "api_routes.h"

// Defined in settings_html.cpp
extern const char* name_for_button_type(STOP_BUTTON_BEHAVIOR behavior);

// NVM key lists. Kept in sync with boolSettingNames / uintSettingNames /
// stringSettingNames in webserver.cpp (duplicated here on purpose so the
// legacy file stays untouched during the migration).
struct UintSettingDef {
  const char* name;
  uint32_t default_value;
};

static const char* API_BOOL_SETTINGS[] = {
    "DBLBTR",      "CNTCTRL",       "CNTCTRLDBL",   "PWMCNTCTRL",    "PERBMSRESET", "SDLOGENABLED", "STATICIP",
    "REMBMSRESET", "EXTPRECHARGE",  "USBENABLED",   "CANLOGUSB",     "WEBENABLED",  "CANFDASCAN",   "CANFD2ASCAN",
    "CANLOGSD",    "WIFIAPENABLED", "MQTTENABLED",  "NOINVDISC",     "HADISC",      "MQTTCELLV",    "GTWRHD",
    "DIGITALHVIL", "PERFPROFILE",   "INTERLOCKREQ", "SOCESTIMATED",  "PYLONOFFSET", "PYLONORDER",   "DEYEBYD",
    "NCCONTACTOR", "TRIBTR",        "CNTCTRLTRI",   "ESPNOWENABLED", "PRIMOGEN24",  "CTINVERT",     "LOWPASSFILTER",
    "WEBAUTH",     "SLOWCANINV",
#ifndef SMALL_FLASH_DEVICE
    "SYSLOGEN",
#endif
};

// Defaults match raw_settings_processor() in settings_html.cpp
static const UintSettingDef API_UINT_SETTINGS[] = {
    {"BATTCVMAX", 0},    {"BATTCVMIN", 0},     {"MAXPRETIME", 15000}, {"MAXPREFREQ", 34000}, {"WIFICHANNEL", 0},
    {"DCHGPOWER", 0},    {"CHGPOWER", 0},      {"MQTTPORT", 1883},    {"MQTTTIMEOUT", 2000}, {"SOFAR_ID", 0},
    {"PYLONSEND", 0},    {"INVCELLS", 0},      {"INVMODULES", 0},     {"INVCELLSPER", 0},    {"INVVLEVEL", 0},
    {"INVCAPACITY", 0},  {"INVBTYPE", 0},      {"PRECHGMS", 100},     {"PWMFREQ", 20000},    {"PWMHOLD", 250},
    {"GTWCOUNTRY", 0},   {"GTWMAPREG", 0},     {"GTWCHASSIS", 0},     {"GTWPACK", 0},        {"LEDMODE", 0},
    {"GPIOOPT1", 0},     {"GPIOOPT2", 0},      {"GPIOOPT3", 0},       {"GPIOOPT4", 0},       {"INVSUNTYPE", 1},
    {"GPIOOPT5", 0},     {"GPIOOPT6", 0},      {"CTVNOM", 40},        {"CTANOM", 100},       {"PYLONBAUD", 500},
    {"PYLONBRAND", 0},   {"DALYPWRPCT", 50},   {"DALYPWRDV", 50},     {"DALYDVSTART", 20},   {"DALYPWRDEG", 60},
    {"DALYPWR0C", 800},  {"RAMPDOWNSOC", 9000}, {"INVICNT", 0},       {"FOXESSTYPE", 0},     {"FOXESSSUBTYPE", 0},
    {"FOXESSMODULES", 0},
#ifndef SMALL_FLASH_DEVICE
    {"SYSLOGPORT", 514}, {"SYSLOGFAC", 1},
#endif
    // Pack voltage limits, stored in tenths of a volt (4500 = 450.0 V)
    {"BATTPVMAX", 0},    {"BATTPVMIN", 0},
};

// Password-type keys are never exposed; the frontend sends blank = unchanged.
static const char* API_STRING_SETTINGS[] = {"HOSTNAME", "MQTTSERVER", "MQTTUSER", "HTTPUSER",
                                            "LOCALIP",  "GATEWAY",    "SUBNET",   "DNS",
#ifndef SMALL_FLASH_DEVICE
                                            "SYSLOGIP",
#endif
};

// ---------------------------------------------------------------------------
// Enum -> select option helpers (JSON flavor of the templates in settings_html.cpp).
// Implemented with a single non-template function to keep flash usage low.
// ---------------------------------------------------------------------------

typedef const char* (*NameForValueFn)(int value);

static void add_enum_options_impl(JsonArray arr, NameForValueFn name_for_value, int highest, int none_value) {
  std::vector<std::pair<const char*, int>> pairs;
  for (int i = 1; i < highest; ++i) {
    const char* name = name_for_value(i);
    if (name != nullptr && name[0] != '\0') {
      pairs.push_back({name, i});
    }
  }
  std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) { return strcmp(a.first, b.first) < 0; });
  if (none_value >= 0) {
    pairs.insert(pairs.begin(), {name_for_value(none_value), none_value});
  }
  for (const auto& [name, value] : pairs) {
    JsonObject o = arr.add<JsonObject>();
    o["value"] = value;
    o["label"] = name;
  }
}

// ---------------------------------------------------------------------------
// GET /api/settings
// ---------------------------------------------------------------------------

static void handle_api_settings(AsyncWebServerRequest* request) {
  BatteryEmulatorSettingsStore settings(true);
  JsonDocument doc;

  // Booleans
  JsonObject bools = doc["bool"].to<JsonObject>();
  for (const char* name : API_BOOL_SETTINGS) {
    const bool default_value = (String(name) == "WIFIAPENABLED") ? wifiap_enabled : false;
    bools[name] = settings.getBool(name, default_value);
  }
  bools["NEWWEBUI"] = new_webui_enabled();

  // Unsigned integers
  JsonObject uints = doc["uint"].to<JsonObject>();
  for (const auto& def : API_UINT_SETTINGS) {
    uints[def.name] = settings.getUInt(def.name, def.default_value);
  }

  // Strings (passwords intentionally omitted)
  JsonObject strings = doc["string"].to<JsonObject>();
  strings["SSID"] = settings.getString("SSID");
  for (const char* name : API_STRING_SETTINGS) {
    if (String(name) == "HTTPUSER") {
      strings[name] = settings.getString(name, "admin");
    } else {
      strings[name] = settings.getString(name);
    }
  }

  // Component type selections
  JsonObject select = doc["select"].to<JsonObject>();
  select["INVTYPE"] = settings.getUInt("INVTYPE", (int)InverterProtocolType::None);
  select["INVCOMM"] = settings.getUInt("INVCOMM", (int)comm_interface::CanNative);
  select["BATTTYPE"] = settings.getUInt("BATTTYPE", (int)BatteryType::None);
  select["BATTCHEM"] = settings.getUInt("BATTCHEM", (int)battery_chemistry_enum::Autodetect);
  select["BATTCOMM"] = settings.getUInt("BATTCOMM", (int)comm_interface::CanNative);
  select["CHGTYPE"] = settings.getUInt("CHGTYPE", (int)ChargerType::None);
  select["CHGCOMM"] = settings.getUInt("CHGCOMM", (int)comm_interface::CanNative);
  select["EQSTOP"] = settings.getUInt("EQSTOP", (int)STOP_BUTTON_BEHAVIOR::NOT_CONNECTED);
  select["BATT2COMM"] = settings.getUInt("BATT2COMM", (int)comm_interface::CanNative);
  select["BATT3COMM"] = settings.getUInt("BATT3COMM", (int)comm_interface::CanNative);
  select["SHUNTTYPE"] = settings.getUInt("SHUNTTYPE", (int)ShuntType::None);
  select["SHUNTCOMM"] = settings.getUInt("SHUNTCOMM", (int)comm_interface::CanNative);
  select["CTATTEN"] = settings.getUInt("CTATTEN", (int)adc_attenuation_enum::ADC_0db);

  // Options for the selects above
  JsonObject options = doc["select_options"].to<JsonObject>();
  add_enum_options_impl(
      options["battery"].to<JsonArray>(), [](int v) { return name_for_battery_type(static_cast<BatteryType>(v)); },
      (int)BatteryType::Highest, (int)BatteryType::None);
  add_enum_options_impl(
      options["inverter"].to<JsonArray>(),
      [](int v) { return name_for_inverter_type(static_cast<InverterProtocolType>(v)); },
      (int)InverterProtocolType::Highest, (int)InverterProtocolType::None);
  add_enum_options_impl(
      options["charger"].to<JsonArray>(), [](int v) { return name_for_charger_type(static_cast<ChargerType>(v)); },
      (int)ChargerType::Highest, (int)ChargerType::None);
  {
    // Autodetect (0) is not part of the 1..Highest range, add it manually
    JsonArray chem = options["chemistry"].to<JsonArray>();
    JsonObject autodetect = chem.add<JsonObject>();
    autodetect["value"] = (int)battery_chemistry_enum::Autodetect;
    autodetect["label"] = name_for_chemistry(battery_chemistry_enum::Autodetect);
    add_enum_options_impl(
        chem, [](int v) { return name_for_chemistry(static_cast<battery_chemistry_enum>(v)); },
        (int)battery_chemistry_enum::Highest, -1);
  }
  add_enum_options_impl(
      options["comm"].to<JsonArray>(), [](int v) { return name_for_comm_interface(static_cast<comm_interface>(v)); },
      (int)comm_interface::Highest, -1);
  add_enum_options_impl(
      options["shunt"].to<JsonArray>(), [](int v) { return name_for_shunt_type(static_cast<ShuntType>(v)); },
      (int)ShuntType::Highest, (int)ShuntType::None);
  add_enum_options_impl(
      options["eqstop"].to<JsonArray>(),
      [](int v) { return name_for_button_type(static_cast<STOP_BUTTON_BEHAVIOR>(v)); },
      (int)STOP_BUTTON_BEHAVIOR::Highest, (int)STOP_BUTTON_BEHAVIOR::NOT_CONNECTED);
  add_enum_options_impl(
      options["ctatten"].to<JsonArray>(),
      [](int v) { return name_for_adc_attenuation(static_cast<adc_attenuation_enum>(v)); },
      (int)adc_attenuation_enum::Highest, (int)adc_attenuation_enum::ADC_0db);

  // Extra strings (may hold negative numbers, stored as text)
  JsonObject string_extra = doc["string_extra"].to<JsonObject>();
  string_extra["CTOFFSET"] = settings.getString("CTOFFSET", "-1.0");

  doc["mqtt_publish_s"] = settings.getUInt("MQTTPUBLISHMS", 5000) / 1000;

  // Live datalayer values shown on the legacy settings page
  JsonObject dl = doc["datalayer"].to<JsonObject>();
  dl["total_capacity_Wh"] = datalayer.battery.info.total_capacity_Wh;
  dl["max_charge_A"] = datalayer.battery.settings.max_user_set_charge_dA / 10.0f;
  dl["max_discharge_A"] = datalayer.battery.settings.max_user_set_discharge_dA / 10.0f;
  dl["soc_max_pct"] = datalayer.battery.settings.max_percentage / 100.0f;
  dl["soc_min_pct"] = datalayer.battery.settings.min_percentage / 100.0f;
  dl["charge_voltage_V"] = datalayer.battery.settings.max_user_set_charge_voltage_dV / 10.0f;
  dl["discharge_voltage_V"] = datalayer.battery.settings.max_user_set_discharge_voltage_dV / 10.0f;
  dl["soc_scaling_active"] = datalayer.battery.settings.soc_scaling_active;
  dl["voltage_limits_active"] = datalayer.battery.settings.user_set_voltage_limits_active;
  dl["manual_balancing"] = datalayer.battery.settings.user_requests_balancing;
  dl["sofar_battery_id"] = datalayer.battery.settings.sofar_user_specified_battery_id;

  api_send_json(request, doc);
}

void init_api_settings_routes(AsyncWebServer& server) {
  def_route_with_auth("/api/settings", server, HTTP_GET, handle_api_settings);
}
