#include "settings_html.h"
#include <Arduino.h>
#include <string>
#include "../../../src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../../src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "LittleFS.h"
#include "html_escape.h"
#include "index_html.h"
#include "src/battery/BATTERIES.h"
#include "src/inverter/INVERTERS.h"

extern bool settingsUpdated;

// Helper function to read file content from LittleFS
String readFileFromLittleFS(const char* filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    return String("<!-- Error: Could not open file: ") + filename + " -->";
  }

  String content = file.readString();
  file.close();
  return content;
}

template <typename E>
constexpr auto to_underlying(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

template <typename EnumType>
std::vector<EnumType> enum_values() {
  static_assert(std::is_enum_v<EnumType>, "Template argument must be an enum type.");

  constexpr auto count = to_underlying(EnumType::Highest);
  std::vector<EnumType> values;
  for (int i = 1; i < count; ++i) {
    values.push_back(static_cast<EnumType>(i));
  }
  return values;
}

template <typename EnumType, typename Func>
std::vector<std::pair<String, EnumType>> enum_values_and_names(Func name_for_type,
                                                               const EnumType* noneValue = nullptr) {
  auto values = enum_values<EnumType>();

  std::vector<std::pair<String, EnumType>> pairs;

  for (auto& type : values) {
    auto name = name_for_type(type);
    if (name != nullptr) {
      pairs.push_back(std::pair(String(name), type));
    }
  }

  std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

  if (noneValue) {
    pairs.insert(pairs.begin(), std::pair(name_for_type(*noneValue), *noneValue));
  }

  return pairs;
}

template <typename TEnum, typename Func>
String options_for_enum_with_none(TEnum selected, Func name_for_type, TEnum noneValue) {
  String options;
  TEnum none = noneValue;
  auto values = enum_values_and_names<TEnum>(name_for_type, &none);
  for (const auto& [name, type] : values) {
    options +=
        ("<option value=\"" + String(static_cast<int>(type)) + "\"" + (selected == type ? " selected" : "") + ">");
    options += name;
    options += "</option>";
  }
  return options;
}

template <typename TEnum, typename Func>
String options_for_enum(TEnum selected, Func name_for_type) {
  String options;
  auto values = enum_values_and_names<TEnum>(name_for_type, nullptr);
  for (const auto& [name, type] : values) {
    if (name[0] == '\0')
      continue;  // Don't show blank options
    options +=
        ("<option value=\"" + String(static_cast<int>(type)) + "\"" + (selected == type ? " selected" : "") + ">");
    options += name;
    options += "</option>";
  }
  return options;
}

template <typename TMap>
String options_from_map(int selected, const TMap& value_name_map) {
  String options;
  for (const auto& [value, name] : value_name_map) {
    options += "<option value=\"" + String(value) + "\"";
    if (selected == value) {
      options += " selected";
    }
    options += ">";
    options += name;
    options += "</option>";
  }
  return options;
}

static const std::map<int, String> led_modes = {{0, "Classic"}, {1, "Energy Flow"}, {2, "Heartbeat"}};

static const std::map<int, String> tesla_countries = {
    {21843, "US (USA)"},     {17217, "CA (Canada)"},  {18242, "GB (UK & N Ireland)"},
    {17483, "DK (Denmark)"}, {17477, "DE (Germany)"}, {16725, "AU (Australia)"}};

static const std::map<int, String> tesla_mapregion = {
    {8, "ME (Middle East)"}, {2, "NONE"},       {3, "CN (China)"},     {6, "TW (Taiwan)"}, {5, "JP (Japan)"},
    {0, "US (USA)"},         {7, "KR (Korea)"}, {4, "AU (Australia)"}, {1, "EU (Europe)"}};

static const std::map<int, String> tesla_chassis = {{0, "Model S"}, {1, "Model X"}, {2, "Model 3"}, {3, "Model Y"}};

static const std::map<int, String> tesla_pack = {{0, "50 kWh"}, {2, "62 kWh"}, {1, "74 kWh"}, {3, "100 kWh"}};

const char* name_for_button_type(STOP_BUTTON_BEHAVIOR behavior) {
  switch (behavior) {
    case STOP_BUTTON_BEHAVIOR::LATCHING_SWITCH:
      return "Latching";
    case STOP_BUTTON_BEHAVIOR::MOMENTARY_SWITCH:
      return "Momentary";
    case STOP_BUTTON_BEHAVIOR::NOT_CONNECTED:
      return "Not connected";
    default:
      return nullptr;
  }
}

const char* name_for_gpioopt1(GPIOOPT1 option) {
  switch (option) {
    case GPIOOPT1::DEFAULT_OPT:
      return "WUP1 / WUP2";
    case GPIOOPT1::I2C_DISPLAY_SSD1306:
      return "I2C Display (SSD1306)";
    case GPIOOPT1::ESTOP_BMS_POWER:
      return "E-Stop / BMS Power";
    default:
      return nullptr;
  }
}

// Special unicode characters
const char* TRUE_CHAR_CODE = "\u2713";   //&#10003";
const char* FALSE_CHAR_CODE = "\u2715";  //&#10005";

String raw_settings_processor(const String& var, BatteryEmulatorSettingsStore& settings);

String settings_processor(const String& var, BatteryEmulatorSettingsStore& settings) {
  // HTML-ready values (such as select options) are returned here. These don't
  // get any additional escaping.

  if (var == "SHUNTCOMM") {
    return options_for_enum((comm_interface)settings.getUInt("SHUNTCOMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }

  if (var == "BATTTYPE") {
    return options_for_enum_with_none((BatteryType)settings.getUInt("BATTTYPE", (int)BatteryType::None),
                                      name_for_battery_type, BatteryType::None);
  }
  if (var == "BATTCOMM") {
    return options_for_enum((comm_interface)settings.getUInt("BATTCOMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }
  if (var == "BATTCHEM") {
    return options_for_enum(
        (battery_chemistry_enum)settings.getUInt("BATTCHEM", (int)battery_chemistry_enum::Autodetect),
        name_for_chemistry);
  }
  if (var == "INVTYPE") {
    return options_for_enum_with_none(
        (InverterProtocolType)settings.getUInt("INVTYPE", (int)InverterProtocolType::None), name_for_inverter_type,
        InverterProtocolType::None);
  }
  if (var == "INVCOMM") {
    return options_for_enum((comm_interface)settings.getUInt("INVCOMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }
  if (var == "CHGTYPE") {
    return options_for_enum_with_none((ChargerType)settings.getUInt("CHGTYPE", (int)ChargerType::None),
                                      name_for_charger_type, ChargerType::None);
  }
  if (var == "CHGCOMM") {
    return options_for_enum((comm_interface)settings.getUInt("CHGCOMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }

  if (var == "SHUNTTYPE") {
    return options_for_enum_with_none((ShuntType)settings.getUInt("SHUNTTYPE", (int)ShuntType::None),
                                      name_for_shunt_type, ShuntType::None);
  }

  if (var == "SHUNTCOMM") {
    return options_for_enum((comm_interface)settings.getUInt("SHUNTCOMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }

  if (var == "EQSTOP") {
    return options_for_enum_with_none(
        (STOP_BUTTON_BEHAVIOR)settings.getUInt("EQSTOP", (int)STOP_BUTTON_BEHAVIOR::NOT_CONNECTED),
        name_for_button_type, STOP_BUTTON_BEHAVIOR::NOT_CONNECTED);
  }

  if (var == "BATT2COMM") {
    return options_for_enum((comm_interface)settings.getUInt("BATT2COMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }

  if (var == "BATT3COMM") {
    return options_for_enum((comm_interface)settings.getUInt("BATT3COMM", (int)comm_interface::CanNative),
                            name_for_comm_interface);
  }

  if (var == "GTWCOUNTRY") {
    return options_from_map(settings.getUInt("GTWCOUNTRY", 0), tesla_countries);
  }

  if (var == "GTWMAPREG") {
    return options_from_map(settings.getUInt("GTWMAPREG", 0), tesla_mapregion);
  }

  if (var == "GTWCHASSIS") {
    return options_from_map(settings.getUInt("GTWCHASSIS", 0), tesla_chassis);
  }

  if (var == "GTWPACK") {
    return options_from_map(settings.getUInt("GTWPACK", 0), tesla_pack);
  }

  if (var == "LEDMODE") {
    return options_from_map(settings.getUInt("LEDMODE", 0), led_modes);
  }

  if (var == "GPIOOPT1") {
    return options_for_enum_with_none((GPIOOPT1)settings.getUInt("GPIOOPT1", (int)GPIOOPT1::DEFAULT_OPT),
                                      name_for_gpioopt1, GPIOOPT1::DEFAULT_OPT);
  }

  // All other values are wrapped by html_escape to avoid HTML injection.

  return html_escape(raw_settings_processor(var, settings));
}

String raw_settings_processor(const String& var, BatteryEmulatorSettingsStore& settings) {
  // All of these returned values are raw un-escaped UTF-8 strings.

  if (var == "HOSTNAME") {
    return settings.getString("HOSTNAME");
  }

  if (var == "BATTERYINTF") {
    if (battery) {
      return battery->interface_name();
    }
  }

  if (var == "SSID") {
    return settings.getString("SSID");
  }

  if (var == "PASSWORD") {
    return settings.getString("PASSWORD");
  }

  if (var == "SAVEDCLASS") {
    if (!settingsUpdated) {
      return "hidden";
    }
  }

  if (var == "BATTERY2CLASS") {
    if (!battery2) {
      return "hidden";
    }
  }

  if (var == "BATTERY2INTF") {
    if (battery2) {
      return battery2->interface_name();
    }
  }

  if (var == "INVCLASS") {
    if (!inverter) {
      return "hidden";
    }
  }

  if (var == "INVBIDCLASS") {
    if (!inverter || !inverter->supports_battery_id()) {
      return "hidden";
    }
  }

  if (var == "INVBID") {
    if (inverter && inverter->supports_battery_id()) {
      return String(datalayer.battery.settings.sofar_user_specified_battery_id);
    }
  }

  if (var == "INVINTF") {
    if (inverter) {
      return inverter->interface_name();
    }
  }

  if (var == "SHUNTCLASS") {
    if (user_selected_shunt_type == ShuntType::None) {
      return "hidden";
    }
  }

  if (var == "CHARGERCLASS") {
    if (!charger) {
      return "hidden";
    }
  }

  if (var == "DBLBTR") {
    return settings.getBool("DBLBTR") ? "checked" : "";
  }

  if (var == "TRIBTR") {
    return settings.getBool("TRIBTR") ? "checked" : "";
  }

  if (var == "SOCESTIMATED") {
    return settings.getBool("SOCESTIMATED") ? "checked" : "";
  }

  if (var == "CNTCTRL") {
    return settings.getBool("CNTCTRL") ? "checked" : "";
  }

  if (var == "NCCONTACTOR") {
    return settings.getBool("NCCONTACTOR") ? "checked" : "";
  }

  if (var == "CNTCTRLDBL") {
    return settings.getBool("CNTCTRLDBL") ? "checked" : "";
  }

  if (var == "CNTCTRLTRI") {
    return settings.getBool("CNTCTRLTRI") ? "checked" : "";
  }

  if (var == "PWMCNTCTRL") {
    return settings.getBool("PWMCNTCTRL") ? "checked" : "";
  }

  if (var == "PERBMSRESET") {
    return settings.getBool("PERBMSRESET") ? "checked" : "";
  }

  if (var == "REMBMSRESET") {
    return settings.getBool("REMBMSRESET") ? "checked" : "";
  }

  if (var == "EXTPRECHARGE") {
    return settings.getBool("EXTPRECHARGE") ? "checked" : "";
  }

  if (var == "MAXPRETIME") {
    return String(settings.getUInt("MAXPRETIME", 15000));
  }

  if (var == "NOINVDISC") {
    return settings.getBool("NOINVDISC") ? "checked" : "";
  }

  if (var == "CANFDASCAN") {
    return settings.getBool("CANFDASCAN") ? "checked" : "";
  }

  if (var == "WIFIAPENABLED") {
    return settings.getBool("WIFIAPENABLED", wifiap_enabled) ? "checked" : "";
  }

  if (var == "APPASSWORD") {
    return settings.getString("APPASSWORD", "123456789");
  }

  if (var == "APNAME") {
    return settings.getString("APNAME", "BatteryEmulator");
  }

  if (var == "STATICIP") {
    return settings.getBool("STATICIP") ? "checked" : "";
  }

  if (var == "WIFICHANNEL") {
    return String(settings.getUInt("WIFICHANNEL", 0));
  }

  if (var == "CHGPOWER") {
    return String(settings.getUInt("CHGPOWER", 0));
  }

  if (var == "DCHGPOWER") {
    return String(settings.getUInt("DCHGPOWER", 0));
  }

  if (var == "LOCALIP1") {
    return String(settings.getUInt("LOCALIP1", 0));
  }

  if (var == "LOCALIP2") {
    return String(settings.getUInt("LOCALIP2", 0));
  }

  if (var == "LOCALIP3") {
    return String(settings.getUInt("LOCALIP3", 0));
  }

  if (var == "LOCALIP4") {
    return String(settings.getUInt("LOCALIP4", 0));
  }

  if (var == "GATEWAY1") {
    return String(settings.getUInt("GATEWAY1", 0));
  }

  if (var == "GATEWAY2") {
    return String(settings.getUInt("GATEWAY2", 0));
  }

  if (var == "GATEWAY3") {
    return String(settings.getUInt("GATEWAY3", 0));
  }

  if (var == "GATEWAY4") {
    return String(settings.getUInt("GATEWAY4", 0));
  }

  if (var == "SUBNET1") {
    return String(settings.getUInt("SUBNET1", 0));
  }

  if (var == "SUBNET2") {
    return String(settings.getUInt("SUBNET2", 0));
  }

  if (var == "SUBNET3") {
    return String(settings.getUInt("SUBNET3", 0));
  }

  if (var == "SUBNET4") {
    return String(settings.getUInt("SUBNET4", 0));
  }

  if (var == "PERFPROFILE") {
    return settings.getBool("PERFPROFILE") ? "checked" : "";
  }

  if (var == "CANLOGUSB") {
    return settings.getBool("CANLOGUSB") ? "checked" : "";
  }

  if (var == "USBENABLED") {
    return settings.getBool("USBENABLED") ? "checked" : "";
  }

  if (var == "WEBENABLED") {
    return settings.getBool("WEBENABLED") ? "checked" : "";
  }

  if (var == "CANLOGSD") {
    return settings.getBool("CANLOGSD") ? "checked" : "";
  }

  if (var == "SDLOGENABLED") {
    return settings.getBool("SDLOGENABLED") ? "checked" : "";
  }

  if (var == "MQTTENABLED") {
    return settings.getBool("MQTTENABLED") ? "checked" : "";
  }

  if (var == "MQTTSERVER") {
    return settings.getString("MQTTSERVER");
  }

  if (var == "MQTTPORT") {
    return String(settings.getUInt("MQTTPORT", 1883));
  }

  if (var == "MQTTUSER") {
    return settings.getString("MQTTUSER");
  }

  if (var == "MQTTPASSWORD") {
    return settings.getString("MQTTPASSWORD");
  }

  if (var == "MQTTTOPICS") {
    return settings.getBool("MQTTTOPICS") ? "checked" : "";
  }

  if (var == "MQTTTOPIC") {
    return settings.getString("MQTTTOPIC");
  }

  if (var == "MQTTTIMEOUT") {
    return String(settings.getUInt("MQTTTIMEOUT", 2000));
  }

  if (var == "MQTTOBJIDPREFIX") {
    return settings.getString("MQTTOBJIDPREFIX");
  }

  if (var == "MQTTDEVICENAME") {
    return settings.getString("MQTTDEVICENAME");
  }

  if (var == "MQTTCELLV") {
    return settings.getBool("MQTTCELLV") ? "checked" : "";
  }

  if (var == "HADEVICEID") {
    return settings.getString("HADEVICEID");
  }

  if (var == "HADISC") {
    return settings.getBool("HADISC") ? "checked" : "";
  }

  if (var == "MANUAL_BAL_CLASS") {
    if (battery && battery->supports_manual_balancing()) {
      return "";
    } else {
      return "hidden";
    }
  }

  if (var == "BATTPVMAX") {
    return String(static_cast<float>(settings.getUInt("BATTPVMAX", 0)) / 10.0f, 1);
  }

  if (var == "BATTPVMIN") {
    return String(static_cast<float>(settings.getUInt("BATTPVMIN", 0)) / 10.0f, 1);
  }

  if (var == "BATTCVMAX") {
    return String(settings.getUInt("BATTCVMAX", 0));
  }

  if (var == "BATTCVMIN") {
    return String(settings.getUInt("BATTCVMIN", 0));
  }

  if (var == "BATTERY_WH_MAX") {
    return String(datalayer.battery.info.total_capacity_Wh);
  }

  if (var == "MAX_CHARGE_SPEED") {
    return String(datalayer.battery.settings.max_user_set_charge_dA / 10.0f, 1);
  }

  if (var == "MAX_DISCHARGE_SPEED") {
    return String(datalayer.battery.settings.max_user_set_discharge_dA / 10.0f, 1);
  }

  if (var == "SOC_MAX_PERCENTAGE") {
    return String(datalayer.battery.settings.max_percentage / 100.0f, 1);
  }

  if (var == "SOC_MIN_PERCENTAGE") {
    return String(datalayer.battery.settings.min_percentage / 100.0f, 1);
  }

  if (var == "CHARGE_VOLTAGE") {
    return String(datalayer.battery.settings.max_user_set_charge_voltage_dV / 10.0f, 1);
  }

  if (var == "DISCHARGE_VOLTAGE") {
    return String(datalayer.battery.settings.max_user_set_discharge_voltage_dV / 10.0f, 1);
  }

  if (var == "SOC_SCALING_ACTIVE_CLASS") {
    return datalayer.battery.settings.soc_scaling_active ? "active" : "inactive";
  }

  if (var == "VOLTAGE_LIMITS_ACTIVE_CLASS") {
    return datalayer.battery.settings.user_set_voltage_limits_active ? "active" : "inactive";
  }

  if (var == "SOC_SCALING_CLASS") {
    return datalayer.battery.settings.soc_scaling_active ? "active" : "inactiveSoc";
  }

  if (var == "SOC_SCALING") {
    return datalayer.battery.settings.soc_scaling_active ? TRUE_CHAR_CODE : FALSE_CHAR_CODE;
  }

  if (var == "FAKE_VOLTAGE_CLASS") {
    return battery && battery->supports_set_fake_voltage() ? "" : "hidden";
  }

  if (var == "MANUAL_BALANCING_CLASS") {
    return datalayer.battery.settings.user_requests_balancing ? "" : "inactiveSoc";
  }

  if (var == "MANUAL_BALANCING") {
    if (datalayer.battery.settings.user_requests_balancing) {
      return TRUE_CHAR_CODE;
    } else {
      return FALSE_CHAR_CODE;
    }
  }

  if (var == "BATTERY_VOLTAGE") {
    if (battery) {
      return String(battery->get_voltage(), 1);
    }
  }

  if (var == "VOLTAGE_LIMITS") {
    if (datalayer.battery.settings.user_set_voltage_limits_active) {
      return TRUE_CHAR_CODE;
    } else {
      return FALSE_CHAR_CODE;
    }
  }

  if (var == "BALANCING_CLASS") {
    return datalayer.battery.settings.user_requests_balancing ? "active" : "inactive";
  }

  if (var == "BALANCING_MAX_TIME") {
    return String(datalayer.battery.settings.balancing_time_ms / 60000.0f, 1);
  }

  if (var == "BAL_POWER") {
    return String(datalayer.battery.settings.balancing_float_power_W / 1.0f, 0);
  }

  if (var == "BAL_MAX_PACK_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_pack_voltage_dV / 10.0f, 0);
  }
  if (var == "BAL_MAX_CELL_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_cell_voltage_mV / 1.0f, 0);
  }
  if (var == "BAL_MAX_DEV_CELL_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV / 1.0f, 0);
  }

  if (var == "BMS_RESET_DURATION") {
    return String(datalayer.battery.settings.user_set_bms_reset_duration_ms / 1000.0f, 0);
  }

  if (var == "CHARGER_CLASS") {
    if (!charger) {
      return "hidden";
    }
  }

  if (var == "CHG_HV_CLASS") {
    if (datalayer.charger.charger_HV_enabled) {
      return "active";
    } else {
      return "inactiveSoc";
    }
  }

  if (var == "CHG_HV") {
    if (datalayer.charger.charger_HV_enabled) {
      return TRUE_CHAR_CODE;
    } else {
      return FALSE_CHAR_CODE;
    }
  }

  if (var == "CHG_AUX12V_CLASS") {
    if (datalayer.charger.charger_aux12V_enabled) {
      return "active";
    } else {
      return "inactiveSoc";
    }
  }

  if (var == "CHG_AUX12V") {
    if (datalayer.charger.charger_aux12V_enabled) {
      return TRUE_CHAR_CODE;
    } else {
      return FALSE_CHAR_CODE;
    }
  }

  if (var == "CHG_VOLTAGE_SETPOINT") {
    return String(datalayer.charger.charger_setpoint_HV_VDC, 1);
  }

  if (var == "CHG_CURRENT_SETPOINT") {
    return String(datalayer.charger.charger_setpoint_HV_IDC, 1);
  }

  if (var == "SOFAR_ID") {
    return String(settings.getUInt("SOFAR_ID", 0));
  }

  if (var == "PYLONSEND") {
    return String(settings.getUInt("PYLONSEND", 0));
  }

  if (var == "PYLONOFFSET") {
    return settings.getBool("PYLONOFFSET") ? "checked" : "";
  }

  if (var == "PYLONORDER") {
    return settings.getBool("PYLONORDER") ? "checked" : "";
  }

  if (var == "INVCELLS") {
    return String(settings.getUInt("INVCELLS", 0));
  }

  if (var == "INVMODULES") {
    return String(settings.getUInt("INVMODULES", 0));
  }

  if (var == "INVCELLSPER") {
    return String(settings.getUInt("INVCELLSPER", 0));
  }

  if (var == "INVVLEVEL") {
    return String(settings.getUInt("INVVLEVEL", 0));
  }

  if (var == "INVCAPACITY") {
    return String(settings.getUInt("INVCAPACITY", 0));
  }

  if (var == "INVBTYPE") {
    return String(settings.getUInt("INVBTYPE", 0));
  }

  if (var == "INVICNT") {
    return settings.getBool("INVICNT") ? "checked" : "";
  }

  if (var == "DEYEBYD") {
    return settings.getBool("DEYEBYD") ? "checked" : "";
  }

  if (var == "CANFREQ") {
    return String(settings.getUInt("CANFREQ", 8));
  }

  if (var == "CANFDFREQ") {
    return String(settings.getUInt("CANFDFREQ", 40));
  }

  if (var == "PRECHGMS") {
    return String(settings.getUInt("PRECHGMS", 100));
  }

  if (var == "PWMFREQ") {
    return String(settings.getUInt("PWMFREQ", 20000));
  }

  if (var == "PWMHOLD") {
    return String(settings.getUInt("PWMHOLD", 250));
  }

  if (var == "INTERLOCKREQ") {
    return settings.getBool("INTERLOCKREQ") ? "checked" : "";
  }

  if (var == "DIGITALHVIL") {
    return settings.getBool("DIGITALHVIL") ? "checked" : "";
  }

  if (var == "GTWRHD") {
    return settings.getBool("GTWRHD") ? "checked" : "";
  }

  return String();
}

const char* getCANInterfaceName(CAN_Interface interface) {
  switch (interface) {
    case CAN_NATIVE:
      return "CAN";
    case CANFD_NATIVE:
      if (use_canfd_as_can) {
        return "CAN-FD Native (Classic CAN)";
      } else {
        return "CAN-FD Native";
      }
    case CAN_ADDON_MCP2515:
      return "Add-on CAN via GPIO MCP2515";
    case CANFD_ADDON_MCP2518:
      if (use_canfd_as_can) {
        return "Add-on CAN-FD via GPIO MCP2518 (Classic CAN)";
      } else {
        return "Add-on CAN-FD via GPIO MCP2518";
      }
    default:
      return "UNKNOWN";
  }
}

// Runtime GPIO setting generation - replaces compile-time #ifdef
String getGpioOpt1Setting() {
#ifdef HW_LILYGO2CAN
  return R"rawliteral(
    <label for="GPIOOPT1">Configurable port:</label>
    <select id="GPIOOPT1" name="GPIOOPT1">
      %GPIOOPT1%
    </select>
  )rawliteral";
#else
  return "";
#endif
}

// Function to process conditional content in HTML files
// Use markers like /*#ifdef HW_LILYGO2CAN*/ and /*#endif*/ in your HTML files
// String processConditionalContent(const String& content) {
//   String result = content;

// #ifdef HW_LILYGO2CAN
//   // Keep HW_LILYGO2CAN sections, remove others
//   result.replace("/*#ifdef HW_LILYGO2CAN*/", "");

//   // Remove content between /*#ifndef HW_LILYGO2CAN*/ and /*#endif*/
//   int start = 0;
//   while ((start = result.indexOf("/*#ifndef HW_LILYGO2CAN*/", start)) != -1) {
//     int end = result.indexOf("/*#endif*/", start);
//     if (end != -1) {
//       result.remove(start, end - start + 10);
//     } else {
//       break;
//     }
//   }

//   result.replace("/*#endif*/", "");
// #else
//   // Keep default sections, remove HW_LILYGO2CAN sections
//   result.replace("/*#ifndef HW_LILYGO2CAN*/", "");

//   // Remove content between /*#ifdef HW_LILYGO2CAN*/ and /*#endif*/
//   int start = 0;
//   while ((start = result.indexOf("/*#ifdef HW_LILYGO2CAN*/", start)) != -1) {
//     int end = result.indexOf("/*#endif*/", start);
//     if (end != -1) {
//       result.remove(start, end - start + 10);
//     } else {
//       break;
//     }
//   }

//   result.replace("/*#endif*/", "");
// #endif

//   return result;
// }

String getSettingsHtmlScripts() {
  String content = readFileFromLittleFS("/settings_scripts.html");
  //return processConditionalContent(content);
  return content;
}

String getSettingsStyle() {
  String content = readFileFromLittleFS("/settings_style.html");
  //return processConditionalContent(content);
  return content;
}

String getSettingsHtmlBody() {
  String content = readFileFromLittleFS("/settings_body.html");

  // Process conditional content
  //content = processConditionalContent(content);

  // Replace the %GPIOOPT1_SETTING% placeholder with runtime-generated content
  content.replace("%GPIOOPT1_SETTING%", getGpioOpt1Setting());

  return content;
}

// Function to build complete settings HTML from LittleFS files
String buildSettingsHtml() {
  String html = INDEX_HTML_HEADER;
  html += COMMON_JAVASCRIPT;
  html += getSettingsStyle();
  html += getSettingsHtmlBody();
  html += getSettingsHtmlScripts();
  html += INDEX_HTML_FOOTER;

  return html;
}

// Legacy const char* for compatibility - will be built dynamically
String settings_html_string;
const char* settings_html = nullptr;
