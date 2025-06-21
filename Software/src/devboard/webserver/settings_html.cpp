#include "settings_html.h"
#include <Arduino.h>
#include "../../../src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../include.h"

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
std::vector<std::pair<String, EnumType>> enum_values_and_names(Func name_for_type) {
  auto values = enum_values<EnumType>();

  std::vector<std::pair<String, EnumType>> pairs;

  for (auto& type : values) {
    auto name = name_for_type(type);
    if (name != nullptr) {
      pairs.push_back(std::pair(String(name), type));
    }
  }

  std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

  pairs.insert(pairs.begin(), std::pair(name_for_type(EnumType::None), EnumType::None));

  return pairs;
}

template <typename TEnum, typename Func>
String options_for_enum(TEnum selected, Func name_for_type) {
  String options;
  auto values = enum_values_and_names<TEnum>(name_for_type);
  for (const auto& [name, type] : values) {
    options +=
        ("<option value=\"" + String(static_cast<int>(type)) + "\"" + (selected == type ? " selected" : "") + ">");
    options += name;
    options += "</option>";
  }
  return options;
}

void render_checkbox(String& content, const char* label, bool enabled, const char* name) {
  content += "<label>" + String(label) + "</label>";
  content += "<input id='" + String(name) + "' name='" + String(name) +
             "' type='checkbox' "
             "style=\"margin-left: 0;\"";
  content += (enabled ? " checked" : "");
  content += " value='on'/>";
}

