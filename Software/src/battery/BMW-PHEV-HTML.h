#ifndef _BMW_PHEV_HTML_H
#define _BMW_PHEV_HTML_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BmwPhevHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    // Power & Voltage Section
    content +=
        "<h3 style='color: #1e88e5; border-bottom: 2px solid #1e88e5; padding-bottom: 5px;'>⚡ Power & Voltage</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>DC Link Voltage: " + String(datalayer_extended.bmwphev.battery_DC_link_voltage) + " V</h4>";
    content +=
        "<h4>Battery Voltage (After Contactor): " + String(datalayer_extended.bmwphev.battery_voltage_after_contactor) +
        " dV</h4>";
    content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
    content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
    content += "<h4>BMS Allowed Charge Amps: " + String(datalayer_extended.bmwphev.allowable_charge_amps) + " A</h4>";
    content +=
        "<h4>BMS Allowed Discharge Amps: " + String(datalayer_extended.bmwphev.allowable_discharge_amps) + " A</h4>";
    content += "</div>";

    // Contactor Status Section
    content +=
        "<h3 style='color: #43a047; border-bottom: 2px solid #43a047; padding-bottom: 5px;'>🔌 Contactor Status</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Contactor Status: ";
    switch (datalayer_extended.bmwphev.ST_DCSW) {
      case 0:
        content += String("Contactors Open</h4>");
        break;
      case 1:
        content += String("Precharge Ongoing</h4>");
        break;
      case 2:
        content += String("Contactors Engaged</h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Precharge Status: ";
    switch (datalayer_extended.bmwphev.ST_precharge) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("Not Active, Closing Not Blocked</h4>");
        break;
      case 2:
        content += String("Error - Precharge Blocked</h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Contactor Weld Status: ";
    switch (datalayer_extended.bmwphev.ST_WELD) {
      case 0:
        content += String("Contactors OK</h4>");
        break;
      case 1:
        content += String("<span style='color: #ff6f00;'>⚠ One Contactor Welded!</span></h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠⚠ Two Contactors Welded!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Request Open Contactors: ";
    switch (datalayer_extended.bmwphev.battery_request_open_contactors) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("Not Active</h4>");
        break;
      case 2:
        content += String("<span style='color: #ff6f00;'>Active</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Request Open Contactors (Fast): ";
    switch (datalayer_extended.bmwphev.battery_request_open_contactors_fast) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("Not Active</h4>");
        break;
      case 2:
        content += String("<span style='color: #ff6f00;'>Active</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Request Open Contactors (Instantly): ";
    switch (datalayer_extended.bmwphev.battery_request_open_contactors_instantly) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("Not Active</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>Active</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "</div>";

    // Safety Systems Section
    content +=
        "<h3 style='color: #e53935; border-bottom: 2px solid #e53935; padding-bottom: 5px;'>🛡️ Safety Systems</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Interlock: ";
    switch (datalayer_extended.bmwphev.ST_interlock) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error! Not Seated!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Emergency Status: ";
    switch (datalayer_extended.bmwphev.ST_EMG) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "</div>";

    // Isolation Monitoring Section
    content +=
        "<h3 style='color: #fb8c00; border-bottom: 2px solid #fb8c00; padding-bottom: 5px;'>🔋 Isolation "
        "Monitoring</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Overall Isolation Status: ";
    switch (datalayer_extended.bmwphev.ST_isolation) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Internal Isolation: ";
    switch (datalayer_extended.bmwphev.ST_iso_int) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>External Isolation: ";
    switch (datalayer_extended.bmwphev.ST_iso_ext) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Isolation Resistance: " + String(datalayer_extended.bmwphev.iso_safety_kohm) + " kΩ</h4>";
    content += "<h4>Isolation Quality: " + String(datalayer_extended.bmwphev.iso_safety_kohm_quality) + "</h4>";
    content += "<h4>Internal Resistance: " + String(datalayer_extended.bmwphev.iso_safety_int_kohm) + " kΩ " +
               (datalayer_extended.bmwphev.iso_safety_int_plausible ? "(Plausible)" : "(Not Plausible)") + "</h4>";
    content += "<h4>External Resistance: " + String(datalayer_extended.bmwphev.iso_safety_ext_kohm) + " kΩ " +
               (datalayer_extended.bmwphev.iso_safety_ext_plausible ? "(Plausible)" : "(Not Plausible)") + "</h4>";
    content += "<h4>Trigger Resistance: " + String(datalayer_extended.bmwphev.iso_safety_trg_kohm) + " kΩ " +
               (datalayer_extended.bmwphev.iso_safety_trg_plausible ? "(Plausible)" : "(Not Plausible)") + "</h4>";
    content += "</div>";

    // Thermal Management Section
    content +=
        "<h3 style='color: #00acc1; border-bottom: 2px solid #00acc1; padding-bottom: 5px;'>❄️ Thermal Management</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Cooling Valve Status: ";
    switch (datalayer_extended.bmwphev.ST_valve_cooling) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String("Invalid Signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Cold Shutoff Valve: ";
    switch (datalayer_extended.bmwphev.ST_cold_shutoff_valve) {
      case 0:
        content += String("OK</h4>");
        break;
      case 1:
        content += String("<span style='color: #d32f2f;'>Short Circuit to GND</span></h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>Short Circuit to 12V</span></h4>");
        break;
      case 3:
        content += String("<span style='color: #d32f2f;'>Line Break</span></h4>");
        break;
      case 6:
        content += String("<span style='color: #d32f2f;'>Driver Error</span></h4>");
        break;
      case 12:
      case 13:
        content += String("<span style='color: #d32f2f;'>Stuck</span></h4>");
        break;
      default:
        content += String("Invalid Signal</h4>");
    }
    content += "</div>";

    // Cell Information Section
    content +=
        "<h3 style='color: #8e24aa; border-bottom: 2px solid #8e24aa; padding-bottom: 5px;'>📊 Cell Information</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
    content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
    content +=
        "<h4>Min Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.min_cell_voltage_data_age) + " ms</h4>";
    content +=
        "<h4>Max Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.max_cell_voltage_data_age) + " ms</h4>";
    content += "</div>";

    // Balancing Status Section
    content +=
        "<h3 style='color: #5e35b1; border-bottom: 2px solid #5e35b1; padding-bottom: 5px;'>⚖️ Balancing Status</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Balancing: ";
    switch (datalayer_extended.bmwphev.balancing_status) {
      case 0:
        content += String("Inactive - Not Needed</h4>");
        break;
      case 1:
        content += String("<span style='color: #43a047;'>Active</span></h4>");
        break;
      case 2:
        content += String("Inactive - Cells Not at Rest (Wait 10 min)</h4>");
        break;
      case 3:
        content += String("Inactive</h4>");
        break;
      case 4:
        content += String("Unknown</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "</div>";

    // Diagnostics Section
    content += "<h3 style='color: #757575; border-bottom: 2px solid #757575; padding-bottom: 5px;'>🔧 Diagnostics</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<h4>Charging Condition Delta: " + String(datalayer_extended.bmwphev.battery_charging_condition_delta) +
               "</h4>";
    content += "</div>";

    content +=
        "<h3 style='color: #27b06c; border-bottom: 2px solid #27b06c; padding-bottom: 5px;'>🔧 Diagnostic Trouble "
        "Codes</h3>";
    content += "<div style='margin-left: 15px; margin-right: 15px;'>";

    if (datalayer_extended.bmwphev.dtc_read_failed) {
      content += "<p style='color: #d32f2f;'>⚠ Last DTC read failed or not supported</p>";
    } else if (datalayer_extended.bmwphev.dtc_count == 0) {
      content += "<p style='color: #4CAF50;'>✓ No DTCs present</p>";
    } else {
      content += "<p><strong>DTC Count:</strong> " + String(datalayer_extended.bmwphev.dtc_count) + "</p>";
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
      content +=
          "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>Description</th>";  // ✅ Add description column
      content += "</tr>";
      content += "</thead>";

      content += "<tbody>";

      for (int i = 0; i < datalayer_extended.bmwphev.dtc_count; i++) {
        uint32_t code = datalayer_extended.bmwix.dtc_codes[i];    //Note we re-use datalayer for iX to save space
        uint8_t status = datalayer_extended.bmwix.dtc_status[i];  //Note we re-use datalayer for iX to save space

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

        content += "<tr>";
        content +=
            "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-family: monospace; font-size: 1.1em; "
            "font-weight: 600;'>" +
            String(dtcStr) + "</td>";
        content += "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; color: " + statusColor +
                   "; font-weight: 500;'>" + statusStr + "</td>";
        content += "<td data-dtc-code='" + String(code) + "' style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-size: 0.95em; color: #ddd;'>Unknown</td>";
        content += "</tr>";
      }

      content += "</tbody>";
      content += "</table>";
      content += "</div>";

      content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "bmw_phev_dtc.json");
    }

    content += "</div>";

    return content;
  }
};

#endif
