#include "settings_html.h"
#include <Arduino.h>
#include "../../../src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../../src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "html_escape.h"
#include "index_html.h"
#include "src/battery/BATTERIES.h"
#include "src/inverter/INVERTERS.h"

extern bool settingsUpdated;

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
    if (!shunt) {
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

  if (var == "SOCESTIMATED") {
    return settings.getBool("SOCESTIMATED") ? "checked" : "";
  }

  if (var == "CNTCTRL") {
    return settings.getBool("CNTCTRL") ? "checked" : "";
  }

  if (var == "CNTCTRLDBL") {
    return settings.getBool("CNTCTRLDBL") ? "checked" : "";
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
    return String(static_cast<float>(settings.getUInt("BATTPVMAX", 0)) / 10.0, 1);
  }

  if (var == "BATTPVMIN") {
    return String(static_cast<float>(settings.getUInt("BATTPVMIN", 0)) / 10.0, 1);
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
    return String(datalayer.battery.settings.max_user_set_charge_dA / 10.0, 1);
  }

  if (var == "MAX_DISCHARGE_SPEED") {
    return String(datalayer.battery.settings.max_user_set_discharge_dA / 10.0, 1);
  }

  if (var == "SOC_MAX_PERCENTAGE") {
    return String(datalayer.battery.settings.max_percentage / 100.0, 1);
  }

  if (var == "SOC_MIN_PERCENTAGE") {
    return String(datalayer.battery.settings.min_percentage / 100.0, 1);
  }

  if (var == "CHARGE_VOLTAGE") {
    return String(datalayer.battery.settings.max_user_set_charge_voltage_dV / 10.0, 1);
  }

  if (var == "DISCHARGE_VOLTAGE") {
    return String(datalayer.battery.settings.max_user_set_discharge_voltage_dV / 10.0, 1);
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
    return String(datalayer.battery.settings.balancing_time_ms / 60000.0, 1);
  }

  if (var == "BAL_POWER") {
    return String(datalayer.battery.settings.balancing_float_power_W / 1.0, 0);
  }

  if (var == "BAL_MAX_PACK_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_pack_voltage_dV / 10.0, 0);
  }
  if (var == "BAL_MAX_CELL_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_cell_voltage_mV / 1.0, 0);
  }
  if (var == "BAL_MAX_DEV_CELL_VOLTAGE") {
    return String(datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV / 1.0, 0);
  }

  if (var == "BMS_RESET_DURATION") {
    return String(datalayer.battery.settings.user_set_bms_reset_duration_ms / 1000.0, 0);
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

#define SETTINGS_HTML_SCRIPTS \
  R"rawliteral(
    <script>

    function askFactoryReset() {
      if (confirm('Are you sure you want to reset the device to factory settings? This will erase all settings and data.')) {
        var xhr = new XMLHttpRequest();
        xhr.onload = function() {
          if (this.status == 200) {
            alert('Factory reset successful. The device will now restart.');
            reboot();
          } else {
            alert('Factory reset failed. Please try again.');
          }
        };
        xhr.onerror = function() {
          alert('An error occurred while trying to reset the device.');
        };
        xhr.open('POST', '/factoryReset', true);
        xhr.send();
      }
    }

    function editComplete(){if(this.status==200){window.location.reload();}}

    function editError(){alert('Invalid input');}

        function editWh(){var value=prompt('How much energy the battery can store. Enter new Wh value (1-400000):');
          if(value!==null){if(value>=1&&value<=400000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateBatterySize?value='+value,true);xhr.send();}else{
          alert('Invalid value. Please enter a value between 1 and 400000.');}}}

        function editUseScaledSOC(){var value=prompt('Extends battery life by rescaling the SOC within the configured minimum and maximum percentage. Should SOC scaling be applied? (0 = No, 1 = Yes):');
          if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateUseScaledSOC?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1.');}}}
    
        function editSocMax(){var value=prompt('Inverter will see fully charged (100pct)SOC when this value is reached. Enter new maximum SOC value that battery will charge to (50.0-100.0):');if(value!==null){if(value>=50&&value<=100){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateSocMax?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 50.0 and 100.0');}}}
    
        function editSocMin(){
          var value=prompt('Inverter will see completely discharged (0pct)SOC when this value is reached. Advanced users can set to negative values. Enter new minimum SOC value that battery will discharge to (-10.0to50.0):');
          if(value!==null){if(value>=-10&&value<=50){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateSocMin?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between -10 and 50.0');}}}
    
        function editMaxChargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new maximum charge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxChargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editMaxDischargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new maximum discharge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxDischargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editUseVoltageLimit(){var value=prompt('Enable this option to manually restrict charge/discharge to a specific voltage set below. If disabled the emulator automatically determines this based on battery limits. Restrict manually? (0 = No, 1 = Yes):');if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateUseVoltageLimit?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1.');}}}
    
        function editMaxChargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter new voltage setpoint batttery should charge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var 
        xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxChargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editMaxDischargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter new voltage setpoint batttery should discharge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var 
        xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxDischargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}

        function editBMSresetDuration(){var value=prompt('Amount of seconds BMS power should be off during periodic daily resets. Requires "Periodic BMS reset" to be enabled. Enter value in seconds (1-59):');if(value!==null){if(value>=1&&value<=59){var 
        xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateBMSresetDuration?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 1 and 59');}}}

        function editTeslaBalAct(){var value=prompt('Enable or disable forced LFP balancing. Makes the battery charge to 101percent. This should be performed once every month, to keep LFP batteries balanced. Ensure battery is fully charged before enabling, and also that you have enough sun or grid power to feed power into the battery while balancing is active. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/TeslaBalAct?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}
    
        function editBalTime(){var value=prompt('Enter new max balancing time in minutes');if(value!==null){if(value>=1&&value<=300){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalTime?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 1 and 300');}}}
    
        function editBalFloatPower(){var value=prompt('Power level in Watt to float charge during forced balancing');if(value!==null){if(value>=100&&value<=2000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalFloatPower?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 100 and 2000');}}}
    
        function editBalMaxPackV(){var value=prompt('Battery pack max voltage temporarily raised to this value during forced balancing. Value in V');if(value!==null){if(value>=380&&value<=410){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxPackV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 380 and 410');}}}

        function editBalMaxCellV(){var value=prompt('Cellvoltage max temporarily raised to this value during forced balancing. Value in mV');if(value!==null){if(value>=3400&&value<=3750){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 3400 and 3750');}}}
    
        function editBalMaxDevCellV(){var value=prompt('Cellvoltage max deviation temporarily raised to this value during forced balancing. Value in mV');if(value!==null){if(value>=300&&value<=600){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxDevCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 300 and 600');}}}

          function editFakeBatteryVoltage(){var value=prompt('Enter new fake battery voltage');if(value!==null){if(value>=0&&value<=5000){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateFakeBatteryVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000');}}}

          function editChargerHVDCEnabled(){var value=prompt('Enable or disable HV DC output. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargerHvEnabled?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}

          function editChargerAux12vEnabled(){var value=prompt('Enable or disable low voltage 12v auxiliary DC output. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargerAux12vEnabled?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter 1 or 0');}}}

          function editChargerSetpointVDC(){var value=prompt('Set charging voltage. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 200 to 420.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeSetpointV?value='+value,true);xhr.send();}else{
            alert('Invalid value. Please enter a value between 0 and 1000');}}}

          function editChargerSetpointIDC(){var value=prompt('Set charging amperage. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 6 to 48.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new           XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeSetpointA?value='+value,true);xhr.send();}else{
              alert('Invalid value. Please enter a value between 0 and 100');}}}

          function editChargerSetpointEndI(){
            var value=prompt('Set amperage that terminates charge as being sufficiently complete. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 1-5.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeEndA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 100');}}}

          function goToMainPage() { window.location.href = '/'; }

          document.querySelectorAll('select,input').forEach(function(sel) {
            function ch() {
              sel.closest('form').setAttribute('data-' + sel.name?.toLowerCase(), sel.type=='checkbox'?sel.checked:sel.value);
            }
            sel.addEventListener('change', ch);
            ch();
          });
    </script>
)rawliteral"

#define SETTINGS_STYLE \
  R"rawliteral(
    <style>
    body { background-color: black; color: white; }
        button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px;
        cursor: pointer; border-radius: 10px; }
    button:hover { background-color: #3A4A52; }
    h4 { margin: 0.6em 0; line-height: 1.2; }
    select, input { max-width: 250px; box-sizing: border-box; }
    .hidden {
      display: none;
    }
    .active {
      color: white;
    }
    .inactive {
      color: darkgrey;
    }

    .inactiveSoc {
      color: red;
    }

    .mqtt-settings, .mqtt-topics {
      display: none;
      grid-column: span 2;
    }

    .settings-card {
    background-color: #3a4b54; /* Slightly lighter than main background */
    padding: 15px 20px;
    margin-bottom: 20px;
    border-radius: 20px; /* Less rounded than 50px for a more card-like feel */
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
  }
  .settings-card h3 {
    color: #fff;
    margin-top: 0;
    margin-bottom: 15px;
    padding-bottom: 8px;
    border-bottom: 1px solid #4d5f69;
  }

    form .if-battery, form .if-inverter, form .if-charger, form .if-shunt { display: contents; }
    form[data-battery="0"] .if-battery { display: none; }
    form[data-inverter="0"] .if-inverter { display: none; }    
    form[data-charger="0"] .if-charger { display: none; }
    form[data-shunt="0"] .if-shunt { display: none; }

    form .if-cbms { display: none; }
    form[data-battery="6"] .if-cbms, form[data-battery="11"] .if-cbms, form[data-battery="22"] .if-cbms, form[data-battery="23"] .if-cbms, form[data-battery="24"] .if-cbms, form[data-battery="31"] .if-cbms {
      display: contents;
    }

    form .if-nissan { display: none; }
    form[data-battery="21"] .if-nissan {
      display: contents;
    }

    form .if-tesla { display: none; }
    form[data-battery="32"] .if-tesla, form[data-battery="33"] .if-tesla {
      display: contents;
    }

    form .if-estimated { display: none; } /* Integrations with manually set charge/discharge power */
    form[data-battery="3"] .if-estimated, 
    form[data-battery="4"] .if-estimated, 
    form[data-battery="6"] .if-estimated, 
    form[data-battery="14"] .if-estimated, 
    form[data-battery="16"] .if-estimated, 
    form[data-battery="24"] .if-estimated,
    form[data-battery="32"] .if-estimated, 
    form[data-battery="33"] .if-estimated {
      display: contents;
    }

    form .if-socestimated { display: none; } /* Integrations where you can turn on SOC estimation */
    form[data-battery="16"] .if-socestimated {
      display: contents;
    }

    form .if-dblbtr { display: none; }
    form[data-dblbtr="true"] .if-dblbtr {
      display: contents;
    }

    form .if-pwmcntctrl { display: none; }
    form[data-pwmcntctrl="true"] .if-pwmcntctrl {
      display: contents;
    }

    form .if-cntctrl { display: none; }
    form[data-cntctrl="true"] .if-cntctrl {
      display: contents;
    }

    form .if-extprecharge { display: none; }
    form[data-extprecharge="true"] .if-extprecharge {
      display: contents;
    }

    form .if-sofar { display: none; }
    form[data-inverter="17"] .if-sofar {
      display: contents;
    }

    form .if-byd { display: none; }
    form[data-inverter="2"] .if-byd {
      display: contents;
    }

    form .if-pylon { display: none; }
    form[data-inverter="10"] .if-pylon {
      display: contents;
    }

    form .if-pylonish { display: none; }
    form[data-inverter="4"] .if-pylonish, form[data-inverter="10"] .if-pylonish, form[data-inverter="19"] .if-pylonish {
      display: contents;
    }

    form .if-solax { display: none; }
    form[data-inverter="18"] .if-solax {
      display: contents;
    }

    form .if-kostal { display: none; }
    form[data-inverter="9"] .if-kostal {
      display: contents;
    }

    form .if-staticip { display: none; }
    form[data-staticip="true"] .if-staticip {
      display: contents;
    }

    form .if-mqtt { display: none; }
    form[data-mqttenabled="true"] .if-mqtt {
      display: contents;
    }

    form .if-topics { display: none; }
    form[data-mqtttopics="true"] .if-topics {
      display: contents;
    }

    </style>
)rawliteral"

#define SETTINGS_HTML_BODY \
  R"rawliteral(
  <button onclick='goToMainPage()'>Back to main page</button>
  <button onclick="askFactoryReset()">Factory reset</button>

<div style='background-color: #404E47; padding: 10px; margin-bottom: 10px; border-radius: 50px'>
        <form action='saveSettings' method='post'>

        <div style='grid-column: span 2; text-align: center; padding-top: 10px;' class="%SAVEDCLASS%">
          <p>Settings saved. Reboot to take the new settings into use.<p> <button type='button' onclick='askReboot()'>Reboot</button>
        </div>

        <div class="settings-card">
        <h3>Network config</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>SSID: </label>
        <input type='text' name='SSID' value="%SSID%" 
        pattern="[ -~]{1,63}" 
        title="Max 63 characters, printable ASCII only"/>

        <label>Password: </label><input type='password' name='PASSWORD' value="%PASSWORD%" 
        pattern="[ -~]{8,63}" 
        title="Password must be 8-63 characters long, printable ASCII only" />
        </div>
        </div>

        <div class="settings-card">
        <h3>Battery config</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label for='battery'>Battery: </label>
        <select name='battery' id='battery'>
            %BATTTYPE%
        </select>

        <div class="if-nissan">
            <label for='interlock'>Interlock required: </label>
            <input type='checkbox' name='INTERLOCKREQ' id='interlock' value='on' %INTERLOCKREQ% />
        </div>

        <div class="if-tesla">
          <label for='digitalhvil'>Digital HVIL (2024+): </label>
          <input type='checkbox' name='DIGITALHVIL' id='digitalhvil' value='on' %DIGITALHVIL% />
          <label>Right hand drive: </label>
          <input type='checkbox' name='GTWRHD' value='on' %GTWRHD% />
          <label for='GTWCOUNTRY'>Country code: </label><select name='GTWCOUNTRY' id='GTWCOUNTRY'>
          %GTWCOUNTRY%
          </select>
          <label for='GTWMAPREG'>Map region: </label><select name='GTWMAPREG' id='GTWMAPREG'>
          %GTWMAPREG%
          </select>
          <label for='GTWCHASSIS'>Chassis type: </label><select name='GTWCHASSIS' id='GTWCHASSIS'>
          %GTWCHASSIS%
          </select>
          <label for='GTWPACK'>Pack type: </label><select name='GTWPACK' id='GTWPACK'>
          %GTWPACK%
          </select>
        </div>

        <div class="if-estimated">
        <label>Manual charging power, watt: </label>
        <input type='number' name='CHGPOWER' value="%CHGPOWER%" 
        min="0" max="65000" step="1"
        title="Continous max charge power. Used since CAN data not valid for this integration. Do not set too high!" />

        <label>Manual discharge power, watt: </label>
        <input type='number' name='DCHGPOWER' value="%DCHGPOWER%" 
        min="0" max="65000" step="1"
        title="Continous max discharge power. Used since CAN data not valid for this integration. Do not set too high!" />
        </div>

        <div class="if-socestimated">
        <label>Use estimated SOC: </label>
        <input type='checkbox' name='SOCESTIMATED' value='on' %SOCESTIMATED% 
        title="Switch to estimated State of Charge when accurate SOC data is not available from the battery" />
        </div>

        <div class="if-battery">
        <label for='BATTCOMM'>Battery interface: </label><select name='BATTCOMM' id='BATTCOMM'>
        %BATTCOMM%
        </select>

        <label>Battery chemistry: </label><select name='BATTCHEM'>
        %BATTCHEM%
        </select>
        </div>

        <div class="if-cbms">
        <label>Battery max design voltage (V): </label>
        <input name='BATTPVMAX' pattern="[0-9]+(\.[0-9]+)?" type='text' value='%BATTPVMAX%'   
        title="Maximum safe voltage for the entire battery pack in volts. Used as charge target and protection limits." />

        <label>Battery min design voltage (V): </label>
        <input name='BATTPVMIN' pattern="[0-9]+(\.[0-9]+)?" type='text' value='%BATTPVMIN%' 
        title="Minimum safe voltage for the entire battery pack in volts. Further discharge not possible below this limit." />

        <label>Cell max design voltage (mV): </label>
        <input name='BATTCVMAX' pattern="[0-9]+" type='text' value='%BATTCVMAX%' 
        title="Maximum voltage per individual cell in millivolts. Charging stops if one cell reaches this voltage." />

        <label>Cell min design voltage (mV): </label>
        <input name='BATTCVMIN' pattern="[0-9]+$" type='text' value='%BATTCVMIN%' 
        title="Minimum voltage per individual cell in millivolts. Discharge stops if one cell drops to this voltage." />
        </div>

        <label>Double battery: </label>
        <input type='checkbox' name='DBLBTR' value='on' %DBLBTR% 
        title="Enable this option if you intend to run two batteries in parallel" />

        <div class="if-dblbtr">
            <label>Battery 2 interface: </label>
            <select name='BATT2COMM'>
                %BATT2COMM%
            </select>
        </div>

        </div>
        </div>

        <div class="settings-card">
      <h3>Inverter config</h3>
      <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>Inverter protocol: </label><select name='inverter'>
        %INVTYPE%
        </select>

        <div class="if-inverter">        
        <label>Inverter interface: </label><select name='INVCOMM'>
        %INVCOMM%     
        </select>
        </div>

        <div class="if-sofar">
        <label>Sofar Battery ID (0-15): </label>
        <input name='SOFAR_ID' type='text' value="%SOFAR_ID%" pattern="[0-9]{1,2}" />
        </div>

        <div class="if-pylon">
        <label>Pylon, send group (0-1): </label>
        <input name='PYLONSEND' type='text' value="%PYLONSEND%" pattern="[0-9]+" 
        title="Select if we should send ###0 or ###1 CAN messages, useful for multi-battery setups or ID problems" />

        <label>Pylon, 30k offset: </label>
        <input type='checkbox' name='PYLONOFFSET' value='on' %PYLONOFFSET% 
        title="When enabled, 30k offset will be applied on some signals, useful for some inverters that see wrong data otherwise" />

        <label>Pylon, invert byteorder: </label>
        <input type='checkbox' name='PYLONORDER' value='on' %PYLONORDER% 
        title="When enabled, byteorder will be inverted on some signals, useful for some inverters that see wrong data otherwise" />
        </div>

        <div class="if-byd">
        <label>Deye offgrid specific fixes: </label>
        <input type='checkbox' name='DEYEBYD' value='on' %DEYEBYD% />
        </div>

        <div class="if-pylonish">
        <label>Reported cell count (0 for default): </label>
        <input name='INVCELLS' type='text' value="%INVCELLS%" pattern="[0-9]+" />
        </div>

        <div class="if-pylonish if-solax">
        <label>Reported module count (0 for default): </label>
        <input name='INVMODULES' type='text' value="%INVMODULES%" pattern="[0-9]+" />
        </div>

        <div class="if-pylonish">
        <label>Reported cells per module (0 for default): </label>
        <input name='INVCELLSPER' type='text' value="%INVCELLSPER%" pattern="[0-9]+" />

        <label>Reported voltage level (0 for default): </label>
        <input name='INVVLEVEL' type='text' value="%INVVLEVEL%" pattern="[0-9]+" />

        <label>Reported Ah capacity (0 for default): </label>
        <input name='INVCAPACITY' type='text' value="%INVCAPACITY%" pattern="[0-9]+" />
        </div>

        <div class="if-solax">
        <label>Reported battery type (in decimal): </label>
        <input name='INVBTYPE' type='text' value="%INVBTYPE%" pattern="[0-9]+" />
        </div>

        <div class="if-kostal if-solax">
        <label>Prevent inverter opening contactors: </label>
        <input type='checkbox' name='INVICNT' value='on' %INVICNT% />
        </div>

        </div>
        </div>

        <div class="settings-card">
        <h3>Optional components config</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>Charger: </label><select name='charger'>
        %CHGTYPE%
        </select>

        <div class="if-charger">
        <label>Charger interface: </label><select name='CHGCOMM'>
        %CHGCOMM%
        </select>
        </div>

        <label>Shunt: </label><select name='SHUNT'>
        %SHUNTTYPE%
        </select>

        <div class="if-shunt">
        <label>Shunt interface: </label><select name='SHUNTCOMM'>
        %SHUNTCOMM%
        </select>
        </div>

        </div>
        </div>

        <div class="settings-card">
        <h3>Hardware config</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>Use CanFD as classic CAN: </label>
        <input type='checkbox' name='CANFDASCAN' value='on' %CANFDASCAN% 
        title="When enabled, CAN-FD channel will operate as normal 500kbps CAN" />

        <label>CAN addon crystal (Mhz): </label>
        <input type='number' name='CANFREQ' value="%CANFREQ%" 
        min="0" max="1000" step="1"
        title="Configure this if you are using a custom add-on CAN board. Integers only" />

        <label>CAN-FD-addon crystal (Mhz): </label>
        <input type='number' name='CANFDFREQ' value="%CANFDFREQ%" 
        min="0" max="1000" step="1"
        title="Configure this if you are using a custom add-on CAN board. Integers only" />
        
        <label>Equipment stop button: </label><select name='EQSTOP'>
        %EQSTOP%  
        </select>

        <div class="if-dblbtr">
            <label>Double-Battery Contactor control via GPIO: </label>
            <input type='checkbox' name='CNTCTRLDBL' value='on' %CNTCTRLDBL% />
        </div>

        <label>Contactor control via GPIO: </label>
        <input type='checkbox' name='CNTCTRL' value='on' %CNTCTRL% />

        <div class="if-cntctrl">
            <label>Precharge time ms: </label>
            <input type='number' name='PRECHGMS' value="%PRECHGMS%" 
            min="1" max="65000" step="1"
            title="Time in milliseconds the precharge should be active" />

            <label>PWM contactor control: </label>
            <input type='checkbox' name='PWMCNTCTRL' value='on' %PWMCNTCTRL% />

             <div class="if-pwmcntctrl">
            <label>PWM Frequency Hz: </label>
            <input name='PWMFREQ' type='text' value="%PWMFREQ%"             
            min="1" max="65000" step="1"
            title="Frequency in Hz used for PWM" />

            <label>PWM Hold 1-1023: </label>
            <input type='number' name='PWMHOLD' value="%PWMHOLD%" 
            min="1" max="1023" step="1"
            title="1-1023 , lower value = lower power consumption" />
              </div>

        </div>

        <label>Periodic BMS reset every 24h: </label>
        <input type='checkbox' name='PERBMSRESET' value='on' %PERBMSRESET% /> 

        <label>External precharge via HIA4V1: </label>
        <input type='checkbox' name='EXTPRECHARGE' value='on' %EXTPRECHARGE% />

        <div class="if-extprecharge">
            <label>Precharge, maximum ms before fault: </label>
            <input name='MAXPRETIME' type='text' value="%MAXPRETIME%" pattern="[0-9]+" />

          <label>Normally Open (NO) inverter disconnect contactor: </label>
          <input type='checkbox' name='NOINVDISC' value='on' %NOINVDISC% />
        </div>

        <label for='LEDMODE'>Status LED pattern: </label><select name='LEDMODE' id='LEDMODE'>
        %LEDMODE%
        </select>

        </div>
        </div>

        <div class="settings-card">
        <h3>Connectivity settings</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>Broadcast Wifi access point: </label>
        <input type='checkbox' name='WIFIAPENABLED' value='on' %WIFIAPENABLED% />

        <label>Access point name: </label>
        <input type='text' name='APNAME' value="%APNAME%" 
        pattern="[ -~]{1,63}" 
        title="Max 63 characters, printable ASCII only"
        required />

        <label>Access point password: </label>
        <input type='text' name='APPASSWORD' value="%APPASSWORD%" 
        pattern="[ -~]{8,63}" 
        title="Password must be 8-63 characters long, printable ASCII only"
        required />

        <label>Wifi channel 0-14: </label>
        <input type='number' name='WIFICHANNEL' value="%WIFICHANNEL%" 
        min="0" max="14" step="1"
        title="Force specific channel. Set to 0 for autodetect" required />

        <label>Custom Wifi hostname: </label>
        <input type='text' name='HOSTNAME' value="%HOSTNAME%" 
        pattern="[A-Za-z0-9\-]+"
        title="Optional: Hostname may only contain letters, numbers and '-'" />

        <label>Use static IP address: </label>
        <input type='checkbox' name='STATICIP' value='on' %STATICIP% />

        <div class='if-staticip'>
        <div>
          <div>Local IP:</div>
          <input type="number" name="LOCALIP1" min="0" max="255" size="3" value="%LOCALIP1%">.
          <input type="number" name="LOCALIP2" min="0" max="255" size="3" value="%LOCALIP2%">.
          <input type="number" name="LOCALIP3" min="0" max="255" size="3" value="%LOCALIP3%">.
          <input type="number" name="LOCALIP4" min="0" max="255" size="3" value="%LOCALIP4%">
        </div>
            
        <div>
            <div>Gateway:</div>
            <input type="number" name="GATEWAY1" min="0" max="255" size="3" value="%GATEWAY1%">.
            <input type="number" name="GATEWAY2" min="0" max="255" size="3" value="%GATEWAY2%">.
            <input type="number" name="GATEWAY3" min="0" max="255" size="3" value="%GATEWAY3%">.
            <input type="number" name="GATEWAY4" min="0" max="255" size="3" value="%GATEWAY4%">
        </div>
    
        <div>
          <div>Subnet:</div>
          <input type="number" name="SUBNET1" min="0" max="255" size="3" value="%SUBNET1%">.
          <input type="number" name="SUBNET2" min="0" max="255" size="3" value="%SUBNET2%">.
          <input type="number" name="SUBNET3" min="0" max="255" size="3" value="%SUBNET3%">.
          <input type="number" name="SUBNET4" min="0" max="255" size="3" value="%SUBNET4%">
        </div>
        <div></div>
        </div>

        <label>Enable MQTT: </label>
        <input type='checkbox' name='MQTTENABLED' value='on' %MQTTENABLED% />

        <div class='if-mqtt'>
        <label>MQTT server: </label>
        <input type='text' name='MQTTSERVER' value="%MQTTSERVER%" 
        pattern="[A-Za-z0-9.\-]+"
        title="Hostname (letters, numbers, '.', '-')" />
        <label>MQTT port: </label>
        <input type='number' name='MQTTPORT' value="%MQTTPORT%" 
        min="1" max="65535" step="1"
        title="Port number (1-65535)" />
        <label>MQTT user: </label><input type='text' name='MQTTUSER' value="%MQTTUSER%"         
        pattern="[ -~]+"
        title="MQTT username can only contain printable ASCII" />
        <label>MQTT password: </label><input type='password' name='MQTTPASSWORD' value="%MQTTPASSWORD%" 
        pattern="[ -~]+"
        title="MQTT password can only contain printable ASCII" />
        <label>MQTT timeout ms: </label>
        <input name='MQTTTIMEOUT' type='number' value="%MQTTTIMEOUT%" 
        min="1" max="60000" step="1"
        title="Timeout in milliseconds (1-60000)" />
        <label>Send all cellvoltages via MQTT: </label><input type='checkbox' name='MQTTCELLV' value='on' %MQTTCELLV% />
        <label>Remote BMS reset via MQTT allowed: </label>
        <input type='checkbox' name='REMBMSRESET' value='on' %REMBMSRESET% />
        <label>Customized MQTT topics: </label>
        <input type='checkbox' name='MQTTTOPICS' value='on' %MQTTTOPICS% />

        <div class='if-topics'>

        <label>MQTT topic name: </label><input type='text' name='MQTTTOPIC' value="%MQTTTOPIC%" />
        <label>Prefix for MQTT object ID: </label><input type='text' name='MQTTOBJIDPREFIX' value="%MQTTOBJIDPREFIX%" />
        <label>HA device name: </label><input type='text' name='MQTTDEVICENAME' value="%MQTTDEVICENAME%" />
        <label>HA device ID: </label><input type='text' name='HADEVICEID' value="%HADEVICEID%" />
        
        </div>

        <label>Enable Home Assistant auto discovery: </label>
        <input type='checkbox' name='HADISC' value='on' %HADISC% />

        </div>

        </div>
        </div>

        <div class="settings-card">
        <h3>Debug options</h3>
        <div style='display: grid; grid-template-columns: 1fr 1.5fr; gap: 10px; align-items: center;'>

        <label>Enable performance profiling on main page: </label>
        <input type='checkbox' name='PERFPROFILE' value='on' %PERFPROFILE%          
              title="For developers. Enable this to get detailed performance metrics on the front page" />

        <label>Enable CAN message logging via USB serial: </label>
        <input type='checkbox' name='CANLOGUSB' value='on' %CANLOGUSB%  
              title="WARNING: Causes performance issues. Enable this to get incoming/outgoing CAN messages logged via USB cable. Avoid if possible" />
        <script> //Make sure user only uses one general logging method, improves performance
        function handleCheckboxSelection(clickedCheckbox) { 
            const usbCheckbox = document.querySelector('input[name="USBENABLED"]');
            const webCheckbox = document.querySelector('input[name="WEBENABLED"]');
            
            if (clickedCheckbox.checked) {
                // If the clicked checkbox is being checked, uncheck the other one
                if (clickedCheckbox.name === 'USBENABLED') {
                    webCheckbox.checked = false;
                } else {
                    usbCheckbox.checked = false;
                }
            }
            // If unchecking, do nothing (allow both to be unchecked)
        }
        </script>

        <label>Enable general logging via USB serial: </label>
        <input type='checkbox' name='USBENABLED' value='on' %USBENABLED% 
              onclick="handleCheckboxSelection(this)" 
              title="WARNING: Causes performance issues. Enable this to get general logging via USB cable. Avoid if possible" />

        <label>Enable general logging via Webserver: </label>
        <input type='checkbox' name='WEBENABLED' value='on' %WEBENABLED% 
              onclick="handleCheckboxSelection(this)"         
              title="Enable this if you want general logging available in the Webserver" />

        <label>Enable CAN message logging via SD card: </label>
        <input type='checkbox' name='CANLOGSD' value='on' %CANLOGSD% 
        title="Enable this if you want incoming/outgoing CAN messages to be stored to an SD card. Only works on select hardware with SD-card slot" />

        <label>Enable general logging via SD card: </label>
        <input type='checkbox' name='SDLOGENABLED' value='on' %SDLOGENABLED% 
        title="Enable this if you want general logging to be stored to an SD card. Only works on select hardware with SD-card slot" />

        </div>
         </div>

        <div style='grid-column: span 2; text-align: center; padding-top: 10px;'><button type='submit'>Save</button></div>

        <div style='grid-column: span 2; text-align: center; padding-top: 10px;' class="%SAVEDCLASS%">
          <p>Settings saved. Reboot to take the new settings into use.<p> <button type='button' onclick='askReboot()'>Reboot</button>
        </div>

        </form>
    </div>
    </div>

      <h4 style='color: white;'>Battery interface: <span id='Battery'>%BATTERYINTF%</span></h4>

      <h4 style='color: white;' class="%BATTERY2CLASS%">Battery interface: <span id='Battery2'>%BATTERY2INTF%</span></h4>

      <h4 style='color: white;' class="%INVCLASS%">Inverter interface: <span id='Inverter'>%INVINTF%</span></h4>
      
      <h4 style='color: white;' class="%SHUNTCLASS%">Shunt interface: <span id='Inverter'>%SHUNTINTF%</span></h4>

    </div>

    <div style='background-color: #2D3F2F; padding: 10px; margin-bottom: 10px;border-radius: 50px'>

      <h4 style='color: white;'>Battery capacity: <span id='BATTERY_WH_MAX'>%BATTERY_WH_MAX% Wh </span> <button onclick='editWh()'>Edit</button></h4>

      <h4 style='color: white;'>Rescale SOC: <span id='BATTERY_USE_SCALED_SOC'><span class='%SOC_SCALING_CLASS%'>%SOC_SCALING%</span>
                </span> <button onclick='editUseScaledSOC()'>Edit</button></h4>

      <h4 class='%SOC_SCALING_ACTIVE_CLASS%'><span>SOC max percentage: %SOC_MAX_PERCENTAGE%</span> <button onclick='editSocMax()'>Edit</button></h4>

      <h4 class='%SOC_SCALING_ACTIVE_CLASS%'><span>SOC min percentage: %SOC_MIN_PERCENTAGE%</span> <button onclick='editSocMin()'>Edit</button></h4>
      
      <h4 style='color: white;'>Max charge speed: %MAX_CHARGE_SPEED% A </span> <button onclick='editMaxChargeA()'>Edit</button></h4>

      <h4 style='color: white;'>Max discharge speed: %MAX_DISCHARGE_SPEED% A </span><button onclick='editMaxDischargeA()'>Edit</button></h4>

      <h4 style='color: white;'>Manual charge voltage limits: <span id='BATTERY_USE_VOLTAGE_LIMITS'>
        <span class='%VOLTAGE_LIMITS_CLASS%'>%VOLTAGE_LIMITS%</span>
                </span> <button onclick='editUseVoltageLimit()'>Edit</button></h4>

      <h4 class='%VOLTAGE_LIMITS_ACTIVE_CLASS%'>Target charge voltage: %CHARGE_VOLTAGE% V </span> <button onclick='editMaxChargeVoltage()'>Edit</button></h4>

      <h4 class='%VOLTAGE_LIMITS_ACTIVE_CLASS%'>Target discharge voltage: %DISCHARGE_VOLTAGE% V </span> <button onclick='editMaxDischargeVoltage()'>Edit</button></h4>

      <h4 style='color: white;'>Periodic BMS reset off time: %BMS_RESET_DURATION% s </span><button onclick='editBMSresetDuration()'>Edit</button></h4>

    </div>

    <div style='background-color: #2E37AD; padding: 10px; margin-bottom: 10px;border-radius: 50px' class="%FAKE_VOLTAGE_CLASS%">
      <h4 style='color: white;'><span>Fake battery voltage: %BATTERY_VOLTAGE% V </span> <button onclick='editFakeBatteryVoltage()'>Edit</button></h4>
    </div>

    <!--if (battery && battery->supports_manual_balancing()) {-->
      
    <div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px' class="%MANUAL_BAL_CLASS%">

          <h4 style='color: white;'>Manual LFP balancing: <span id='TSL_BAL_ACT'><span class="%MANUAL_BALANCING_CLASS%">%MANUAL_BALANCING%</span>
          </span> <button onclick='editTeslaBalAct()'>Edit</button></h4>

          <h4 class="%BALANCING_CLASS%"><span>Balancing max time: %BAL_MAX_TIME% Minutes</span> <button onclick='editBalTime()'>Edit</button></h4>

          <h4 class="%BALANCING_CLASS%"><span>Balancing float power: %BAL_POWER% W </span> <button onclick='editBalFloatPower()'>Edit</button></h4>

           <h4 class="%BALANCING_CLASS%"><span>Max battery voltage: %BAL_MAX_PACK_VOLTAGE% V</span> <button onclick='editBalMaxPackV()'>Edit</button></h4>

           <h4 class="%BALANCING_CLASS%"><span>Max cell voltage: %BAL_MAX_CELL_VOLTAGE% mV</span> <button onclick='editBalMaxCellV()'>Edit</button></h4>

          <h4 class="%BALANCING_CLASS%"><span>Max cell voltage deviation: %BAL_MAX_DEV_CELL_VOLTAGE% mV</span> <button onclick='editBalMaxDevCellV()'>Edit</button></h4>

    </div>

     <div style='background-color: #FF6E00; padding: 10px; margin-bottom: 10px;border-radius: 50px' class="%CHARGER_CLASS%">

      <h4 style='color: white;'>
        Charger HVDC Enabled: <span class="%CHG_HV_CLASS%">%CHG_HV%</span>
        <button onclick='editChargerHVDCEnabled()'>Edit</button>
      </h4>

      <h4 style='color: white;'>
        Charger Aux12VDC Enabled: <span class="%CHG_AUX12V_CLASS%">%CHG_AUX12V%</span>
        <button onclick='editChargerAux12vEnabled()'>Edit</button>
      </h4>

      <h4 style='color: white;'><span>Charger Voltage Setpoint: %CHG_VOLTAGE_SETPOINT% V </span> <button onclick='editChargerSetpointVDC()'>Edit</button></h4>

      <h4 style='color: white;'><span>Charger Current Setpoint: %CHG_CURRENT_SETPOINT% A</span> <button onclick='editChargerSetpointIDC()'>Edit</button></h4>

      </div>
    
  </div>

)rawliteral"

const char settings_html[] =
    INDEX_HTML_HEADER COMMON_JAVASCRIPT SETTINGS_STYLE SETTINGS_HTML_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
