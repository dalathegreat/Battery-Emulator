#include "settings_html.h"
#include <Arduino.h>
#include "../../../src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../include.h"
#include "index_html.h"

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
    options +=
        ("<option value=\"" + String(static_cast<int>(type)) + "\"" + (selected == type ? " selected" : "") + ">");
    options += name;
    options += "</option>";
  }
  return options;
}

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

String settings_processor(const String& var, BatteryEmulatorSettingsStore& settings) {

  if (var == "HOSTNAME") {
    return settings.getString("HOSTNAME");
  }

  if (var == "BATTERYINTF") {
    if (battery) {
      return battery->interface_name();
    }
  }
  if (var == "SSID") {
    return String(ssid.c_str());
  }

#ifndef COMMON_IMAGE
  if (var == "COMMONIMAGEDIVCLASS") {
    return "hidden";
  }
#endif

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
    return options_for_enum((battery_chemistry_enum)settings.getUInt("BATTCHEM", (int)battery_chemistry_enum::NCA),
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

  if (var == "DBLBTR") {
    return settings.getBool("DBLBTR") ? "checked" : "";
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

  if (var == "CANFDASCAN") {
    return settings.getBool("CANFDASCAN") ? "checked" : "";
  }

  if (var == "WIFIAPENABLED") {
    return settings.getBool("WIFIAPENABLED", wifiap_enabled) ? "checked" : "";
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

  if (var == "MQTTOBJIDPREFIX") {
    return settings.getString("MQTTOBJIDPREFIX");
  }

  if (var == "MQTTDEVICENAME") {
    return settings.getString("MQTTDEVICENAME");
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
    return datalayer.battery.settings.soc_scaling_active ? "&#10003;" : "&#10005;";
  }

  if (var == "FAKE_VOLTAGE_CLASS") {
    return battery && battery->supports_set_fake_voltage() ? "" : "hidden";
  }

  if (var == "MANUAL_BALANCING_CLASS") {
    return datalayer.battery.settings.user_requests_balancing ? "" : "inactiveSoc";
  }

  if (var == "MANUAL_BALANCING") {
    if (datalayer.battery.settings.user_requests_balancing) {
      return "&#10003;";
    } else {
      return "&#10005;";
    }
  }

  if (var == "BATTERY_VOLTAGE") {
    if (battery) {
      return String(battery->get_voltage(), 1);
    }
  }

  if (var == "VOLTAGE_LIMITS") {
    if (datalayer.battery.settings.user_set_voltage_limits_active) {
      return "&#10003;";
    } else {
      return "&#10005;";
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
      return "&#10003;";
    } else {
      return "&#10005;";
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
      return "&#10003;";
    } else {
      return "&#10005;";
    }
  }

  if (var == "CHG_VOLTAGE_SETPOINT") {
    return String(datalayer.charger.charger_setpoint_HV_VDC, 1);
  }

  if (var == "CHG_CURRENT_SETPOINT") {
    return String(datalayer.charger.charger_setpoint_HV_IDC, 1);
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

        function editSSID(){var value=prompt('Enter new SSID:');if(value!==null){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateSSID?value='+encodeURIComponent(value),true);xhr.send();}}
        
        function editPassword(){var value=prompt('Enter new password:');if(value!==null){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updatePassword?value='+encodeURIComponent(value),true);xhr.send();}}
    
        function editWh(){var value=prompt('How much energy the battery can store. Enter new Wh value (1-120000):');
          if(value!==null){if(value>=1&&value<=120000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateBatterySize?value='+value,true);xhr.send();}else{
          alert('Invalid value. Please enter a value between 1 and 120000.');}}}

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

          function toggleMqtt() {
            var mqttEnabled = document.querySelector('input[name="MQTTENABLED"]').checked;
            document.querySelectorAll('.mqtt-settings').forEach(function (el) {
              el.style.display = mqttEnabled ? 'contents' : 'none';
            });
          }

          document.addEventListener('DOMContentLoaded', toggleMqtt);

          function toggleTopics() {
            var topicsEnabled = document.querySelector('input[name="MQTTTOPICS"]').checked;
            document.querySelectorAll('.mqtt-topics').forEach(function (el) {
              el.style.display = topicsEnabled ? 'contents' : 'none';
            });
          }

          document.addEventListener('DOMContentLoaded', toggleTopics);
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
    select { max-width: 250px; }
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

    </style>
)rawliteral"

#define SETTINGS_HTML_BODY \
  R"rawliteral(
  <button onclick='goToMainPage()'>Back to main page</button>

  <div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>
    <h4 style='color: white;'>SSID: <span id='SSID'>%SSID%</span><button onclick='editSSID()'>Edit</button></h4>
    <h4 style='color: white;'>Password: ######## <span id='Password'></span> <button onclick='editPassword()'>Edit</button></h4>
    
    <div style='background-color: #404E47; padding: 10px; margin-bottom: 10px;border-radius: 50px; class="%COMMONIMAGEDIVCLASS%">
    <div style='max-width: 500px;'>
        <form action='saveSettings' method='post' style='display: grid; grid-template-columns: 1fr 2fr; gap: 10px;
        align-items: center;'>

        <label for='battery'>Battery: </label><select name='battery' if='battery'>
        %BATTTYPE%
        </select>

        <label for='BATTCOMM'>Battery comm I/F: </label><select name='BATTCOMM' id='BATTCOMM'>
        %BATTCOMM%
        </select>

        <label>Battery chemistry: </label><select name='BATTCHEM'>
        %BATTCHEM%
        </select>

        <label>Inverter protocol: </label><select name='inverter'>
        %INVTYPE%
        </select>

        <label>Inverter comm I/F: </label><select name='INVCOMM'>
        %INVCOMM%     
        </select>

        <label>Charger: </label><select name='charger'>
        %CHGTYPE%
        </select>

        <label>Charger comm I/F: </label><select name='CHGCOMM'>
        %CHGCOMM%
        </select>

        <label>Shunt: </label><select name='SHUNT'>
        %SHUNTTYPE%
        </select>

        <label>Shunt comm I/F: </label><select name='SHUNTCOMM'>
        %SHUNTCOMM%
        </select>

        <label>Equipment stop button: </label><select name='EQSTOP'>
        %EQSTOP%  
        </select>

        <label>Double battery: </label>
        <input type='checkbox' name='DBLBTR' value='on' style='margin-left: 0;' %DBLBTR% />

        <label>Battery 2 comm I/F: </label><select name='BATT2COMM'>
        %BATT2COMM%
        </select>

        <label>Contactor control: </label>
        <input type='checkbox' name='CNTCTRL' value='on' style='margin-left: 0;' %CNTCTRL% />

        <label>Contactor control double battery: </label>
        <input type='checkbox' name='CNTCTRLDBL' value='on' style='margin-left: 0;' %CNTCTRLDBL% />

        <label>PWM contactor control: </label>
        <input type='checkbox' name='PWMCNTCTRL' value='on' style='margin-left: 0;' %PWMCNTCTRL% />

        <label>Periodic BMS reset: </label>
        <input type='checkbox' name='PERBMSRESET' value='on' style='margin-left: 0;' %PERBMSRESET% /> 

        <label>Remote BMS reset: </label>
        <input type='checkbox' name='REMBMSRESET' value='on' style='margin-left: 0;' %REMBMSRESET% />

        <label>Use CanFD as classic CAN: </label>
        <input type='checkbox' name='CANFDASCAN' value='on' style='margin-left: 0;' %CANFDASCAN% /> 

        <label>Enable WiFi AP: </label>
        <input type='checkbox' name='WIFIAPENABLED' value='on' style='margin-left: 0;' %WIFIAPENABLED% />

        <label>Custom hostname: </label>
        <input style='max-width: 250px;' type='text' name='HOSTNAME' value="%HOSTNAME%" />

        <label>Enable MQTT: </label>
        <input type='checkbox' name='MQTTENABLED' value='on' onchange='toggleMqtt()' style='margin-left: 0;' %MQTTENABLED% />

        <div class='mqtt-settings'>
        <label>MQTT server: </label><input style='max-width: 250px;' type='text' name='MQTTSERVER' value="%MQTTSERVER%" />
        <label>MQTT port: </label><input style='max-width: 250px;' type='text' name='MQTTPORT' value="%MQTTPORT%" />
        <label>MQTT user: </label><input style='max-width: 250px;' type='text' name='MQTTUSER' value="%MQTTUSER%" />
        <label>MQTT password: </label><input style='max-width: 250px;' type='password' name='MQTTPASSWORD' value="%MQTTPASSWORD%" />

        <label>Customized MQTT topics: </label>
        <input type='checkbox' name='MQTTTOPICS' value='on' onchange='toggleTopics()' style='margin-left: 0;' %MQTTTOPICS% />

        <div class='mqtt-topics'>

        <label>MQTT topic name: </label><input style='max-width: 250px;' type='text' name='MQTTTOPIC' value="%MQTTTOPIC%" />
        <label>Prefix for MQTT object ID: </label><input style='max-width: 250px;' type='text' name='MQTTOBJIDPREFIX' value="%MQTTOBJIDPREFIX%" />
        <label>HA device name: </label><input style='max-width: 250px;' type='text' name='MQTTDEVICENAME' value="%MQTTDEVICENAME%" />
        <label>HA device ID: </label><input style='max-width: 250px;' type='text' name='HADEVICEID' value="%HADEVICEID%" />

        </div>

        <label>Enable Home Assistant auto discovery: </label>
        <input type='checkbox' name='HADISC' value='on' style='margin-left: 0;' %HADISC% />

        </div>

        <div style='grid-column: span 2; text-align: center; padding-top: 10px;'><button "type='submit'>Save</button></div>

        <div style='grid-column: span 2; text-align: center; padding-top: 10px;' class="%SAVEDCLASS%">
          <p>Settings saved. Reboot to take the settings into use.<p> <button onclick='askReboot()'>Reboot</button>
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

      <button onclick="askFactoryReset()">Factory reset</button>
    
  </div>

)rawliteral"

const char settings_html[] =
    INDEX_HTML_HEADER COMMON_JAVASCRIPT SETTINGS_STYLE SETTINGS_HTML_BODY SETTINGS_HTML_SCRIPTS INDEX_HTML_FOOTER;
