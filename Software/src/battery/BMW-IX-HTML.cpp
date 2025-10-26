#include "BMW-IX-HTML.h"
#include "BMW-IX-BATTERY.h"

String BmwIXHtmlRenderer::getDTCDescription(uint32_t code) {
  switch (code) {
    // Add BMW iX specific DTC descriptions here
    // For now, return empty string for unknown codes
    default:
      return "";
  }
}

String BmwIXHtmlRenderer::get_status_html() {
  String content;

  // Power & Voltage Section
  content +=
      "<h3 style='color: #1e88e5; border-bottom: 2px solid #1e88e5; padding-bottom: 5px;'>⚡ Power & Voltage</h3>";
  content += "<div style='margin-left: 15px;'>";
  content +=
      "<h4>Battery Voltage (After Contactor): " + String(batt.get_battery_voltage_after_contactor()) + " dV</h4>";
  content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
  content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
  content += "<h4>T30 Terminal Voltage: " + String(batt.get_T30_Voltage()) + " mV</h4>";
  content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
  content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
  content += "<h4>BMS Allowed Charge Amps: " + String(batt.get_allowable_charge_amps()) + " A</h4>";
  content += "<h4>BMS Allowed Discharge Amps: " + String(batt.get_allowable_discharge_amps()) + " A</h4>";
  content += "</div>";

  // Cell Information Section
  content +=
      "<h3 style='color: #8e24aa; border-bottom: 2px solid #8e24aa; padding-bottom: 5px;'>📊 Cell Information</h3>";
  content += "<div style='margin-left: 15px;'>";
  content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
  content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
  content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
  content += "<h4>Min Cell Voltage Data Age: " + String(batt.get_min_cell_voltage_data_age()) + " ms</h4>";
  content += "<h4>Max Cell Voltage Data Age: " + String(batt.get_max_cell_voltage_data_age()) + " ms</h4>";
  content += "</div>";

  // Balancing Status Section
  content +=
      "<h3 style='color: #5e35b1; border-bottom: 2px solid #5e35b1; padding-bottom: 5px;'>⚖️ Balancing Status</h3>";
  content += "<div style='margin-left: 15px;'>";
  content += "<h4>Balancing: ";
  switch (batt.get_balancing_status()) {
    case 0:
      content += "No Balancing Mode Active</h4>";
      break;
    case 1:
      content += "<span style='color: #43a047;'>Voltage-Controlled Balancing Mode</span></h4>";
      break;
    case 2:
      content +=
          "<span style='color: #43a047;'>Time-Controlled Balancing Mode with Demand Calculation at End of "
          "Charging</span></h4>";
      break;
    case 3:
      content +=
          "<span style='color: #43a047;'>Time-Controlled Balancing Mode with Demand Calculation at Resting "
          "Voltage</span></h4>";
      break;
    case 4:
      content += "No Balancing Mode Active (Qualifier Invalid)</h4>";
      break;
    default:
      content += "Unknown</h4>";
  }
  content += "</div>";

  // Safety Systems Section
  content += "<h3 style='color: #e53935; border-bottom: 2px solid #e53935; padding-bottom: 5px;'>🛡️ Safety Systems</h3>";
  content += "<div style='margin-left: 15px;'>";
  content += "<h4>HVIL Status: ";
  switch (batt.get_hvil_status()) {
    case 0:
      content += "<span style='color: #d32f2f;'>⚠ Error (Loop Open)</span></h4>";
      break;
    case 1:
      content += "OK (Loop Closed)</h4>";
      break;
    default:
      content += "Unknown</h4>";
  }
  content += "<h4>Pyro Status PSS1: ";
  switch (batt.get_pyro_status_pss1()) {
    case 0:
      content += "Value Invalid</h4>";
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>⚠ Successfully Blown</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>Disconnected</span></h4>";
      break;
    case 3:
      content += "Not Activated - Pyro Intact</h4>";
      break;
    case 4:
      content += "Unknown</h4>";
      break;
    default:
      content += "Unknown</h4>";
  }
  content += "<h4>Pyro Status PSS4: ";
  switch (batt.get_pyro_status_pss4()) {
    case 0:
      content += "Value Invalid</h4>";
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>⚠ Successfully Blown</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>Disconnected</span></h4>";
      break;
    case 3:
      content += "Not Activated - Pyro Intact</h4>";
      break;
    case 4:
      content += "Unknown</h4>";
      break;
    default:
      content += "Unknown</h4>";
  }
  content += "<h4>Pyro Status PSS6: ";
  switch (batt.get_pyro_status_pss6()) {
    case 0:
      content += "Value Invalid</h4>";
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>⚠ Successfully Blown</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>Disconnected</span></h4>";
      break;
    case 3:
      content += "Not Activated - Pyro Intact</h4>";
      break;
    case 4:
      content += "Unknown</h4>";
      break;
    default:
      content += "Unknown</h4>";
  }
  content += "</div>";

  // Isolation Monitoring Section
  content +=
      "<h3 style='color: #fb8c00; border-bottom: 2px solid #fb8c00; padding-bottom: 5px;'>🔋 Isolation "
      "Monitoring</h3>";
  content += "<div style='margin-left: 15px;'>";
  content +=
      "<h4>Isolation Positive: " + String(batt.get_iso_safety_positive()) + " kΩ (2147483647 = maximum/invalid)</h4>";
  content +=
      "<h4>Isolation Negative: " + String(batt.get_iso_safety_negative()) + " kΩ (2147483647 = maximum/invalid)</h4>";
  content +=
      "<h4>Isolation Parallel: " + String(batt.get_iso_safety_parallel()) + " kΩ (2147483647 = maximum/invalid)</h4>";
  content += "</div>";

  // Diagnostics Section
  content += "<h3 style='color: #757575; border-bottom: 2px solid #757575; padding-bottom: 5px;'>🔧 Diagnostics</h3>";
  content += "<div style='margin-left: 15px;'>";
  content += "<h4>BMS Uptime: " + String(batt.get_bms_uptime()) + " seconds</h4>";
  content += "</div>";

  // Diagnostic Trouble Codes Section
  content +=
      "<h3 style='color: #27b06c; border-bottom: 2px solid #27b06c; padding-bottom: 5px;'>🔧 Diagnostic Trouble "
      "Codes</h3>";
  content += "<div style='margin-left: 15px; margin-right: 15px;'>";

  if (datalayer_extended.bmwix.dtc_read_failed) {
    content += "<p style='color: #d32f2f;'>⚠ Last DTC read failed or not supported</p>";
  } else if (datalayer_extended.bmwix.dtc_count == 0) {
    content += "<p style='color: #4CAF50;'>✓ No DTCs present</p>";
  } else {
    content += "<p><strong>DTC Count:</strong> " + String(datalayer_extended.bmwix.dtc_count) + "</p>";
    content += "<p><strong>Last Read:</strong> " +
               String((millis() - datalayer_extended.bmwix.dtc_last_read_millis) / 1000) + "s ago</p>";

    content += "<div style='overflow-x: auto; margin-top: 10px; margin-bottom: 15px;'>";
    content +=
        "<table style='width: auto; margin: 0 auto; border-collapse: separate; border-spacing: 0; border: 1px solid "
        "#ddd; border-radius: 8px; overflow: hidden;'>";

    content += "<thead>";
    content += "<tr style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white;'>";
    content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>DTC Code</th>";
    content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>Status</th>";
    content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>Description</th>";
    content += "</tr>";
    content += "</thead>";

    content += "<tbody>";

    for (int i = 0; i < datalayer_extended.bmwix.dtc_count; i++) {
      uint32_t code = datalayer_extended.bmwix.dtc_codes[i];
      uint8_t status = datalayer_extended.bmwix.dtc_status[i];

      char dtcStr[12];
      sprintf(dtcStr, "%06lX", code);

      String statusStr = "Stored";
      String statusColor = "#757575";

      if (status & 0x08) {
        statusStr = "Confirmed";
        statusColor = "#ff6f00";
      }

      if (status & 0x01) {
        statusStr = "Active";
        statusColor = "#d32f2f";
      }

      String description = getDTCDescription(code);
      if (description.length() == 0) {
        description = "Unknown";
      }

      content += "<tr>";
      content +=
          "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-family: monospace; font-size: 1.1em; "
          "font-weight: 600;'>" +
          String(dtcStr) + "</td>";
      content += "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; color: " + statusColor +
                 "; font-weight: 500;'>" + statusStr + "</td>";
      content += "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-size: 0.95em; color: #ddd;'>" +
                 description + "</td>";
      content += "</tr>";
    }

    content += "</tbody>";
    content += "</table>";
    content += "</div>";
  }

  content += "</div>";

  return content;
}
