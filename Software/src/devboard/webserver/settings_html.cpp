#include "settings_html.h"
#include <Arduino.h>

String settings_processor(const String& var) {
  if (var == "PLACEHOLDER") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Show current settings with edit buttons and input fields
    content += "<h4 style='color: white;'>Battery capacity: <span id='BATTERY_WH_MAX'>" + String(BATTERY_WH_MAX) +
               " Wh </span> <button onclick='editWh()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>SOC max percentage: " + String(MAXPERCENTAGE / 10.0, 1) +
               " </span> <button onclick='editSocMax()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>SOC min percentage: " + String(MINPERCENTAGE / 10.0, 1) +
               " </span> <button onclick='editSocMin()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max charge speed: " + String(MAXCHARGEAMP / 10.0, 1) +
               " A </span> <button onclick='editMaxChargeA()'>Edit</button></h4>";
    content += "<h4 style='color: white;'>Max discharge speed: " + String(MAXDISCHARGEAMP / 10.0, 1) +
               " A </span> <button onclick='editMaxDischargeA()'>Edit</button></h4>";
    // Close the block
    content += "</div>";

#ifdef TEST_FAKE_BATTERY
    // Start a new block with blue background color
    content += "<div style='background-color: #2E37AD; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";
    float voltageFloat = static_cast<float>(battery_voltage) / 10.0;  // Convert to float and divide by 10
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

    content += "<script>";
    content += "function editWh() {";
    content += "var value = prompt('How much energy the battery can store. Enter new Wh value (1-65000):');";
    content += "if (value !== null) {";
    content += "  if (value >= 1 && value <= 65000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateBatterySize?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 1 and 65000.');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editSocMax() {";
    content +=
        "var value = prompt('Inverter will see fully charged (100pct)SOC when this value is reached. Enter new maximum "
        "SOC value that battery will charge to (50.0-100.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 50 && value <= 100) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateSocMax?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 50.0 and 100.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editSocMin() {";
    content +=
        "var value = prompt('Inverter will see completely discharged (0pct)SOC when this value is reached. Enter new "
        "minimum SOC value that battery will discharge to (0-50.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 50) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateSocMin?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 50.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editMaxChargeA() {";
    content +=
        "var value = prompt('BYD CAN specific setting, some inverters needs to be artificially limited. Enter new "
        "maximum charge current in A (0-1000.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateMaxChargeA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000.0');";
    content += "  }";
    content += "}";
    content += "}";
    content += "function editMaxDischargeA() {";
    content +=
        "var value = prompt('BYD CAN specific setting, some inverters needs to be artificially limited. Enter new "
        "maximum discharge current in A (0-1000.0):');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateMaxDischargeA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000.0');";
    content += "  }";
    content += "}";
    content += "}";

#ifdef TEST_FAKE_BATTERY
    content += "function editFakeBatteryVoltage() {";
    content += "  var value = prompt('Enter new fake battery voltage');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 5000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateFakeBatteryVoltage?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000');";
    content += "  }";
    content += "}";
    content += "}";
#endif

#if defined CHEVYVOLT_CHARGER || defined NISSANLEAF_CHARGER
    content += "function editChargerHVDCEnabled() {";
    content += "  var value = prompt('Enable or disable HV DC output. Enter 1 for enabled, 0 for disabled');";
    content += "  if (value !== null) {";
    content += "    if (value == 0 || value == 1) {";
    content += "      var xhr = new XMLHttpRequest();";
    content += "      xhr.open('GET', '/updateChargerHvEnabled?value=' + value, true);";
    content += "      xhr.send();";
    content += "    }";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter 1 or 0');";
    content += "  }";
    content += "}";

    content += "function editChargerAux12vEnabled() {";
    content +=
        "var value = prompt('Enable or disable low voltage 12v auxiliary DC output. Enter 1 for enabled, 0 for "
        "disabled');";
    content += "if (value !== null) {";
    content += "  if (value == 0 || value == 1) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateChargerAux12vEnabled?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter 1 or 0');";
    content += "  }";
    content += "}";
    content += "}";

    content += "function editChargerSetpointVDC() {";
    content +=
        "var value = prompt('Set charging voltage. Input will be validated against inverter and/or charger "
        "configuration parameters, but use sensible values like 200 to 420.');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateChargeSetpointV?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 1000');";
    content += "  }";
    content += "}";
    content += "}";

    content += "function editChargerSetpointIDC() {";
    content +=
        "var value = prompt('Set charging amperage. Input will be validated against inverter and/or charger "
        "configuration parameters, but use sensible values like 6 to 48.');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateChargeSetpointA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 100');";
    content += "  }";
    content += "}";
    content += "}";

    content += "function editChargerSetpointEndI() {";
    content +=
        "var value = prompt('Set amperage that terminates charge as being sufficiently complete. Input will be "
        "validated against inverter and/or charger configuration parameters, but use sensible values like 1-5.');";
    content += "if (value !== null) {";
    content += "  if (value >= 0 && value <= 1000) {";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/updateChargeEndA?value=' + value, true);";
    content += "    xhr.send();";
    content += "  } else {";
    content += "    alert('Invalid value. Please enter a value between 0 and 100');";
    content += "  }";
    content += "}";
    content += "}";
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
