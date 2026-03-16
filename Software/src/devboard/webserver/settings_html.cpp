#include "settings_html.h"
#include <Arduino.h>
#include "../../../src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../../src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../system_settings.h"
#include "html_escape.h"
#include "index_html.h"
#include "src/battery/BATTERIES.h"
#include "src/battery/Shunt.h"
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

static const std::map<int, String> led_modes = {{0, "Classic"}, {1, "Energy Flow"}, {2, "Heartbeat"}, {3, "Disabled"}};

static const std::map<int, String> tesla_countries = {
    {21843, "US (USA)"},     {17217, "CA (Canada)"},  {18242, "GB (UK & N Ireland)"},
    {17483, "DK (Denmark)"}, {17477, "DE (Germany)"}, {16725, "AU (Australia)"}};

static const std::map<int, String> tesla_mapregion = {
    {8, "ME (Middle East)"}, {2, "NONE"},       {3, "CN (China)"},     {6, "TW (Taiwan)"}, {5, "JP (Japan)"},
    {0, "US (USA)"},         {7, "KR (Korea)"}, {4, "AU (Australia)"}, {1, "EU (Europe)"}};

static const std::map<int, String> tesla_chassis = {{0, "Model S"}, {1, "Model X"}, {2, "Model 3"}, {3, "Model Y"}};

static const std::map<int, String> tesla_pack = {{0, "50 kWh"}, {2, "62 kWh"}, {1, "74 kWh"}, {3, "100 kWh"}};

static const std::map<int, String> sungrow_models = {
    {0, "SBR064 (6.4 kWh, 2 modules)"},  {1, "SBR096 (9.6 kWh, 3 modules)"},  {2, "SBR128 (12.8 kWh, 4 modules)"},
    {3, "SBR160 (16.0 kWh, 5 modules)"}, {4, "SBR192 (19.2 kWh, 6 modules)"}, {5, "SBR224 (22.4 kWh, 7 modules)"},
    {6, "SBR256 (25.6 kWh, 8 modules)"}};

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
#ifdef HW_LILYGO2CAN
const char* name_for_gpioopt1(GPIOOPT1 option) {
  switch (option) {
    case GPIOOPT1::DEFAULT_OPT:
      return "WUP1 / WUP2";
    case GPIOOPT1::I2C_DEVICES:
      return "I2C Devices (QWIIC)";
    case GPIOOPT1::ESTOP_BMS_POWER:
      return "E-Stop / BMS Power";
    default:
      return nullptr;
  }
}

const char* name_for_display_type(DisplayType type) {
  switch (type) {
    case DisplayType::NONE:
      return "None";
    case DisplayType::OLED_I2C:
      return "I2C OLED (SSD1306)";
    case DisplayType::EPAPER_SPI_42_3C:
      return "SPI E-Paper 4.2\" (B/W/Red)";
    case DisplayType::EPAPER_SPI_42_BW:
      return "SPI E-Paper 4.2\" (B/W)";
    default:
      return nullptr;
  }
}
#endif
const char* name_for_gpioopt2(GPIOOPT2 option) {
  switch (option) {
    case GPIOOPT2::DEFAULT_OPT_BMS_POWER_18:
      return "Pin 18";
    case GPIOOPT2::BMS_POWER_25:
      return "Pin 25";
    default:
      return nullptr;
  }
}
const char* name_for_gpioopt3(GPIOOPT3 option) {
  switch (option) {
    case GPIOOPT3::DEFAULT_SMA_ENABLE_05:
      return "Pin 5";
    case GPIOOPT3::SMA_ENABLE_33:
      return "Pin 33";
    default:
      return nullptr;
  }
}