String settings_processor(const String& var) {
  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content +=
        "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
        "cursor: pointer; border-radius: 10px; }";
    content += "button:hover { background-color: #3A4A52; }";
    content += "h4 { margin: 0.6em 0; line-height: 1.2; }";
    content += "</style>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4 style='color: white;'>SSID: <span id='SSID'>" + String(ssid.c_str()) +
               " </span> <button onclick='editSSID()'>Edit</button></h4>";
    content +=
        "<h4 style='color: white;'>Password: ######## <span id='Password'></span> <button "
        "onclick='editPassword()'>Edit</button></h4>";

#ifdef COMMON_IMAGE
    BatteryEmulatorSettingsStore settings;

    // It's important that we read/write settings directly to settings store instead of the run-time values
    // since the run-time values may have direct effect on operation.
    content +=
        "<div style='background-color: #404E47; padding: 10px; margin-bottom: 10px;border-radius: 50px;'><div "
        "style='max-width: 500px;'>";
    content +=
        "<form action='saveSettings' method='post' style='display: grid; grid-template-columns: 1fr 2fr; gap: 10px; "
        "align-items: center;'>";
    content += "<label>Battery: </label><select style='max-width: 250px;' name='battery'>";

    content +=
        options_for_enum((BatteryType)settings.getUInt("BATTTYPE", (int)BatteryType::None), name_for_battery_type);
    content += "</select>";
    content += "<label>Inverter protocol: </label><select style='max-width: 250px;' name='inverter'>";
    content += options_for_enum((InverterProtocolType)settings.getUInt("INVTYPE", (int)InverterProtocolType::None),
                                name_for_inverter_type);
    content += "</select>";
    content += "<label>Charger: </label><select style='max-width: 250px;' name='charger'>";
    content +=
        options_for_enum((ChargerType)settings.getUInt("CHGTYPE", (int)ChargerType::None), name_for_charger_type);
    content += "</select>";

    // TODO: Generalize settings: define settings in one place and use the definitions to render
    // UI and handle load/save
    render_checkbox(content, "Double battery", settings.getBool("DBLBTR"), "DBLBTR");
    render_checkbox(content, "Contactor control", settings.getBool("CNTCTRL"), "CNTCTRL");
    render_checkbox(content, "Contactor control double battery", settings.getBool("CNTCTRLDBL"), "CNTCTRLDBL");
    render_checkbox(content, "PWM contactor control", settings.getBool("PWMCNTCTRL"), "PWMCNTCTRL");
    render_checkbox(content, "Periodic BMS reset", settings.getBool("PERBMSRESET"), "PERBMSRESET");
    render_checkbox(content, "Remote BMS reset", settings.getBool("REMBMSRESET"), "REMBMSRESET");

    content +=
        "<div style='grid-column: span 2; text-align: center; padding-top: 10px;'><button "
        "type='submit'>Save</button></div>";

    if (settingsUpdated) {
      content += "<p>Settings saved. Reboot to take the settings into use.</p>";
    }

    content += "</form></div></div>";
#endif

    if (battery) {
      content += "<h4 style='color: white;'>Battery interface: <span id='Battery'>" + battery->interface_name() +
                 "</span></h4>";
    }

    if (battery2) {
      content += "<h4 style='color: white;'>Battery #2 interface: <span id='Battery'>" + battery2->interface_name() +
                 "</span></h4>";
    }

    if (inverter) {
      content += "<h4 style='color: white;'>Inverter interface: <span id='Inverter'>" +
                 String(inverter->interface_name()) + "</span></h4>";
    }

    if (shunt) {
      content +=
          "<h4 style='color: white;'>Shunt Interface: <span id='Shunt'>" + shunt->interface_name() + "</span></h4>";
    }

    // Close the block
    content += "</div>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #2D3F2F; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Show current settings with edit buttons and input fields
    content += "<h4 style='color: white;'>Battery capacity: <span id='BATTERY_WH_MAX'>" +
               String(datalayer.battery.info.total_capacity_Wh) +
               " Wh </span> <button onclick='editWh()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Rescale SOC: <span id='BATTERY_USE_SCALED_SOC'>" +
               String(datalayer.battery.settings.soc_scaling_active ? "<span>&#10003;</span>"
                                                                    : "<span style='color: red;'>&#10005;</span>") +
               "</span> <button onclick='editUseScaledSOC()'>Edit</button></h4>";
    content += "<h4 style='color: " + String(datalayer.battery.settings.soc_scaling_active ? "white" : "darkgrey") +
               ";'>SOC max percentage: " + String(datalayer.battery.settings.max_percentage / 100.0, 1) +
               " </span> <button onclick='editSocMax()'>Edit</button></h4>";
    content += "<h4 style='color: " + String(datalayer.battery.settings.soc_scaling_active ? "white" : "darkgrey") +
               ";'>SOC min percentage: " + String(datalayer.battery.settings.min_percentage / 100.0, 1) +
               " </span> <button onclick='editSocMin()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max charge speed: " +
               String(datalayer.battery.settings.max_user_set_charge_dA / 10.0, 1) +
               " A </span> <button onclick='editMaxChargeA()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max discharge speed: " +
               String(datalayer.battery.settings.max_user_set_discharge_dA / 10.0, 1) +
               " A </span> <button onclick='editMaxDischargeA()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Manual charge voltage limits: <span id='BATTERY_USE_VOLTAGE_LIMITS'>" +
               String(datalayer.battery.settings.user_set_voltage_limits_active
                          ? "<span>&#10003;</span>"
                          : "<span style='color: red;'>&#10005;</span>") +
               "</span> <button onclick='editUseVoltageLimit()'>Edit</button></h4>";
    content +=
        "<h4 style='color: " +
        String(datalayer.battery.settings.user_set_voltage_limits_active ? "white" : "darkgrey") +
        ";'>Target charge voltage: " + String(datalayer.battery.settings.max_user_set_charge_voltage_dV / 10.0, 1) +
        " V </span> <button onclick='editMaxChargeVoltage()'>Edit</button></h4>";
    content += "<h4 style='color: " +
               String(datalayer.battery.settings.user_set_voltage_limits_active ? "white" : "darkgrey") +
               ";'>Target discharge voltage: " +
               String(datalayer.battery.settings.max_user_set_discharge_voltage_dV / 10.0, 1) +
               " V </span> <button onclick='editMaxDischargeVoltage()'>Edit</button></h4>";
    // Close the block
    content += "</div>";

    if (battery && battery->supports_set_fake_voltage()) {
      content += "<div style='background-color: #2E37AD; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";
      content += "<h4 style='color: white;'>Fake battery voltage: " + String(battery->get_voltage(), 1) +
                 " V </span> <button onclick='editFakeBatteryVoltage()'>Edit</button></h4>";
      content += "</div>";
    }

    if (battery && battery->supports_manual_balancing()) {
      // Start a new block with grey background color
      content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

      content +=
          "<h4 style='color: white;'>Manual LFP balancing: <span id='TSL_BAL_ACT'>" +
          String(datalayer.battery.settings.user_requests_balancing ? "<span>&#10003;</span>"
                                                                    : "<span style='color: red;'>&#10005;</span>") +
          "</span> <button onclick='editTeslaBalAct()'>Edit</button></h4>";
      content +=
          "<h4 style='color: " + String(datalayer.battery.settings.user_requests_balancing ? "white" : "darkgrey") +
          ";'>Balancing max time: " + String(datalayer.battery.settings.balancing_time_ms / 60000.0, 1) +
          " Minutes </span> <button onclick='editBalTime()'>Edit</button></h4>";
      content +=
          "<h4 style='color: " + String(datalayer.battery.settings.user_requests_balancing ? "white" : "darkgrey") +
          ";'>Balancing float power: " + String(datalayer.battery.settings.balancing_float_power_W / 1.0, 0) +
          " W </span> <button onclick='editBalFloatPower()'>Edit</button></h4>";
      content +=
          "<h4 style='color: " + String(datalayer.battery.settings.user_requests_balancing ? "white" : "darkgrey") +
          ";'>Max battery voltage: " + String(datalayer.battery.settings.balancing_max_pack_voltage_dV / 10.0, 0) +
          " V </span> <button onclick='editBalMaxPackV()'>Edit</button></h4>";
      content +=
          "<h4 style='color: " + String(datalayer.battery.settings.user_requests_balancing ? "white" : "darkgrey") +
          ";'>Max cell voltage: " + String(datalayer.battery.settings.balancing_max_cell_voltage_mV / 1.0, 0) +
          " mV </span> <button onclick='editBalMaxCellV()'>Edit</button></h4>";
      content +=
          "<h4 style='color: " + String(datalayer.battery.settings.user_requests_balancing ? "white" : "darkgrey") +
          ";'>Max cell voltage deviation: " +
          String(datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV / 1.0, 0) +
          " mV </span> <button onclick='editBalMaxDevCellV()'>Edit</button></h4>";

      // Close the block
      content += "</div>";
    }

    if (charger) {
      // Start a new block with orange background color
      content += "<div style='background-color: #FF6E00; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

      content += "<h4 style='color: white;'>Charger HVDC Enabled: ";
      if (datalayer.charger.charger_HV_enabled) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005;</span>";
      }
      content += " <button onclick='editChargerHVDCEnabled()'>Edit</button></h4>";

      content += "<h4 style='color: white;'>Charger Aux12VDC Enabled: ";
      if (datalayer.charger.charger_aux12V_enabled) {
        content += "<span>&#10003;</span>";
      } else {
        content += "<span style='color: red;'>&#10005;</span>";
      }
      content += " <button onclick='editChargerAux12vEnabled()'>Edit</button></h4>";

      content += "<h4 style='color: white;'>Charger Voltage Setpoint: " +
                 String(datalayer.charger.charger_setpoint_HV_VDC, 1) +
                 " V </span> <button onclick='editChargerSetpointVDC()'>Edit</button></h4>";
      content += "<h4 style='color: white;'>Charger Current Setpoint: " +
                 String(datalayer.charger.charger_setpoint_HV_IDC, 1) +
                 " A </span> <button onclick='editChargerSetpointIDC()'>Edit</button></h4>";

      // Close the block
      content += "</div>";
    }

    content += "<script>";  // Note, this section is minified to improve performance
    content += "function editComplete(){if(this.status==200){window.location.reload();}}";
    content += "function editError(){alert('Invalid input');}";
    content +=
        "function editSSID(){var value=prompt('Enter new SSID:');if(value!==null){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateSSID?value='+encodeURIComponent(value),true);xhr.send();}}";
    content +=
        "function editPassword(){var value=prompt('Enter new password:');if(value!==null){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updatePassword?value='+encodeURIComponent(value),true);xhr.send();}}";
    content +=
        "function editWh(){var value=prompt('How much energy the battery can store. Enter new Wh value "
        "(1-120000):');if(value!==null){if(value>=1&&value<=120000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateBatterySize?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 1 "
        "and 120000.');}}}";
    content +=
        "function editUseScaledSOC(){var value=prompt('Extends battery life by rescaling the SOC within the configured "
        "minimum "
        "and maximum percentage. Should SOC scaling be applied? (0 = No, 1 = "
        "Yes):');if(value!==null){if(value==0||value==1){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateUseScaledSOC?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 "
        "and 1.');}}}";
    content +=
        "function editSocMax(){var value=prompt('Inverter will see fully charged (100pct)SOC when this value is "
        "reached. Enter new maximum SOC value that battery will charge to "
        "(50.0-100.0):');if(value!==null){if(value>=50&&value<=100){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateSocMax?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 50.0 and "
        "100.0');}}}";
    content +=
        "function editSocMin(){var value=prompt('Inverter will see completely discharged (0pct)SOC when this value is "
        "reached. Advanced users can set to negative values. Enter new minimum SOC value that battery will discharge "
        "to "
        "(-10.0to50.0):');if(value!==null){if(value>=-10&&value<=50){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateSocMin?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between -10 and "
        "50.0');}}}";
    content +=
        "function editMaxChargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new "
        "maximum charge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateMaxChargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 "
        "and 1000.0');}}}";
    content +=
        "function editMaxDischargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new "
        "maximum discharge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateMaxDischargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 "
        "and 1000.0');}}}";
    content +=
        "function editUseVoltageLimit(){var value=prompt('Enable this option to manually restrict charge/discharge to "
        "a specific voltage set below."
        "If disabled the emulator automatically determines this based on battery limits. Restrict manually? (0 = No, 1 "
        "= Yes)"
        ":');if(value!==null){if(value==0||value==1){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateUseVoltageLimit?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between "
        "0 "
        "and 1.');}}}";
    content +=
        "function editMaxChargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter new "
        "voltage setpoint batttery should charge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var "
        "xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateMaxChargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 0 "
        "and 1000.0');}}}";
    content +=
        "function editMaxDischargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter "
        "new "
        "voltage setpoint batttery should discharge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var "
        "xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateMaxDischargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 0 "
        "and 1000.0');}}}";

    content +=
        "function editTeslaBalAct(){var value=prompt('Enable or disable forced LFP balancing. Makes the battery charge "
        "to 101percent. This should be performed once every month, to keep LFP batteries balanced. Ensure battery is "
        "fully charged before enabling, and also that you have enough sun or grid power to feed power into the battery "
        "while balancing is active. Enter 1 for enabled, 0 "
        "for disabled');if(value!==null){if(value==0||value==1){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "TeslaBalAct?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}";
    content +=
        "function editBalTime(){var value=prompt('Enter new max balancing time in "
        "minutes');if(value!==null){if(value>=1&&value<=300){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "BalTime?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 1 and 300');}}}";
    content +=
        "function editBalFloatPower(){var value=prompt('Power level in Watt to float charge during forced "
        "balancing');if(value!==null){if(value>=100&&value<=2000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "BalFloatPower?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 100 and 2000');}}}";
    content +=
        "function editBalMaxPackV(){var value=prompt('Battery pack max voltage temporarily raised to this value during "
        "forced balancing. Value in V');if(value!==null){if(value>=380&&value<=410){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "BalMaxPackV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 380 and 410');}}}";
    content +=
        "function editBalMaxCellV(){var value=prompt('Cellvoltage max temporarily raised to this value during forced "
        "balancing. Value in mV');if(value!==null){if(value>=3400&&value<=3750){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "BalMaxCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 3400 and 3750');}}}";
    content +=
        "function editBalMaxDevCellV(){var value=prompt('Cellvoltage max deviation temporarily raised to this value "
        "during forced balancing. Value in mV');if(value!==null){if(value>=300&&value<=600){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "BalMaxDevCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 300 and 600');}}}";

    if (battery && battery->supports_set_fake_voltage()) {
      content +=
          "function editFakeBatteryVoltage(){var value=prompt('Enter new fake battery "
          "voltage');if(value!==null){if(value>=0&&value<=5000){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateFakeBatteryVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
          "between 0 and 1000');}}}";
    }

    if (charger) {
      content +=
          "function editChargerHVDCEnabled(){var value=prompt('Enable or disable HV DC output. Enter 1 for enabled, 0 "
          "for disabled');if(value!==null){if(value==0||value==1){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateChargerHvEnabled?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}";
      content +=
          "function editChargerAux12vEnabled(){var value=prompt('Enable or disable low voltage 12v auxiliary DC "
          "output. "
          "Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateChargerAux12vEnabled?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter 1 or "
          "0');}}}";
      content +=
          "function editChargerSetpointVDC(){var value=prompt('Set charging voltage. Input will be validated against "
          "inverter and/or charger configuration parameters, but use sensible values like 200 to "
          "420.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateChargeSetpointV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
          "between "
          "0 and 1000');}}}";
      content +=
          "function editChargerSetpointIDC(){var value=prompt('Set charging amperage. Input will be validated against "
          "inverter and/or charger configuration parameters, but use sensible values like 6 to "
          "48.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateChargeSetpointA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
          "between "
          "0 and 100');}}}";
      content +=
          "function editChargerSetpointEndI(){var value=prompt('Set amperage that terminates charge as being "
          "sufficiently complete. Input will be validated against inverter and/or charger configuration parameters, "
          "but "
          "use sensible values like 1-5.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
          "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
          "updateChargeEndA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 "
          "and 100');}}}";
    }
    content += "</script>";

    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}

const char* getCANInterfaceName(CAN_Interface interface) {
  switch (interface) {
    case CAN_NATIVE:
      return "CAN";
    case CANFD_NATIVE:
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
      return "CAN-FD Native (Classic CAN)";
#else
      return "CAN-FD Native";
#endif
    case CAN_ADDON_MCP2515:
      return "Add-on CAN via GPIO MCP2515";
    case CANFD_ADDON_MCP2518:
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
      return "Add-on CAN-FD via GPIO MCP2518 (Classic CAN)";
#else
      return "Add-on CAN-FD via GPIO MCP2518";
#endif
    default:
      return "UNKNOWN";
  }
}
