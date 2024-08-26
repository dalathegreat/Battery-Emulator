#include "settings_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"

String settings_processor(const String& var) {
  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4 style='color: white;'>SSID: <span id='SSID'>" + String(ssid.c_str()) +
               " </span> <button onclick='editSSID()'>Edit</button></h4>";
    content +=
        "<h4 style='color: white;'>Password: ######## <span id='Password'></span> <button "
        "onclick='editPassword()'>Edit</button></h4>";

    content += "<h4 style='color: white;'>Battery interface: <span id='Battery'>" +
               String(getCANInterfaceName(can_config.battery)) + "</span></h4>";

#ifdef DOUBLE_BATTERY
    content += "<h4 style='color: white;'>Battery #2 interface: <span id='Battery'>" +
               String(getCANInterfaceName(can_config.battery_double)) + "</span></h4>";
#endif  // DOUBLE_BATTERY

#ifdef CAN_INVERTER_SELECTED
    content += "<h4 style='color: white;'>Inverter interface: <span id='Inverter'>" +
               String(getCANInterfaceName(can_config.inverter)) + "</span></h4>";
#endif  //CAN_INVERTER_SELECTED
#ifdef MODBUS_INVERTER_SELECTED
    content += "<h4 style='color: white;'>Inverter interface: RS485<span id='Inverter'></span></h4>";
#endif

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
    content +=
        "<h4 style='color: white;'>Max charge speed: " + String(datalayer.battery.info.max_charge_amp_dA / 10.0, 1) +
        " A </span> <button onclick='editMaxChargeA()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max discharge speed: " +
               String(datalayer.battery.info.max_discharge_amp_dA / 10.0, 1) +
               " A </span> <button onclick='editMaxDischargeA()'>Edit</button></h4>";
    // Close the block
    content += "</div>";

#ifdef TEST_FAKE_BATTERY
    // Start a new block with blue background color
    content += "<div style='background-color: #2E37AD; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";
    float voltageFloat =
        static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0;  // Convert to float and divide by 10
    content += "<h4 style='color: white;'>Fake battery voltage: " + String(voltageFloat, 1) +
               " V </span> <button onclick='editFakeBatteryVoltage()'>Edit</button></h4>";

    // Close the block
    content += "</div>";
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER

    // Start a new block with orange background color
    content += "<div style='background-color: #FF6E00; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4 style='color: white;'>Charger HVDC Enabled: ";
    if (charger_HV_enabled) {
      content += "<span>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += " <button onclick='editChargerHVDCEnabled()'>Edit</button></h4>";

    content += "<h4 style='color: white;'>Charger Aux12VDC Enabled: ";
    if (charger_aux12V_enabled) {
      content += "<span>&#10003;</span>";
    } else {
      content += "<span style='color: red;'>&#10005;</span>";
    }
    content += " <button onclick='editChargerAux12vEnabled()'>Edit</button></h4>";

    content += "<h4 style='color: white;'>Charger Voltage Setpoint: " + String(charger_setpoint_HV_VDC, 1) +
               " V </span> <button onclick='editChargerSetpointVDC()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Charger Current Setpoint: " + String(charger_setpoint_HV_IDC, 1) +
               " A </span> <button onclick='editChargerSetpointIDC()'>Edit</button></h4>";

    // Close the block
    content += "</div>";
#endif

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
        "function editUseScaledSOC(){var value=prompt('Should SOC% be scaled? (0 = No, 1 = "
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
        "reached. Enter new minimum SOC value that battery will discharge to "
        "(0-50.0):');if(value!==null){if(value>=0&&value<=50){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateSocMin?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and "
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
    content += "</script>";

#ifdef TEST_FAKE_BATTERY
    content +=
        "function editFakeBatteryVoltage(){var value=prompt('Enter new fake battery "
        "voltage');if(value!==null){if(value>=0&&value<=5000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateFakeBatteryVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value "
        "between 0 and 1000');}}}";
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
    content +=
        "function editChargerHVDCEnabled(){var value=prompt('Enable or disable HV DC output. Enter 1 for enabled, 0 "
        "for disabled');if(value!==null){if(value==0||value==1){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateChargerHvEnabled?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}";
    content +=
        "function editChargerAux12vEnabled(){var value=prompt('Enable or disable low voltage 12v auxiliary DC output. "
        "Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateChargerAux12vEnabled?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter 1 or "
        "0');}}}";
    content +=
        "function editChargerSetpointVDC(){var value=prompt('Set charging voltage. Input will be validated against "
        "inverter and/or charger configuration parameters, but use sensible values like 200 to "
        "420.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateChargeSetpointV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between "
        "0 and 1000');}}}";
    content +=
        "function editChargerSetpointIDC(){var value=prompt('Set charging amperage. Input will be validated against "
        "inverter and/or charger configuration parameters, but use sensible values like 6 to "
        "48.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateChargeSetpointA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between "
        "0 and 100');}}}";
    content +=
        "function editChargerSetpointEndI(){var value=prompt('Set amperage that terminates charge as being "
        "sufficiently complete. Input will be validated against inverter and/or charger configuration parameters, but "
        "use sensible values like 1-5.');if(value!==null){if(value>=0&&value<=1000){var xhr=new "
        "XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/"
        "updateChargeEndA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 "
        "and 100');}}}";
#endif
    content += "</script>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";
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
    case CAN_ADDON_FD_MCP2518:
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
      return "Add-on CAN-FD via GPIO MCP2518 (Classic CAN)";
#else
      return "Add-on CAN-FD via GPIO MCP2518";
#endif
    default:
      return "UNKNOWN";
  }
}