const char* name_for_gpioopt4(GPIOOPT4 option) {
  switch (option) {
    case GPIOOPT4::DEFAULT_SD_CARD:
      return "uSD Card";
    case GPIOOPT4::I2C_DEVICES:
      return "I2C Devices";
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

  if (var == "CTATTEN") {
    return options_for_enum_with_none(
        (adc_attenuation_enum)settings.getUInt("CTATTEN", (int)adc_attenuation_enum::ADC_0db), name_for_adc_attenuation,
        adc_attenuation_enum::ADC_0db);
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
    return options_from_map(settings.getUInt("LEDMODE", 3), led_modes);  // Default to disabled to save power
  }

  if (var == "LEDTAIL") {
    return String(settings.getUInt("LEDTAIL", 4));
  }
  if (var == "LEDCOUNT") {
    return String(settings.getUInt("LEDCOUNT", 8));
  }

  if (var == "SUNGROW_MODEL") {
    return options_from_map(settings.getUInt("INVSUNTYPE", 1), sungrow_models);  // Default: SBR096
  }

#ifdef HW_LILYGO2CAN
  if (var == "GPIOOPT1") {
    return options_for_enum_with_none((GPIOOPT1)settings.getUInt("GPIOOPT1", (int)GPIOOPT1::DEFAULT_OPT),
                                      name_for_gpioopt1, GPIOOPT1::DEFAULT_OPT);
  }

  if (var == "DISPLAYTYPE") {
    return options_for_enum_with_none((DisplayType)settings.getUInt("DISPLAYTYPE", (int)DisplayType::OLED_I2C),
                                      name_for_display_type, DisplayType::NONE);
  }
#endif
  if (var == "GPIOOPT2") {
    return options_for_enum_with_none((GPIOOPT2)settings.getUInt("GPIOOPT2", (int)GPIOOPT2::DEFAULT_OPT_BMS_POWER_18),
                                      name_for_gpioopt2, GPIOOPT2::DEFAULT_OPT_BMS_POWER_18);
  }

  if (var == "GPIOOPT3") {
    return options_for_enum_with_none((GPIOOPT3)settings.getUInt("GPIOOPT3", (int)GPIOOPT3::DEFAULT_SMA_ENABLE_05),
                                      name_for_gpioopt3, GPIOOPT3::DEFAULT_SMA_ENABLE_05);
  }

  if (var == "GPIOOPT4") {
    return options_for_enum_with_none((GPIOOPT4)settings.getUInt("GPIOOPT4", (int)GPIOOPT4::DEFAULT_SD_CARD),
                                      name_for_gpioopt4, GPIOOPT4::DEFAULT_SD_CARD);
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

  if (var == "WEBAUTH_1") {
    // get WEBAUTH, if null set is 1 (safty1st)
    return settings.getUInt("WEBAUTH", 0) == 1 ? "selected" : "";
  }

  if (var == "WEBAUTH_0") {
    return settings.getUInt("WEBAUTH", 0) == 0 ? "selected" : "";
  }

  if (var == "AUTH_DISPLAY") {
    return settings.getUInt("WEBAUTH", 0) == 1 ? "block" : "none";
  }

  if (var == "WEBUSER") {
    return settings.getString("WEBUSER", DEFAULT_WEB_USER);
  }

  if (var == "WEBPASS") {
    return settings.getString("WEBPASS", DEFAULT_WEB_PASS);
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

  if (var == "SHUNTINTF") {
    if (shunt) {
      return shunt->interface_name();
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

  if (var == "EPAPREFRESHBTN") {
    return settings.getBool("EPAPREFRESHBTN") ? "checked" : "";
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

  if (var == "MAXPREFREQ") {
    return String(settings.getUInt("MAXPREFREQ", 34000));
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

  if (var == "ESPNOWENABLED") {
    return settings.getBool("ESPNOWENABLED") ? "checked" : "";
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

  if (var == "MQTTPUBLISHMS") {
    return String(settings.getUInt("MQTTPUBLISHMS", 5000) / 1000);
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
    return String(datalayer.battery.settings.balancing_max_time_ms / 60000.0f, 1);
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

  if (var == "PYLONBAUD") {
    return String(settings.getUInt("PYLONBAUD", 500));
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

  if (var == "PRIMOGEN24") {
    return settings.getBool("PRIMOGEN24") ? "checked" : "";
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

  if (var == "MULTII2C") {
    return settings.getBool("MULTII2C") ? "checked" : "";
  }
  if (var == "I2C_OLED") {
    return settings.getBool("I2C_OLED") ? "checked" : "";
  }
  if (var == "I2C_SHT30") {
    return settings.getBool("I2C_SHT30") ? "checked" : "";
  }
  if (var == "I2C_ATECC") {
    return settings.getBool("I2C_ATECC") ? "checked" : "";
  }
  if (var == "I2C_RTC") {
    return settings.getBool("I2C_RTC") ? "checked" : "";
  }
  if (var == "I2C_IO") {
    return settings.getBool("I2C_IO") ? "checked" : "";
  }
  if (var == "CTOFFSET") {
    return settings.getString("CTOFFSET", "-1.0");
  }

  if (var == "CTVNOM") {
    return String(settings.getUInt("CTVNOM", 40));
  }

  if (var == "CTANOM") {
    return String(settings.getUInt("CTANOM", 100));
  }

  if (var == "CTINVERT") {
    return settings.getBool("CTINVERT") ? "checked" : "";
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

#ifdef HW_LILYGO2CAN
#define GPIOOPT1_SETTING \
  R"rawliteral(
    <label for="GPIOOPT1">Configurable port:</label>
    <select id="GPIOOPT1" name="GPIOOPT1">
      %GPIOOPT1%
    </select>
    
    <div class="if-i2c" style="grid-column: span 2; background: #f9f9f9; padding: 15px; border-left: 4px solid #3498db; margin: 10px 0; border-radius: 4px;">
        
        <div style="display: flex; align-items: center; gap: 10px; flex-wrap: wrap; margin-bottom: 5px;">
          <label style="font-weight: bold; color: #333; margin: 0; text-align: left; padding: 0;">Enable Multiple I2C Devices:</label>
          <input type='checkbox' name='MULTII2C' value='on' %MULTII2C% style="margin: 0; width: 20px; height: 20px;" />
          <span style="font-size: 0.85em; color: #856404; background: #fff3cd; padding: 4px 8px; border-radius: 4px;">
              💡 To use OLED, select it in "Display Type" below
          </span>
        </div>
        
        <div class="if-multii2c" style="margin-top: 15px; padding-top: 15px; border-top: 1px dashed #ccc;">
          <h4 style="margin-bottom: 12px; color: #555; text-align: left;">Select active I2C devices on the bus:</h4>
          
          <div style="display: flex; flex-direction: column; gap: 12px; padding-left: 5px;">
            <label style="display: flex; align-items: center; gap: 10px; text-align: left; font-weight: normal; margin: 0; padding: 0; cursor: pointer;">
              <input type="checkbox" name="I2C_SHT30" value="on" %I2C_SHT30% style="margin: 0; flex-shrink: 0;">
              <span><code>0x44</code> - SHT30 (Ambient Temp/Humidity)</span>
            </label>
            
            <label style="display: flex; align-items: center; gap: 10px; text-align: left; font-weight: normal; margin: 0; padding: 0; cursor: pointer;">
              <input type="checkbox" name="I2C_ATECC" value="on" %I2C_ATECC% style="margin: 0; flex-shrink: 0;">
              <span><code>0x60</code> - ATECC608A (Hardware Security)</span>
            </label>
            
            <label style="display: flex; align-items: center; gap: 10px; text-align: left; font-weight: normal; margin: 0; padding: 0; cursor: pointer;">
              <input type="checkbox" name="I2C_RTC" value="on" %I2C_RTC% style="margin: 0; flex-shrink: 0;">
              <span><code>0x68</code> - DS3231 (Offline RTC Logging)</span>
            </label>
            
            <label style="display: flex; align-items: center; gap: 10px; text-align: left; font-weight: normal; margin: 0; padding: 0; cursor: pointer;">
              <input type="checkbox" name="I2C_IO" value="on" %I2C_IO% style="margin: 0; flex-shrink: 0;">
              <span><code>0x20</code> - PCF8574 (External IO & Relays)</span>
            </label>
          </div>
        </div> 
        </div> 
    )rawliteral"
#else
#define GPIOOPT1_SETTING ""
#endif

#ifdef HW_LILYGO2CAN
#define DISPLAY_SETTING \
  R"rawliteral(
    <label for="DISPLAYTYPE">Display Type:</label>
    <select id="DISPLAYTYPE" name="DISPLAYTYPE" onchange="checkDisplayWarning()">
      %DISPLAYTYPE%
    </select>
    <div class="if-epaper3c">
      <label>Enable Manual Refresh Button (Pin 40): </label>
      <input type='checkbox' name='EPAPREFRESHBTN' value='on' %EPAPREFRESHBTN% />
    </div>
    
    <div id="epaper_warning" style="display:none; margin-top:5px; padding:10px; background-color:#fff3cd; border:1px solid #ffeeba; color:#856404; border-radius:5px; font-size: 0.9em; grid-column: span 2;">
      ⚠️ <b>Constraint Warning:</b><br>
      Selecting E-Paper will disable:<br>
      • <b>SMA Inverter Control</b> (Pin 46 used for CS)<br>
      • <b>Battery3 CTR</b> (Pin 4 used for BUSY)<br>
      <br><b>Required Pins:</b> SCK:16, MOSI:15, CS:46, DC:45, RST:47, BUSY:4
    </div>

    <div id="oled_warning" style="display:none; margin-top:5px; padding:10px; background-color:#d1ecf1; border:1px solid #bee5eb; color:#0c5460; border-radius:5px; font-size: 0.9em; grid-column: span 2;">
      ℹ️ <b>Port Auto-Assigned:</b><br>
      I2C OLED(0x3C) is using the <b>QWIIC Port (IO1, IO2)</b>.<br>
      <i>The "Configurable port" above is disabled to prevent conflicts.</i>
    </div>
  )rawliteral"
#else
#define DISPLAY_SETTING ""
#endif

#ifdef HW_LILYGO
#define GPIOOPT2_SETTING \
  R"rawliteral(
    <label for="GPIOOPT2">BMS Power pin:</label>
    <select id="GPIOOPT2" name="GPIOOPT2">
      %GPIOOPT2%
    </select>
  )rawliteral"
#else
#define GPIOOPT2_SETTING ""
#endif

#ifdef HW_LILYGO
#define GPIOOPT3_SETTING \
  R"rawliteral(
    <label for="GPIOOPT3">SMA enable pin:</label>
    <select id="GPIOOPT3" name="GPIOOPT3">
      %GPIOOPT3%
    </select>
  )rawliteral"
#else
#define GPIOOPT3_SETTING ""
#endif

#ifdef HW_LILYGO
#define GPIOOPT4_SETTING \
  R"rawliteral(
    <label for="GPIOOPT4">uSD Slot:</label>
    <select id="GPIOOPT4" name="GPIOOPT4">
      %GPIOOPT4%
    </select>
  )rawliteral"
#else
#define GPIOOPT4_SETTING ""
#endif

#define SETTINGS_HTML_SCRIPTS \
  R"rawliteral(
    <script> 
      function checkLedPower(){var e=document.getElementById("LEDCOUNT"),t=document.getElementById("led_power_warning"),n=document.getElementById("led_mA"),d=document.getElementById("led_msg");if(e&&t){var l=parseInt(e.value)*10;n.innerText=l,l>500?(t.style.backgroundColor="#f8d7da",t.style.color="#721c24",t.style.borderColor="#f5c6cb",d.innerText="⚠️ DANGER: Exceeds 500mA limit! May crash ESP32!"):(t.style.backgroundColor="#e2e3e5",t.style.color="#383d41",t.style.borderColor="#d6d8db",d.innerText="(Safe: Under 500mA limit)")}}
      function checkLedMode(){var e=document.getElementById("LEDMODE"),t=document.getElementById("lbl_ledtail"),n=document.getElementById("input_ledtail");e&&t&&n&&("1"==e.value?(t.style.display="",n.style.display=""):(t.style.display="none",n.style.display="none"))}
      
      function askFactoryReset(){confirm("Are you sure you want to reset the device to factory settings?")&&((xhr=new XMLHttpRequest).onload=function(){200==this.status?(alert("Factory reset successful. Restarting..."),window.location.href="/reboot"):alert("Factory reset failed.")},xhr.onerror=function(){alert("Error resetting device.")},xhr.open("POST","/factoryReset",!0),xhr.send())}
      
      function askReboot() {
        if(confirm("Are you sure you want to reboot the system?")) {
          var modal = document.createElement('div');
          
          modal.innerHTML = '<div style="position:fixed;top:0;left:0;width:100vw;height:100vh;background:rgba(0,0,0,0.85);z-index:99999;display:flex;justify-content:center;align-items:center;backdrop-filter:blur(5px);">' +
            '<div style="background:#fff;padding:35px 20px;border-radius:15px;text-align:center;box-shadow:0 10px 30px rgba(0,0,0,0.5);width:350px;max-width:90vw;font-family:sans-serif;">' +
              '<h2 style="color:#e74c3c;margin-top:0;font-size:1.8rem;">🔄 System Rebooting</h2>' +
              '<div style="border:5px solid #f3f3f3;border-top:5px solid #e74c3c;border-radius:50vw;width:60px;height:60px;animation:spin 1s linear infinite;margin:25px auto;"></div>' +
              '<p style="color:#555;font-size:1.1rem;margin:10px 0;">Please wait <span id="rebootCount" style="font-weight:bold;color:#e74c3c;font-size:1.3rem;">15</span> seconds.</p>' +
              '<p style="font-size:0.85rem;color:#888;">The system is restarting.<br>The page will reload automatically.</p>' +
            '</div></div><style>@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }</style>';
          
          document.body.appendChild(modal);

          fetch('/reboot', { method: 'GET', keepalive: true }).catch(function(){});
          
          let timeLeft = 15;
          let countEl = document.getElementById('rebootCount');
          let timer = setInterval(function() {
            timeLeft--;
            countEl.innerText = timeLeft;
            if (timeLeft <= 0) {
              clearInterval(timer);
              window.location.href = '/';
            }
          }, 1000);
        }
      }

      function editComplete(){200==this.status&&window.location.reload()}function editError(){alert("Invalid input")}
      function sendEdit(e,t,n,d,l,o){var r=prompt(e);null!==r&&(r>=d&&l>=r?((xhr=new XMLHttpRequest).onload=editComplete,xhr.onerror=editError,xhr.open("GET",t+r,parseInt(o)),xhr.send()):alert(n))}
      function editRecoveryMode(){sendEdit("Extremely dangerous! Start 30min recovery process? (0=No, 1=Yes):","/enableRecoveryMode?value=","Enter 0 or 1",0,1,!0)}
      function editWh(){sendEdit("Enter new Wh capacity (1-400000):","/updateBatterySize?value=","Value between 1-400000",1,4e5,!0)}
      function editUseScaledSOC(){sendEdit("Apply SOC scaling? (0=No, 1=Yes):","/updateUseScaledSOC?value=","Enter 0 or 1",0,1,!0)}
      function editSocMax(){sendEdit("Enter max SOC percentage (50.0-100.0):","/updateSocMax?value=","Value between 50-100",50,100,!0)}
      function editSocMin(){sendEdit("Enter min SOC percentage (-10.0 to 50.0):","/updateSocMin?value=","Value between -10 and 50",-10,50,!0)}
      function editMaxChargeA(){sendEdit("Enter max charge current in A (0-1000.0):","/updateMaxChargeA?value=","Value between 0-1000",0,1e3,!0)}
      function editMaxDischargeA(){sendEdit("Enter max discharge current in A (0-1000.0):","/updateMaxDischargeA?value=","Value between 0-1000",0,1e3,!0)}
      function editUseVoltageLimit(){sendEdit("Restrict voltage manually? (0=No, 1=Yes):","/updateUseVoltageLimit?value=","Enter 0 or 1",0,1,!0)}
      function editMaxChargeVoltage(){sendEdit("Enter voltage setpoint to charge to (0-1000.0):","/updateMaxChargeVoltage?value=","Value between 0-1000",0,1e3,!0)}
      function editMaxDischargeVoltage(){sendEdit("Enter voltage setpoint to discharge to (0-1000.0):","/updateMaxDischargeVoltage?value=","Value between 0-1000",0,1e3,!0)}
      function editBMSresetDuration(){sendEdit("Seconds BMS power off during reset (1-59):","/updateBMSresetDuration?value=","Value between 1-59",1,59,!0)}
      function editTeslaBalAct(){sendEdit("Enable forced LFP balancing (1=Yes, 0=No):","/TeslaBalAct?value=","Enter 0 or 1",0,1,!0)}
      function editBalTime(){sendEdit("Enter max balancing time (minutes 1-300):","/BalTime?value=","Value between 1-300",1,300,!0)}
      function editBalFloatPower(){sendEdit("Float charge power (Watt 100-2000):","/BalFloatPower?value=","Value between 100-2000",100,2e3,!0)}
      function editBalMaxPackV(){sendEdit("Temp max pack voltage (V 380-410):","/BalMaxPackV?value=","Value between 380-410",380,410,!0)}
      function editBalMaxCellV(){sendEdit("Temp max cell voltage (mV 3400-3750):","/BalMaxCellV?value=","Value between 3400-3750",3400,3750,!0)}
      function editBalMaxDevCellV(){sendEdit("Temp max cell deviation (mV 300-600):","/BalMaxDevCellV?value=","Value between 300-600",300,600,!0)}
      function editFakeBatteryVoltage(){sendEdit("Enter fake battery voltage (0-5000):","/updateFakeBatteryVoltage?value=","Value between 0-5000",0,5e3,!0)}
      function editChargerHVDCEnabled(){sendEdit("Enable HV DC output (1=Yes, 0=No):","/updateChargerHvEnabled?value=","Enter 0 or 1",0,1,!0)}
      function editChargerAux12vEnabled(){sendEdit("Enable Aux 12V output (1=Yes, 0=No):","/updateChargerAux12vEnabled?value=","Enter 0 or 1",0,1,!0)}
      function editChargerSetpointVDC(){sendEdit("Set charging voltage (V 0-1000):","/updateChargeSetpointV?value=","Value between 0-1000",0,1e3,!0)}
      function editChargerSetpointIDC(){sendEdit("Set charging amperage (A 0-1000):","/updateChargeSetpointA?value=","Value between 0-1000",0,1e3,!0)}
      function editChargerSetpointEndI(){sendEdit("Set cut-off amperage (A 0-1000):","/updateChargeEndA?value=","Value between 0-1000",0,1e3,!0)}
      function goToMainPage(){window.location.href="/"}
      function checkDisplayWarning(){var e=document.getElementById("DISPLAYTYPE"),t=document.getElementById("epaper_warning"),n=document.getElementById("oled_warning"),d=document.getElementById("GPIOOPT1");e&&(t&&(t.style.display="none"),n&&(n.style.display="none"),d&&(d.disabled=!1),"2"==e.value||"3"==e.value?t&&(t.style.display="block"):"1"==e.value&&(n&&(n.style.display="block"),d&&(d.disabled=!0)))}
      
      window.addEventListener("load",function(){checkLedPower(),checkLedMode(),checkDisplayWarning()});
      var frm=document.querySelector("form");if(frm){document.querySelectorAll("select,input").forEach(function(e){function t(){if(e.name)frm.setAttribute("data-"+e.name.toLowerCase(),e.type=="checkbox"?e.checked:e.value)}e.addEventListener("change",t);t()})}
      
      function selectScannedNet() {
        var sel = document.getElementById('scanned_ssids');
        if(sel.value) {
          document.getElementById('net_ssid').value = sel.value;
          document.getElementById('net_pass').focus();
        }
      }

      function scanWifi() {
        var sel = document.getElementById('scanned_ssids');
        sel.innerHTML = '<option value="">⏳ Scanning...</option>';
        fetch('/startScan', {method: 'POST'}).then(function() {
            var poll = setInterval(function() {
                fetch('/getScan').then(function(r) { return r.json(); }).then(function(data) {
                    if(data.status !== "scanning") {
                        clearInterval(poll);
                        sel.innerHTML = '<option value="">-- Select Network --</option>';
                        data.forEach(function(net) {
                            var opt = document.createElement('option');
                            opt.value = net.ssid;
                            opt.text = net.ssid + " (" + net.rssi + "dBm)";
                            sel.appendChild(opt);
                        });
                    }
                });
            }, 1500);
        });
      }

      function applyWifi() {
        var ssid = document.getElementById('net_ssid').value;
        var pass = document.getElementById('net_pass').value;
        var keep_ap = document.getElementById('chk_ap').checked;
        if(!ssid) { alert("Please enter SSID!"); return; }

        var modal = document.createElement('div');
        modal.id = 'wifi_modal';
        modal.innerHTML = '<div style="position:fixed;top:0;left:0;width:100vw;height:100vh;background:rgba(0,0,0,0.85);z-index:9999;display:flex;justify-content:center;align-items:center;">' +
            '<div style="background:#fff;padding:30px;border-radius:10px;text-align:center;max-width:400px;width:90vw;box-shadow:0 10px 25px rgba(0,0,0,0.5);">' +
                '<h3 id="wm_title" style="color:#333;margin-top:0;">🚀 Connecting...</h3>' +
                '<p id="wm_desc" style="color:#555;font-size:1rem;">Please wait up to 10s while we connect to <b>' + ssid + '</b>...</p>' +
                '<div id="wm_spinner" style="margin:20px auto;border:5px solid #f3f3f3;border-top:5px solid #3498db;border-radius:50vw;width:45px;height:45px;animation:spin 1s linear infinite;"></div>' +
            '</div></div>';
        document.body.appendChild(modal);

        fetch('/applyWifi', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass) + '&keep_ap=' + (keep_ap ? '1' : '0')
        }).then(function() {
            var poll = setInterval(function() {
                fetch('/applyWifiStatus').then(function(r) { return r.json(); }).then(function(data) {
                    if(data.state == 2) {
                        clearInterval(poll);
                        document.getElementById('wm_title').innerHTML = "✅ Success!";
                        document.getElementById('wm_title').style.color = "#28a745";
                        document.getElementById('wm_spinner').style.display = "none";
                        
                        var actionHtml = '<p style="font-size:1.1rem;margin:15px 0;">Device is online!</p>' +
                                        '<a href="http://' + data.ip + '" style="display:inline-block;padding:12px 25px;background:#3498db;color:#fff;text-decoration:none;border-radius:6px;font-weight:bold;margin-bottom:10px;box-shadow:0 4px 6px rgba(52,152,219,0.3);">Go to New IP: ' + data.ip + '</a>';
                        
                        if(!keep_ap) {
                            actionHtml += '<p style="color:#e74c3c;font-size:0.85rem;margin-top:15px;">⚠️ Hotspot is now turning off.<br>Connect your phone to <b>' + ssid + '</b> to continue.</p>';
                        } else {
                            actionHtml += '<p style="color:#7f8c8d;font-size:0.85rem;margin-top:15px;">(Hotspot is still active, you can close this popup)</p>' +
                                          '<button onclick="document.getElementById(\'wifi_modal\').remove();" style="padding:8px 20px; border:none; background:#eee; font-weight:bold; border-radius:4px; cursor:pointer;">Close</button>';
                        }
                        document.getElementById('wm_desc').innerHTML = actionHtml;
                    } else if(data.state == 3) {
                        clearInterval(poll);
                        document.getElementById('wm_title').innerHTML = "❌ Failed";
                        document.getElementById('wm_title').style.color = "#e74c3c";
                        document.getElementById('wm_spinner').style.display = "none";
                        document.getElementById('wm_desc').innerHTML = '<p>Could not connect to <b>' + ssid + '</b>. Please check password.</p>' +
                                                                        '<button onclick="document.getElementById(\'wifi_modal\').remove()" style="margin-top:15px;padding:10px 25px;background:#555;color:#fff;font-weight:bold;border:none;border-radius:5px;cursor:pointer;">Try Again</button>';
                    }
                });
            }, 1500);
        });
      }
    </script>
  )rawliteral"

#define SETTINGS_STYLE \
  R"rawliteral(
<style>h3{color:#2c3e50;font-size:1.15rem;margin-top:0;padding-bottom:8px;border-bottom:2px solid #eee;margin-bottom:15px}h4{margin:0;font-weight:500;color:#444;font-size:.95rem}input[type=number],input[type=password],input[type=text],select{width:100%;max-width:250px;padding:6px 10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;font-size:.9rem;transition:.2s}input:focus,select:focus{border-color:#3498db;outline:0}input[type=checkbox]{width:18px;height:18px;cursor:pointer;justify-self:start;margin:0}.hidden{display:none!important}.active{color:#2ecc71!important;font-weight:700}.inactive{color:#bdc3c7!important;font-style:italic}.inactiveSoc{color:#e74c3c!important;font-weight:700}.form-grid{display:grid;grid-template-columns:1fr 1.5fr;gap:12px 10px;align-items:center}.form-grid label{color:#555;font-size:.9rem;font-weight:600;text-align:right;padding-right:10px}.override-grid{display:grid;grid-template-columns:1fr auto;gap:15px;align-items:center;padding:12px 0;border-bottom:1px dashed #eee}.override-grid:last-child{border-bottom:none}.mqtt-settings,.mqtt-topics{display:none;grid-column:span 2}form .if-battery,form .if-charger,form .if-inverter,form .if-shunt{display:contents}form[data-battery="0"] .if-battery,form[data-charger="0"] .if-charger,form[data-inverter="0"] .if-inverter,form[data-shunttype="0"] .if-shunt{display:none}form .if-auth{display:contents}form[data-webauth="0"] .if-auth{display:none}form .if-epaper3c{display:none}form[data-displaytype="2"] .if-epaper3c{display:contents}form .if-cbms{display:none}form[data-battery="11"] .if-cbms,form[data-battery="22"] .if-cbms,form[data-battery="23"] .if-cbms,form[data-battery="24"] .if-cbms,form[data-battery="31"] .if-cbms,form[data-battery="41"] .if-cbms,form[data-battery="48"] .if-cbms,form[data-battery="49"] .if-cbms,form[data-battery="6"] .if-cbms{display:contents}form .if-nissan{display:none}form[data-battery="21"] .if-nissan{display:contents}form .if-tesla{display:none}form[data-battery="32"] .if-tesla,form[data-battery="33"] .if-tesla{display:contents}form .if-estimated{display:none}form[data-battery="14"] .if-estimated,form[data-battery="16"] .if-estimated,form[data-battery="24"] .if-estimated,form[data-battery="3"] .if-estimated,form[data-battery="32"] .if-estimated,form[data-battery="33"] .if-estimated,form[data-battery="4"] .if-estimated,form[data-battery="40"] .if-estimated,form[data-battery="41"] .if-estimated,form[data-battery="44"] .if-estimated,form[data-battery="6"] .if-estimated{display:contents}form .if-socestimated{display:none}form[data-battery="16"] .if-socestimated,form[data-battery="41"] .if-socestimated{display:contents}form .if-dblbtr{display:none}form[data-dblbtr="true"] .if-dblbtr{display:contents}form .if-tribtr{display:none}form[data-tribtr="true"] .if-tribtr{display:contents}form .if-pwmcntctrl{display:none}form[data-pwmcntctrl="true"] .if-pwmcntctrl{display:contents}form .if-cntctrl{display:none}form[data-cntctrl="true"] .if-cntctrl{display:contents}form .if-extprecharge{display:none}form[data-extprecharge="true"] .if-extprecharge{display:contents}form .if-sofar{display:none}form[data-inverter="17"] .if-sofar{display:contents}form .if-byd{display:none}form[data-inverter="2"] .if-byd{display:contents}form .if-pylon{display:none}form[data-battery="22"] .if-pylon,form[data-inverter="10"] .if-pylon{display:contents}form .if-pylon-inverter{display:none}form[data-inverter="10"] .if-pylon-inverter{display:contents}form .if-pylon-battery{display:none}form[data-battery="22"] .if-pylon-battery{display:contents}form .if-pylonish{display:none}form[data-inverter="10"] .if-pylonish,form[data-inverter="19"] .if-pylonish,form[data-inverter="4"] .if-pylonish{display:contents}form .if-solax{display:none}form[data-inverter="18"] .if-solax{display:contents}form .if-sungrow{display:none}form[data-inverter="21"] .if-sungrow{display:contents}form .if-kostal{display:none}form[data-inverter="9"] .if-kostal{display:contents}form .if-staticip{display:none}form[data-staticip="true"] .if-staticip{display:contents}form .if-mqtt{display:none}form[data-mqttenabled="true"] .if-mqtt{display:contents}form .if-topics{display:none}form[data-mqtttopics="true"] .if-topics{display:contents} form .if-i2c { display:none } form[data-gpioopt1="1"] .if-i2c, form[data-displaytype="1"] .if-i2c { display:block } form .if-multii2c { display:none } form[data-multii2c="true"] .if-multii2c { display:block } form .if-ctclamp { display:none } form[data-shunttype="3"] .if-ctclamp { display:contents }
</style>
)rawliteral"

// =======================================================
// 🗂️ Menu (Tab Navigation)
// =======================================================
#define TABS_CSS \
  R"rawliteral(
  <style>
    .set-tabs { display: flex; flex-wrap: wrap; gap: 8px; margin-bottom: 20px; border-bottom: 2px solid #ddd; padding-bottom: 12px; }
    .set-tab { padding: 10px 15px; background: #f8f9fa; border: 1px solid #ccc; border-radius: 6px; color: #555; text-decoration: none; font-weight: bold; transition: 0.2s; font-size: 0.95rem; }
    .set-tab:hover { background: #e2e6ea; }
    .set-tab.active { background: #3498db; color: white; border-color: #3498db; box-shadow: 0 2px 4px rgba(52,152,219,0.3); }
  </style>
  )rawliteral"

#define SAVE_BTN \
  R"rawliteral(<div style='text-align:center;padding-top:15px'><button type='submit' class="btn btn-primary" style="padding:12px 40px;font-size:1.1rem;box-shadow:0 4px 6px rgba(52,152,219,0.3);">💾 Save Settings</button></div></form></div>)rawliteral"

#define SAVE_ALERT \
  R"rawliteral(<div style='grid-column:span 2;text-align:center;padding-bottom:15px;' class="%SAVEDCLASS%"><p style="color:#28a745;font-weight:bold;background:#e8f8f5;padding:10px;border-radius:4px">✅ Settings saved. Reboot to take the new settings into use.</p><button type='button' class="btn btn-primary" onclick='askReboot()'>Reboot Now</button></div>)rawliteral"
  
// =======================================================
// 🔋 Page 1: Battery & Inverter (URL: /settings)
// =======================================================
#define BATT_BODY \
  R"rawliteral(
  <div class="set-tabs"><a href="/settings" class="set-tab active">🔋 Battery & Inverter</a><a href="/set_network" class="set-tab">📡 Network</a><a href="/set_hardware" class="set-tab">⚙️ Hardware</a><a href="/set_web" class="set-tab">🌐 Admin & Debug</a><a href="/set_overrides" class="set-tab" style="background:#fdf2f2; border-color:#fadbd8; color:#c0392b;">⚡ Overrides</a></div>
  <div class="card card-warning"><form action='/saveSettings' method='post'><input type='hidden' name='PAGE_ID' value='/settings'>
  )rawliteral" SAVE_ALERT R"rawliteral(
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>🔋 Battery config</h3>
    <div class="form-grid">
      <label>Battery:</label><select name='battery'>%BATTTYPE%</select>
      <div class="if-nissan"><label>Interlock required:</label><input type='checkbox' name='INTERLOCKREQ' value='on' %INTERLOCKREQ% /></div>
      <div class="if-tesla">
        <label>Digital HVIL (2024+):</label><input type='checkbox' name='DIGITALHVIL' value='on' %DIGITALHVIL% />
        <label>Right hand drive:</label><input type='checkbox' name='GTWRHD' value='on' %GTWRHD% />
        <label>Country code:</label><select name='GTWCOUNTRY'>%GTWCOUNTRY%</select>
        <label>Map region:</label><select name='GTWMAPREG'>%GTWMAPREG%</select>
        <label>Chassis type:</label><select name='GTWCHASSIS'>%GTWCHASSIS%</select>
        <label>Pack type:</label><select name='GTWPACK'>%GTWPACK%</select>
      </div>
      <div class="if-estimated">
        <label>Manual charging power (W):</label><input type='number' name='CHGPOWER' value="%CHGPOWER%"/>
        <label>Manual discharge power (W):</label><input type='number' name='DCHGPOWER' value="%DCHGPOWER%"/>
      </div>
      <div class="if-socestimated"><label>Use estimated SOC:</label><input type='checkbox' name='SOCESTIMATED' value='on' %SOCESTIMATED% /></div>
      <div class="if-battery">
        <label>Battery interface:</label><select name='BATTCOMM'>%BATTCOMM%</select>
        <label>Battery chemistry:</label><select name='BATTCHEM'>%BATTCHEM%</select>
      </div>
      <div class="if-pylon-battery"><label>Pylon CAN baudrate:</label><select name='PYLONBAUD'><option value='250' %PYLONBAUD250%>250 kbps</option><option value='500' %PYLONBAUD500%>500 kbps</option></select></div>
      <div class="if-cbms">
        <label>Battery max design voltage (V):</label><input name='BATTPVMAX' type='text' value='%BATTPVMAX%'/>
        <label>Battery min design voltage (V):</label><input name='BATTPVMIN' type='text' value='%BATTPVMIN%'/>
        <label>Cell max design voltage (mV):</label><input name='BATTCVMAX' type='text' value='%BATTCVMAX%'/>
        <label>Cell min design voltage (mV):</label><input name='BATTCVMIN' type='text' value='%BATTCVMIN%'/>
      </div>
      <label>Double battery:</label><input type='checkbox' name='DBLBTR' value='on' %DBLBTR% />
      <div class="if-dblbtr">
        <label>Battery 2 interface:</label><select name='BATT2COMM'>%BATT2COMM%</select>
        <label>Triple battery:</label><input type='checkbox' name='TRIBTR' value='on' %TRIBTR% />
        <div class="if-tribtr"><label>Battery 3 interface:</label><select name='BATT3COMM'>%BATT3COMM%</select></div>
      </div>
    </div>
  </div>
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>🔌 Inverter config</h3>
    <div class="form-grid">
      <label>Inverter protocol:</label><select name='inverter'>%INVTYPE%</select>
      <div class="if-inverter"><label>Inverter interface:</label><select name='INVCOMM'>%INVCOMM%</select></div>
      <div class="if-sofar"><label>Sofar Battery ID (0-15):</label><input name='SOFAR_ID' type='text' value="%SOFAR_ID%"/></div>
      <div class="if-pylon-inverter">
        <label>Pylon, send group (0-1):</label><input name='PYLONSEND' type='text' value="%PYLONSEND%"/>
        <label>Pylon, 30k offset:</label><input type='checkbox' name='PYLONOFFSET' value='on' %PYLONOFFSET% />
        <label>Pylon, invert byteorder:</label><input type='checkbox' name='PYLONORDER' value='on' %PYLONORDER% />
      </div>
      <div class="if-byd"><label>Deye avoid over/undercharge fix:</label><input type='checkbox' name='DEYEBYD' value='on' %DEYEBYD% /></div>
      <div class="if-pylonish"><label>Reported cell count:</label><input name='INVCELLS' type='text' value="%INVCELLS%"/></div>
      <div class="if-pylonish if-solax"><label>Reported module count:</label><input name='INVMODULES' type='text' value="%INVMODULES%"/></div>
      <div class="if-pylonish">
        <label>Reported cells per module:</label><input name='INVCELLSPER' type='text' value="%INVCELLSPER%"/>
        <label>Reported voltage level:</label><input name='INVVLEVEL' type='text' value="%INVVLEVEL%"/>
        <label>Reported Ah capacity:</label><input name='INVCAPACITY' type='text' value="%INVCAPACITY%"/>
      </div>
      <div class="if-solax"><label>Reported battery type:</label><input name='INVBTYPE' type='text' value="%INVBTYPE%"/></div>
      <div class="if-sungrow"><label>Battery model:</label><select name='INVSUNTYPE'>%SUNGROW_MODEL%</select></div>
      <div class="if-kostal if-solax"><label>Prevent inverter opening contactors:</label><input type='checkbox' name='INVICNT' value='on' %INVICNT% /></div>
    </div>
  </div>
  )rawliteral" SAVE_BTN

// =======================================================
// 📡 Page 2: Network & MQTT (URL: /set_network)
// =======================================================
#define NET_BODY \
  R"rawliteral(
  <div class="set-tabs"><a href="/settings" class="set-tab">🔋 Battery & Inverter</a><a href="/set_network" class="set-tab active">📡 Network</a><a href="/set_hardware" class="set-tab">⚙️ Hardware</a><a href="/set_web" class="set-tab">🌐 Admin & Debug</a><a href="/set_overrides" class="set-tab" style="background:#fdf2f2; border-color:#fadbd8; color:#c0392b;">⚡ Overrides</a></div>
  <div class="card card-warning"><form action='/saveSettings' method='post'><input type='hidden' name='PAGE_ID' value='/set_network'>
  )rawliteral" SAVE_ALERT R"rawliteral(
  
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc; border-top: 4px solid #3498db;">
    <h3 style="color:#2c3e50; margin-bottom:5px;">🚀 Smart Connect (WiFi)</h3>
    <p style="font-size:0.85rem; color:#7f8c8d; margin-bottom:15px; margin-top:0;">Scan for networks or enter manually to connect instantly.</p>
    <div class="form-grid">
      <label>Scan Networks:</label>
      <div style="display:flex; gap:8px; max-width:250px; flex:1;">
        <select id="scanned_ssids" onchange="selectScannedNet()" style="flex:1; width:auto;">
          <option value="">-- Click Scan --</option>
        </select>
        <button type="button" onclick="scanWifi()" style="background:#6c757d; color:#fff; border:none; padding:6px 12px; border-radius:4px; cursor:pointer;">🔍</button>
      </div>

      <label>SSID:</label><input type='text' id='net_ssid' name='SSID' value="%SSID%"/>
      <label>Password:</label><input type='password' id='net_pass' name='PASSWORD' value="%PASSWORD%"/>
      
      <label></label>
      <button type="button" onclick="applyWifi()" style="background:#28a745; color:white; border:none; padding:10px 15px; border-radius:4px; font-weight:bold; cursor:pointer; box-shadow:0 2px 4px rgba(0,0,0,0.1); transition:0.2s; max-width: 250px; display:block; flex:1;">🔗 Connect & Save</button>
    </div>
  </div>

  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>📡 Connectivity Details</h3>
    <div class="form-grid">
      <label>Broadcast Wifi AP:</label><input type='checkbox' id='chk_ap' name='WIFIAPENABLED' value='on' %WIFIAPENABLED% />
      <label>AP name:</label><input type='text' name='APNAME' value="%APNAME%"/>
      <label>AP password:</label><input type='password' name='APPASSWORD' value="%APPASSWORD%"/>
      <label>Wifi channel 0-14:</label><input type='number' name='WIFICHANNEL' value="%WIFICHANNEL%"/>
      <label>Custom Wifi hostname:</label><input type='text' name='HOSTNAME' value="%HOSTNAME%"/>
      <label>Use static IP address:</label><input type='checkbox' name='STATICIP' value='on' %STATICIP% />
      <div class='if-staticip'>
        <div style="grid-column:span 2"><strong>Local IP:</strong> <input type="number" name="LOCALIP1" value="%LOCALIP1%" style="width:50px">.<input type="number" name="LOCALIP2" value="%LOCALIP2%" style="width:50px">.<input type="number" name="LOCALIP3" value="%LOCALIP3%" style="width:50px">.<input type="number" name="LOCALIP4" value="%LOCALIP4%" style="width:50px"></div>
        <div style="grid-column:span 2"><strong>Gateway:</strong> <input type="number" name="GATEWAY1" value="%GATEWAY1%" style="width:50px">.<input type="number" name="GATEWAY2" value="%GATEWAY2%" style="width:50px">.<input type="number" name="GATEWAY3" value="%GATEWAY3%" style="width:50px">.<input type="number" name="GATEWAY4" value="%GATEWAY4%" style="width:50px"></div>
        <div style="grid-column:span 2"><strong>Subnet:</strong> <input type="number" name="SUBNET1" value="%SUBNET1%" style="width:50px">.<input type="number" name="SUBNET2" value="%SUBNET2%" style="width:50px">.<input type="number" name="SUBNET3" value="%SUBNET3%" style="width:50px">.<input type="number" name="SUBNET4" value="%SUBNET4%" style="width:50px"></div>
      </div>
      <label>Enable ESPNow:</label><input type='checkbox' name='ESPNOWENABLED' value='on' %ESPNOWENABLED% />
      <label>Enable MQTT:</label><input type='checkbox' name='MQTTENABLED' value='on' %MQTTENABLED% />
      <div class='if-mqtt'>
        <label>MQTT server:</label><input type='text' name='MQTTSERVER' value="%MQTTSERVER%"/>
        <label>MQTT port:</label><input type='number' name='MQTTPORT' value="%MQTTPORT%"/>
        <label>MQTT user:</label><input type='text' name='MQTTUSER' value="%MQTTUSER%"/>
        <label>MQTT password:</label><input type='password' name='MQTTPASSWORD' value="%MQTTPASSWORD%"/>
        <label>MQTT timeout ms:</label><input name='MQTTTIMEOUT' type='number' value="%MQTTTIMEOUT%"/>
        <label>MQTT publish interval (s):</label><input name='MQTTPUBLISHMS' type='number' value="%MQTTPUBLISHMS%"/>
        <label>Send all cellvoltages:</label><input type='checkbox' name='MQTTCELLV' value='on' %MQTTCELLV% />
        <label>Remote BMS reset allowed:</label><input type='checkbox' name='REMBMSRESET' value='on' %REMBMSRESET% />
        <label>Custom MQTT topics:</label><input type='checkbox' name='MQTTTOPICS' value='on' %MQTTTOPICS% />
        <div class='if-topics'>
          <label>Topic name:</label><input type='text' name='MQTTTOPIC' value="%MQTTTOPIC%"/>
          <label>Prefix for object ID:</label><input type='text' name='MQTTOBJIDPREFIX' value="%MQTTOBJIDPREFIX%"/>
          <label>HA device name:</label><input type='text' name='MQTTDEVICENAME' value="%MQTTDEVICENAME%"/>
          <label>HA device ID:</label><input type='text' name='HADEVICEID' value="%HADEVICEID%"/>
        </div>
        <label>HA auto discovery:</label><input type='checkbox' name='HADISC' value='on' %HADISC% />
      </div>
    </div>
  </div>
  )rawliteral" SAVE_BTN

// =======================================================
// ⚙️ Page 3: Hardware (URL: /set_hardware)
// =======================================================
#define HW_BODY \
  R"rawliteral(
  <div class="set-tabs"><a href="/settings" class="set-tab">🔋 Battery & Inverter</a><a href="/set_network" class="set-tab">📡 Network</a><a href="/set_hardware" class="set-tab active">⚙️ Hardware</a><a href="/set_web" class="set-tab">🌐 Admin & Debug</a><a href="/set_overrides" class="set-tab" style="background:#fdf2f2; border-color:#fadbd8; color:#c0392b;">⚡ Overrides</a></div>
  <div class="card card-warning"><form action='/saveSettings' method='post'><input type='hidden' name='PAGE_ID' value='/set_hardware'>
  )rawliteral" SAVE_ALERT R"rawliteral(
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>⚡ Optional components</h3>
    <div class="form-grid">
      <label>Charger:</label><select name='charger'>%CHGTYPE%</select>
      <div class="if-charger"><label>Charger interface:</label><select name='CHGCOMM'>%CHGCOMM%</select></div>
      <label>Shunt:</label><select name='SHUNTTYPE'>%SHUNTTYPE%</select>
      <div class="if-shunt"><label>Shunt interface:</label><select name='SHUNTCOMM'>%SHUNTCOMM%</select></div>
      <div class="if-ctclamp">
        <label>CT Clamp offset (mV):</label>
        <input type='number' name='CTOFFSET' value="%CTOFFSET%" min="-1" max="3000" step="1" title="Voltage offset required to calibrate 0A reading. -1 = auto-detect" />
        <label>CT Clamp nominal voltage (dV):</label>
        <input type='number' name='CTVNOM' value="%CTVNOM%" min="0" max="500" step="1" title="Nominal voltage of the CT Clamp x10. Integer only." />
        <label>CT Clamp nominal current (A):</label>
        <input type='number' name='CTANOM' value="%CTANOM%" min="0" max="200" step="1" title="Nominal current of the CT Clamp. Integer only." />
        <label>ESP32 pin attenuation:</label>
        <select name='CTATTEN'>%CTATTEN%</select>
        <label>Invert CT current:</label>
        <input type='checkbox' name='CTINVERT' value='on' %CTINVERT% title="Invert the current reading from the CT clamp, +ve is charging, -ve is discharging" />
      </div>
    </div>
  </div>
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>⚙️ Hardware config</h3>
    <div class="form-grid">
      <label>Use CanFD as classic CAN:</label><input type='checkbox' name='CANFDASCAN' value='on' %CANFDASCAN% />
      <label>CAN addon crystal (Mhz):</label><input type='number' name='CANFREQ' value="%CANFREQ%"/>
      <label>CAN-FD-addon crystal (Mhz):</label><input type='number' name='CANFDFREQ' value="%CANFDFREQ%"/>
      <label>Equipment stop button:</label><select name='EQSTOP'>%EQSTOP%</select>
      <div class="if-dblbtr">
        <label>Double-Bat Contactor via GPIO:</label><input type='checkbox' name='CNTCTRLDBL' value='on' %CNTCTRLDBL% />
        <label>Triple-Bat Contactor via GPIO:</label><input type='checkbox' name='CNTCTRLTRI' value='on' %CNTCTRLTRI% />
      </div>
      <label>Contactor control via GPIO:</label><input type='checkbox' name='CNTCTRL' value='on' %CNTCTRL% />
      <div class="if-cntctrl">
        <label>Precharge time ms:</label><input type='number' name='PRECHGMS' value="%PRECHGMS%"/>
        <label>Normally Closed logic:</label><input type='checkbox' name='NCCONTACTOR' value='on' %NCCONTACTOR% />
        <label>PWM contactor control:</label><input type='checkbox' name='PWMCNTCTRL' value='on' %PWMCNTCTRL% />
        <div class="if-pwmcntctrl">
          <label>PWM Frequency Hz:</label><input name='PWMFREQ' type='number' value="%PWMFREQ%"/>
          <label>PWM Hold 1-1023:</label><input type='number' name='PWMHOLD' value="%PWMHOLD%"/>
        </div>
      </div>
      <label>Periodic BMS reset every 24h:</label><input type='checkbox' name='PERBMSRESET' value='on' %PERBMSRESET% />
      <label>External precharge via HIA4V1:</label><input type='checkbox' name='EXTPRECHARGE' value='on' %EXTPRECHARGE% />
      <div class="if-extprecharge">
        <label>Precharge max ms before fault:</label><input name='MAXPRETIME' type='number' value="%MAXPRETIME%"/>
        <label>Precharge max PWM frequency:</label><input name='MAXPREFREQ' type='number' value="%MAXPREFREQ%"/>
        <label>Normally Open (NO) inverter disconnect:</label><input type='checkbox' name='NOINVDISC' value='on' %NOINVDISC% />
      </div>
      <label>Status LED pattern:</label><select name='LEDMODE' id='LEDMODE' onchange="checkLedMode()">%LEDMODE%</select>
      <label>Number of LEDs (WS2812B):</label><input type='number' id='LEDCOUNT' name='LEDCOUNT' value="%LEDCOUNT%" oninput="checkLedPower()"/>
      <div id="led_power_warning" style="padding:10px;font-size:.9em;grid-column:span 2">⚡ Max Current: <strong id="led_mA">--</strong> <strong>mA</strong> <span id="led_msg"></span></div>
      <label id="lbl_ledtail">Energy Flow Tail Length:</label><input id="input_ledtail" type='number' name='LEDTAIL' value="%LEDTAIL%"/>
      )rawliteral" GPIOOPT1_SETTING R"rawliteral()rawliteral" GPIOOPT2_SETTING R"rawliteral()rawliteral" GPIOOPT3_SETTING R"rawliteral()rawliteral" GPIOOPT4_SETTING R"rawliteral()rawliteral" DISPLAY_SETTING R"rawliteral(
    </div>
  </div>
  )rawliteral" SAVE_BTN

// =======================================================
// 🌐 Page 4: Admin & Debug (URL: /set_web)
// =======================================================
#define WEB_BODY \
  R"rawliteral(
  <div class="set-tabs"><a href="/settings" class="set-tab">🔋 Battery & Inverter</a><a href="/set_network" class="set-tab">📡 Network</a><a href="/set_hardware" class="set-tab">⚙️ Hardware</a><a href="/set_web" class="set-tab active">🌐 Admin & Debug</a><a href="/set_overrides" class="set-tab" style="background:#fdf2f2; border-color:#fadbd8; color:#c0392b;">⚡ Overrides</a></div>
  <div style="display:flex;justify-content:flex-end;margin-bottom:15px"><button class="btn btn-danger" onclick="askFactoryReset()">⚠️ Factory Reset</button></div>
  <div class="card card-warning"><form action='/saveSettings' method='post'><input type='hidden' name='PAGE_ID' value='/set_web'>
  )rawliteral" SAVE_ALERT R"rawliteral(
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>🌐 Webpage config</h3>
    <div class="form-grid">
      <label>Require Web Login:</label><select name="WEBAUTH"><option value="1" %WEBAUTH_1%>Enable</option><option value="0" %WEBAUTH_0%>Disable</option></select>
      <div class="if-auth">
        <label>Admin Username:</label><input type='text' name='WEBUSER' value="%WEBUSER%"/>
        <label>Admin Password:</label><input type='password' name='WEBPASS' value="%WEBPASS%"/>
      </div>
    </div>
  </div>
  <div class="card" style="box-shadow:none;border:1px solid #eee;margin-bottom:20px;background:#fcfcfc">
    <h3>🐞 Debug options</h3>
    <div class="form-grid">
      <label>Enable performance profiling:</label><input type='checkbox' name='PERFPROFILE' value='on' %PERFPROFILE% />
      <label>CAN log via USB serial:</label><input type='checkbox' name='CANLOGUSB' value='on' %CANLOGUSB% />
      <label>General log via USB serial:</label><input type='checkbox' name='USBENABLED' value='on' %USBENABLED% />
      <label>General log via Webserver:</label><input type='checkbox' name='WEBENABLED' value='on' %WEBENABLED% />
      <label>CAN log via SD card:</label><input type='checkbox' name='CANLOGSD' value='on' %CANLOGSD% />
      <label>General log via SD card:</label><input type='checkbox' name='SDLOGENABLED' value='on' %SDLOGENABLED% />
    </div>
  </div>
  )rawliteral" SAVE_BTN

// =======================================================
// ⚡ Page 5: Manual Overrides (URL: /set_overrides)
// =======================================================
#define OVERRIDE_BODY \
  R"rawliteral(
  <div class="set-tabs"><a href="/settings" class="set-tab">🔋 Battery & Inverter</a><a href="/set_network" class="set-tab">📡 Network</a><a href="/set_hardware" class="set-tab">⚙️ Hardware</a><a href="/set_web" class="set-tab">🌐 Admin & Debug</a><a href="/set_overrides" class="set-tab active" style="background:#e74c3c; border-color:#e74c3c; color:#fff;">⚡ Overrides</a></div>
  
  <div class="card card-danger">
    <h3 style="color:#e74c3c">⚡ Manual Overrides (Live Update)</h3>
    <div class="override-grid"><h4>Battery capacity: <strong style="color:#3498db">%BATTERY_WH_MAX% Wh</strong></h4><button class="btn btn-primary" onclick='editWh()'>Edit</button></div>
    <div class="override-grid"><h4>Rescale SOC: <strong class='%SOC_SCALING_CLASS%'>%SOC_SCALING%</strong></h4><button class="btn btn-primary" onclick='editUseScaledSOC()'>Edit</button></div>
    <div class="%SOC_SCALING_ACTIVE_CLASS%">
      <div class="override-grid" style="padding-left:20px;background:#fdfdfd"><h4>↳ SOC max: <strong>%SOC_MAX_PERCENTAGE% &#37;</strong></h4><button class="btn btn-primary" onclick='editSocMax()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px;background:#fdfdfd"><h4>↳ SOC min: <strong>%SOC_MIN_PERCENTAGE% &#37;</strong></h4><button class="btn btn-primary" onclick='editSocMin()'>Edit</button></div>
    </div>
    <div class="override-grid"><h4>Max charge speed: <strong>%MAX_CHARGE_SPEED% A</strong></h4><button class="btn btn-primary" onclick='editMaxChargeA()'>Edit</button></div>
    <div class="override-grid"><h4>Max discharge speed: <strong>%MAX_DISCHARGE_SPEED% A</strong></h4><button class="btn btn-primary" onclick='editMaxDischargeA()'>Edit</button></div>
    <div class="override-grid"><h4>Manual charge voltage limits: <strong class='%VOLTAGE_LIMITS_CLASS%'>%VOLTAGE_LIMITS%</strong></h4><button class="btn btn-primary" onclick='editUseVoltageLimit()'>Edit</button></div>
    <div class="%VOLTAGE_LIMITS_ACTIVE_CLASS%">
      <div class="override-grid" style="padding-left:20px;background:#fdfdfd"><h4>↳ Target charge: <strong>%CHARGE_VOLTAGE% V</strong></h4><button class="btn btn-primary" onclick='editMaxChargeVoltage()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px;background:#fdfdfd"><h4>↳ Target discharge: <strong>%DISCHARGE_VOLTAGE% V</strong></h4><button class="btn btn-primary" onclick='editMaxDischargeVoltage()'>Edit</button></div>
    </div>
    <div class="override-grid"><h4>Periodic BMS reset off time: <strong>%BMS_RESET_DURATION% s</strong></h4><button class="btn btn-primary" onclick='editBMSresetDuration()'>Edit</button></div>
    <div class="override-grid" style="background:#fdf2f2"><h4 style="color:#c0392b">🚨 Undercharged emergency recovery:</h4><button class="btn btn-danger" onclick='editRecoveryMode()'>Start</button></div>
  </div>
  
  <div class="card card-warning %FAKE_VOLTAGE_CLASS%">
    <div class="override-grid" style="border:none"><h4>Fake battery voltage: <strong>%BATTERY_VOLTAGE% V</strong></h4><button class="btn btn-warning" onclick='editFakeBatteryVoltage()'>Edit</button></div>
  </div>
  
  <div class="card card-warning %MANUAL_BAL_CLASS%">
    <h3 style="color:#f39c12">⚖️ LFP Manual Balancing</h3>
    <div class="override-grid"><h4>Manual LFP balancing: <strong class="%MANUAL_BALANCING_CLASS%">%MANUAL_BALANCING%</strong></h4><button class="btn btn-primary" onclick='editTeslaBalAct()'>Edit</button></div>
    <div class="%BALANCING_CLASS%">
      <div class="override-grid" style="padding-left:20px"><h4>↳ Max time: <strong>%BAL_MAX_TIME% Min</strong></h4><button class="btn btn-primary" onclick='editBalTime()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px"><h4>↳ Float power: <strong>%BAL_POWER% W</strong></h4><button class="btn btn-primary" onclick='editBalFloatPower()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px"><h4>↳ Max pack voltage: <strong>%BAL_MAX_PACK_VOLTAGE% V</strong></h4><button class="btn btn-primary" onclick='editBalMaxPackV()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px"><h4>↳ Max cell voltage: <strong>%BAL_MAX_CELL_VOLTAGE% mV</strong></h4><button class="btn btn-primary" onclick='editBalMaxCellV()'>Edit</button></div>
      <div class="override-grid" style="padding-left:20px"><h4>↳ Max cell deviation: <strong>%BAL_MAX_DEV_CELL_VOLTAGE% mV</strong></h4><button class="btn btn-primary" onclick='editBalMaxDevCellV()'>Edit</button></div>
    </div>
  </div>
  
  <div class="card card-warning %CHARGER_CLASS%">
    <h3 style="color:#f39c12">🔌 Charger Setpoints</h3>
    <div class="override-grid"><h4>Charger HVDC: <strong class="%CHG_HV_CLASS%">%CHG_HV%</strong></h4><button class="btn btn-primary" onclick='editChargerHVDCEnabled()'>Edit</button></div>
    <div class="override-grid"><h4>Charger Aux12VDC: <strong class="%CHG_AUX12V_CLASS%">%CHG_AUX12V%</strong></h4><button class="btn btn-primary" onclick='editChargerAux12vEnabled()'>Edit</button></div>
    <div class="override-grid"><h4>Voltage Setpoint: <strong>%CHG_VOLTAGE_SETPOINT% V</strong></h4><button class="btn btn-primary" onclick='editChargerSetpointVDC()'>Edit</button></div>
    <div class="override-grid"><h4>Current Setpoint: <strong>%CHG_CURRENT_SETPOINT% A</strong></h4><button class="btn btn-primary" onclick='editChargerSetpointIDC()'>Edit</button></div>
  </div>
  
  <script>
    var frm = document.querySelector("form");
    if (frm) {
      document.querySelectorAll("select,input").forEach(function(e) {
        function t() {
          if (e.name) frm.setAttribute("data-" + e.name.toLowerCase(), e.type == "checkbox" ? e.checked : e.value);
        }
        e.addEventListener("change", t);
        t();
      });
    }
  </script>
  )rawliteral"

// =======================================================
// Combine HTML for WebServer
// =======================================================
const char settings_batt_html[] = INDEX_HTML_HEADER TABS_CSS SETTINGS_STYLE BATT_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
const char settings_net_html[] = INDEX_HTML_HEADER TABS_CSS SETTINGS_STYLE NET_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
const char settings_hw_html[] = INDEX_HTML_HEADER TABS_CSS SETTINGS_STYLE HW_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
const char settings_web_html[] = INDEX_HTML_HEADER TABS_CSS SETTINGS_STYLE WEB_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
const char settings_overrides_html[] = INDEX_HTML_HEADER TABS_CSS SETTINGS_STYLE OVERRIDE_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
