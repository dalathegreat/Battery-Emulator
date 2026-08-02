#include "BMW-IX-HTML.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/utils/time_format.h"
#include "BMW-IX-BATTERY.h"

String BmwIXHtmlRenderer::get_status_html() {
  String content;

  // Power & Voltage Section
  content +=
      "<h3 style='color: #1e88e5; border-bottom: 2px solid #1e88e5; padding-bottom: 5px;'>⚡ Power & Voltage</h3>";
  content += "<div style='margin-left: 15px;'>";
  tr_h4(content, TrKey::DRV_BATTERY_VOLTAGE_AFTER_CONTACTOR, String(batt.get_battery_voltage_after_contactor()), " dV");
  tr_h4(content, TrKey::DRV_MAX_DESIGN_VOLTAGE, String(datalayer.battery.info.max_design_voltage_dV), " dV");
  tr_h4(content, TrKey::DRV_MIN_DESIGN_VOLTAGE, String(datalayer.battery.info.min_design_voltage_dV), " dV");
  tr_h4(content, TrKey::DRV_T30_TERMINAL_VOLTAGE, String(batt.get_T30_Voltage()), " mV");
  tr_h4(content, TrKey::DRV_ALLOWED_CHARGE_POWER, String(datalayer.battery.status.max_charge_power_W), " W");
  tr_h4(content, TrKey::DRV_ALLOWED_DISCHARGE_POWER, String(datalayer.battery.status.max_discharge_power_W), " W");
  tr_h4(content, TrKey::DRV_BMS_ALLOWED_CHARGE_AMPS, String(batt.get_allowable_charge_amps()), " A");
  tr_h4(content, TrKey::DRV_BMS_ALLOWED_DISCHARGE_AMPS, String(batt.get_allowable_discharge_amps()), " A");
  content += "</div>";

  // Cell Information Section
  content +=
      "<h3 style='color: #8e24aa; border-bottom: 2px solid #8e24aa; padding-bottom: 5px;'>📊 Cell Information</h3>";
  content += "<div style='margin-left: 15px;'>";
  tr_h4(content, TrKey::DRV_DETECTED_CELL_COUNT, String(datalayer.battery.info.number_of_cells));
  tr_h4(content, TrKey::DRV_MAX_CELL_DESIGN_VOLTAGE, String(datalayer.battery.info.max_cell_voltage_mV), " mV");
  tr_h4(content, TrKey::DRV_MIN_CELL_DESIGN_VOLTAGE, String(datalayer.battery.info.min_cell_voltage_mV), " mV");
  tr_h4(content, TrKey::DRV_MIN_CELL_VOLTAGE_DATA_AGE, String(batt.get_min_cell_voltage_data_age()), " ms");
  tr_h4(content, TrKey::DRV_MAX_CELL_VOLTAGE_DATA_AGE, String(batt.get_max_cell_voltage_data_age()), " ms");
  content += "</div>";

  // Battery Status Section
  content += "<h3 style='color: #35b1ab; border-bottom: 2px solid #35b1ab; padding-bottom: 5px;'>⚖️ Battery Status</h3>";
  content += "<div style='margin-left: 15px;'>";
  tr_h4_open(content, TrKey::DRV_BALANCING);
  switch (batt.get_balancing_status()) {
    case 0:
      tr_h4_end(content, TrKey::DRV_NO_BALANCING_MODE_ACTIVE);
      break;
    case 1:
      content += "<span style='color: #43a047;'>" + TR(TrKey::DRV_VOLTAGE_CONTROLLED_BALANCING_MODE) + "</span></h4>";
      break;
    case 2:
      content += "<span style='color: #43a047;'>" +
                 TR(TrKey::DRV_TIME_CONTROLLED_BALANCING_MODE_DEMAND_CALCULATION_AT_END) + " " +
                 TR(TrKey::DRV_CHARGING) + "</span></h4>";
      break;
    case 3:
      content += "<span style='color: #43a047;'>" +
                 TR(TrKey::DRV_TIME_CONTROLLED_BALANCING_MODE_DEMAND_CALCULATION_AT_RESTING) + " " +
                 TR(TrKey::UI_VOLTAGE) + "</span></h4>";
      break;
    case 4:
      tr_h4_end(content, TrKey::DRV_NO_BALANCING_MODE_ACTIVE_QUALIFIER_INVALID);
      break;
    default:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
  }
  // Energy Saving Mode Status
  tr_h4_open(content, TrKey::DRV_ENERGY_SAVING_MODE);
  int energy_mode = batt.get_energy_saving_mode_status();
  switch (energy_mode) {
    case 0:
      tr_h4_end(content, TrKey::DRV_NO_OPERATING_MODE_SET_NORMAL);
      break;
    case 1:
      content += "<span style='color: #fb8c00;'>" + TR(TrKey::DRV_PRODUCTION_MODE_ACTIVE) + "</span></h4>";
      break;
    case 2:
      content += "<span style='color: #fb8c00;'>" + TR(TrKey::DRV_TRANSPORT_MODE_ACTIVE) + "</span></h4>";
      break;
    case 3:
      content += "<span style='color: #fb8c00;'>" + TR(TrKey::DRV_FLASH_MODE_ACTIVE) + "</span></h4>";
      break;
    default:
      if (energy_mode >= 4 && energy_mode <= 16) {
        content += "<span style='color: #fb8c00;'>" + TR(TrKey::DRV_EXTENDED_OPERATING_MODE) + " " +
                   String(energy_mode) + "</span></h4>";
      } else {
        content += tr_expand(TR(TrKey::DRV_UNKNOWN_CODE), String(energy_mode)) + "</h4>";
      }
      break;
  }
  content += "</div>";

  // Safety Systems Section
  content += "<h3 style='color: #e53935; border-bottom: 2px solid #e53935; padding-bottom: 5px;'>🛡️ Safety Systems</h3>";
  content += "<div style='margin-left: 15px;'>";
  tr_h4_open(content, TrKey::DRV_HVIL_STATUS);
  switch (batt.get_hvil_status()) {
    case 0:
      content += "<span style='color: #d32f2f;'>" + TR(TrKey::DRV_ERROR_LOOP_OPEN) + "</span></h4>";
      break;
    case 1:
      tr_h4_end(content, TrKey::DRV_OK_LOOP_CLOSED);
      break;
    default:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
  }
  tr_h4_open(content, TrKey::DRV_PYRO_STATUS_PSS1);
  switch (batt.get_pyro_status_pss1()) {
    case 0:
      tr_h4_end(content, TrKey::DRV_VALUE_INVALID);
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>" + TR(TrKey::DRV_SUCCESSFULLY_BLOWN) + "</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>" + TR(TrKey::UI_DISCONNECTED) + "</span></h4>";
      break;
    case 3:
      tr_h4_end(content, TrKey::DRV_NOT_ACTIVATED_PYRO_INTACT);
      break;
    case 4:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
      break;
    default:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
  }
  tr_h4_open(content, TrKey::DRV_PYRO_STATUS_PSS4);
  switch (batt.get_pyro_status_pss4()) {
    case 0:
      tr_h4_end(content, TrKey::DRV_VALUE_INVALID);
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>" + TR(TrKey::DRV_SUCCESSFULLY_BLOWN) + "</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>" + TR(TrKey::UI_DISCONNECTED) + "</span></h4>";
      break;
    case 3:
      tr_h4_end(content, TrKey::DRV_NOT_ACTIVATED_PYRO_INTACT);
      break;
    case 4:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
      break;
    default:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
  }
  tr_h4_open(content, TrKey::DRV_PYRO_STATUS_PSS6);
  switch (batt.get_pyro_status_pss6()) {
    case 0:
      tr_h4_end(content, TrKey::DRV_VALUE_INVALID);
      break;
    case 1:
      content += "<span style='color: #d32f2f;'>" + TR(TrKey::DRV_SUCCESSFULLY_BLOWN) + "</span></h4>";
      break;
    case 2:
      content += "<span style='color: #ff6f00;'>" + TR(TrKey::UI_DISCONNECTED) + "</span></h4>";
      break;
    case 3:
      tr_h4_end(content, TrKey::DRV_NOT_ACTIVATED_PYRO_INTACT);
      break;
    case 4:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
      break;
    default:
      tr_h4_end(content, TrKey::UI_UNKNOWN);
  }
  content += "</div>";

  // Isolation Monitoring Section
  content += "<h3 style='color: #fb8c00; border-bottom: 2px solid #fb8c00; padding-bottom: 5px;'>🔋 Isolation " +
             TR(TrKey::DRV_MONITORING) + "</h3>";
  content += "<div style='margin-left: 15px;'>";
  tr_h4(content, TrKey::DRV_ISOLATION_POSITIVE,
        String(batt.get_iso_safety_positive()) + " " + TR(TrKey::DRV_K_2147483647_MAXIMUM_INVALID));
  tr_h4(content, TrKey::DRV_ISOLATION_NEGATIVE,
        String(batt.get_iso_safety_negative()) + " " + TR(TrKey::DRV_K_2147483647_MAXIMUM_INVALID));
  tr_h4(content, TrKey::DRV_ISOLATION_PARALLEL,
        String(batt.get_iso_safety_parallel()) + " " + TR(TrKey::DRV_K_2147483647_MAXIMUM_INVALID));
  content += "</div>";

  // Diagnostics Section
  content += "<h3 style='color: #757575; border-bottom: 2px solid #757575; padding-bottom: 5px;'>🔧 Diagnostics</h3>";
  content += "<div style='margin-left: 15px;'>";

  // Uptime formatting moved to format_ms_string() upstream; keep the translated label.
  tr_h4(content, TrKey::DRV_BMS_UPTIME, format_ms_string((uint64_t)batt.get_bms_uptime() * 1000));
  content += "</div>";

  // Diagnostic Trouble Codes Section
  content +=
      "<h3 style='color: #27b06c; border-bottom: 2px solid #27b06c; padding-bottom: 5px;'>🔧 Diagnostic Trouble " +
      TR(TrKey::DRV_CODES) + "</h3>";
  content += "<div style='margin-left: 15px; margin-right: 15px;'>";

  if (datalayer_extended.bmwix.dtc_last_read_millis == 0) {
    // No DTC read has been performed yet
    content += "<p style='color: #ff9800;'>" +
               TR(TrKey::DRV_DTCS_HAVE_NOT_BEEN_READ_YET_CLICK_READ_DTC_SCAN_FAULT_CODES) + "</p>";
  } else if (datalayer_extended.bmwix.dtc_read_failed) {
    content += "<p style='color: #d32f2f;'>" + TR(TrKey::DRV_LAST_DTC_READ_FAILED_NOT_SUPPORTED) + "</p>";
  } else if (datalayer_extended.bmwix.dtc_count == 0) {
    content += "<p style='color: #4CAF50;'>✓ " + TR(TrKey::DRV_NO_DTCS_PRESENT) + "</p>";
  } else {
    content +=
        "<p><strong>" + TR(TrKey::DRV_DTC_COUNT) + "</strong> " + String(datalayer_extended.bmwix.dtc_count) + "</p>";

    // Convert last read time to days:hours:minutes:seconds format
    unsigned long last_read_seconds = (millis() - datalayer_extended.bmwix.dtc_last_read_millis) / 1000;
    unsigned long read_days = last_read_seconds / 86400;
    unsigned long read_hours = (last_read_seconds % 86400) / 3600;
    unsigned long read_minutes = (last_read_seconds % 3600) / 60;
    unsigned long read_seconds = last_read_seconds % 60;

    content += "<p><strong>" + TR(TrKey::DRV_LAST_READ) + "</strong> ";
    if (read_days > 0) {
      content += String(read_days) + "d ";
    }
    if (read_hours > 0 || read_days > 0) {
      content += String(read_hours) + "h ";
    }
    content += String(read_minutes) + "m " + String(read_seconds) + TR(TrKey::DRV_S_AGO) + "</p>";

    content += "<div style='overflow-x: auto; margin-top: 10px; margin-bottom: 15px;'>";
    content +=
        "<table style='width: auto; margin: 0 auto; border-collapse: separate; border-spacing: 0; border: 1px solid "
        "#ddd; border-radius: 8px; overflow: hidden;'>";

    content += "<thead>";
    content += "<tr style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white;'>";
    content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>DTC Code</th>";
    content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>Status</th>";
    content +=
        "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>" + TR(TrKey::DRV_DESCRIPTION) + "</th>";
    content += "</tr>";
    content += "</thead>";

    content += "<tbody>";

    for (int i = 0; i < datalayer_extended.bmwix.dtc_count; i++) {
      uint32_t code = datalayer_extended.bmwix.dtc_codes[i];
      uint8_t status = datalayer_extended.bmwix.dtc_status[i];

      char dtcStr[12];
      sprintf(dtcStr, "%06lX", code);

      String statusStr = TR(TrKey::DRV_STORED);
      String statusColor = "#757575";

      if (status & 0x08) {
        statusStr = TR(TrKey::DRV_CONFIRMED);
        statusColor = "#ff6f00";
      }

      if (status & 0x01) {
        statusStr = TR(TrKey::DRV_ACTIVE_STATUS);
        statusColor = "#d32f2f";
      }

      content += "<tr>";
      content +=
          "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-family: monospace; font-size: 1.1em; "
          "font-weight: 600;'>" +
          String(dtcStr) + "</td>";
      content += "<td style='padding: 12px 15px; border-top: 1px solid #e0e0e0; color: " + statusColor +
                 "; font-weight: 500;'>" + statusStr + "</td>";
      content +=
          "<td data-dtc-code='" + String(code) +
          "' style='padding: 12px 15px; border-top: 1px solid #e0e0e0; font-size: 0.95em; color: #ddd;'>Unknown</td>";
      content += "</tr>";
    }

    content += "</tbody>";
    content += "</table>";
    content += "</div>";

    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "bmw_ix_dtc.json");
  }

  content += "</div>";

  return content;
}
