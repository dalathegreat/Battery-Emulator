#ifndef _BMW_PHEV_HTML_H
#define _BMW_PHEV_HTML_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BmwPhevHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    // Power & Voltage Section
    content +=
        "<h3 style='color: #1e88e5; border-bottom: 2px solid #1e88e5; padding-bottom: 5px;'>⚡ Power & Voltage</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4(content, TrKey::DRV_BATTERY_VOLTAGE_AFTER_CONTACTOR,
          String(datalayer_extended.bmwphev.battery_voltage_after_contactor), " dV");
    tr_h4(content, TrKey::DRV_MAX_DESIGN_VOLTAGE, String(datalayer.battery.info.max_design_voltage_dV), " dV");
    tr_h4(content, TrKey::DRV_MIN_DESIGN_VOLTAGE, String(datalayer.battery.info.min_design_voltage_dV), " dV");
    tr_h4(content, TrKey::DRV_ALLOWED_CHARGE_POWER, String(datalayer.battery.status.max_charge_power_W), " W");
    tr_h4(content, TrKey::DRV_ALLOWED_DISCHARGE_POWER, String(datalayer.battery.status.max_discharge_power_W), " W");
    tr_h4(content, TrKey::DRV_BMS_ALLOWED_CHARGE_AMPS, String(datalayer_extended.bmwphev.allowable_charge_amps), " A");
    tr_h4(content, TrKey::DRV_BMS_ALLOWED_DISCHARGE_AMPS, String(datalayer_extended.bmwphev.allowable_discharge_amps),
          " A");
    content += "</div>";

    // Contactor Status Section
    content +=
        "<h3 style='color: #43a047; border-bottom: 2px solid #43a047; padding-bottom: 5px;'>🔌 Contactor Status</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4_open(content, TrKey::DRV_CONTACTOR_STATUS);
    switch (datalayer_extended.bmwphev.ST_DCSW) {
      case 0:
        content += String(TR(TrKey::DRV_CONTACTORS_OPEN) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::DRV_PRECHARGE_ONGOING) + "</h4>");
        break;
      case 2:
        content += String(TR(TrKey::DRV_CONTACTORS_ENGAGED) + "</h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_PRECHARGE_STATUS);
    switch (datalayer_extended.bmwphev.ST_precharge) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::DRV_NOT_ACTIVE_CLOSING_NOT_BLOCKED) + "</h4>");
        break;
      case 2:
        content += String(TR(TrKey::DRV_ERROR_PRECHARGE_BLOCKED) + "</h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_CONTACTOR_WELD_STATUS);
    switch (datalayer_extended.bmwphev.ST_WELD) {
      case 0:
        content += String(TR(TrKey::DRV_CONTACTORS_OK) + "</h4>");
        break;
      case 1:
        content += String("<span style='color: #ff6f00;'>⚠ One Contactor Welded!</span></h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠⚠ Two Contactors Welded!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_REQUEST_OPEN_CONTACTORS);
    switch (datalayer_extended.bmwphev.battery_request_open_contactors) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::DRV_NOT_ACTIVE) + "</h4>");
        break;
      case 2:
        content += String("<span style='color: #ff6f00;'>" + TR(TrKey::DRV_ACTIVE_STATUS) + "</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_REQUEST_OPEN_CONTACTORS_FAST);
    switch (datalayer_extended.bmwphev.battery_request_open_contactors_fast) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::DRV_NOT_ACTIVE) + "</h4>");
        break;
      case 2:
        content += String("<span style='color: #ff6f00;'>" + TR(TrKey::DRV_ACTIVE_STATUS) + "</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_REQUEST_OPEN_CONTACTORS_INSTANTLY);
    switch (datalayer_extended.bmwphev.battery_request_open_contactors_instantly) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::DRV_NOT_ACTIVE) + "</h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_ACTIVE_STATUS) + "</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    content += "</div>";

    // Safety Systems Section
    content +=
        "<h3 style='color: #e53935; border-bottom: 2px solid #e53935; padding-bottom: 5px;'>🛡️ Safety Systems</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4_start(content, TrKey::DRV_INTERLOCK);
    content += ": ";
    switch (datalayer_extended.bmwphev.ST_interlock) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_ERROR_NOT_SEATED) + "</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_EMERGENCY_STATUS);
    switch (datalayer_extended.bmwphev.ST_EMG) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    content += "</div>";

    // Isolation Monitoring Section
    content += "<h3 style='color: #fb8c00; border-bottom: 2px solid #fb8c00; padding-bottom: 5px;'>🔋 Isolation " +
               TR(TrKey::DRV_MONITORING) + "</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4_open(content, TrKey::DRV_OVERALL_ISOLATION_STATUS);
    switch (datalayer_extended.bmwphev.ST_isolation) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_INTERNAL_ISOLATION);
    switch (datalayer_extended.bmwphev.ST_iso_int) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_EXTERNAL_ISOLATION);
    switch (datalayer_extended.bmwphev.ST_iso_ext) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4(content, TrKey::DRV_ISOLATION_RESISTANCE, String(datalayer_extended.bmwphev.iso_safety_kohm), " kΩ");
    tr_h4(content, TrKey::DRV_ISOLATION_QUALITY, String(datalayer_extended.bmwphev.iso_safety_kohm_quality));
    tr_h4(content, TrKey::DRV_INTERNAL_RESISTANCE,
          String(datalayer_extended.bmwphev.iso_safety_int_kohm) + " kΩ " +
              (datalayer_extended.bmwphev.iso_safety_int_plausible ? TR(TrKey::DRV_PLAUSIBLE)
                                                                   : TR(TrKey::DRV_NOT_PLAUSIBLE)));
    tr_h4(content, TrKey::DRV_EXTERNAL_RESISTANCE,
          String(datalayer_extended.bmwphev.iso_safety_ext_kohm) + " kΩ " +
              (datalayer_extended.bmwphev.iso_safety_ext_plausible ? TR(TrKey::DRV_PLAUSIBLE)
                                                                   : TR(TrKey::DRV_NOT_PLAUSIBLE)));
    tr_h4(content, TrKey::DRV_TRIGGER_RESISTANCE,
          String(datalayer_extended.bmwphev.iso_safety_trg_kohm) + " kΩ " +
              (datalayer_extended.bmwphev.iso_safety_trg_plausible ? TR(TrKey::DRV_PLAUSIBLE)
                                                                   : TR(TrKey::DRV_NOT_PLAUSIBLE)));
    content += "</div>";

    // Thermal Management Section
    content +=
        "<h3 style='color: #00acc1; border-bottom: 2px solid #00acc1; padding-bottom: 5px;'>❄️ Thermal Management</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4_open(content, TrKey::DRV_COOLING_VALVE_STATUS);
    switch (datalayer_extended.bmwphev.ST_valve_cooling) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_EVALUATED) + "</h4>");
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>⚠ Error!</span></h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_COLD_SHUTOFF_VALVE);
    switch (datalayer_extended.bmwphev.ST_cold_shutoff_valve) {
      case 0:
        tr_h4_end(content, TrKey::DRV_OK);
        break;
      case 1:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_SHORT_CIRCUIT_GND) + "</span></h4>");
        break;
      case 2:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_SHORT_CIRCUIT_12V) + "</span></h4>");
        break;
      case 3:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_LINE_BREAK) + "</span></h4>");
        break;
      case 6:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_DRIVER_ERROR) + "</span></h4>");
        break;
      case 12:
      case 13:
        content += String("<span style='color: #d32f2f;'>" + TR(TrKey::DRV_STUCK) + "</span></h4>");
        break;
      default:
        content += String(TR(TrKey::DRV_INVALID_SIGNAL) + "</h4>");
    }
    content += "</div>";

    // Cell Information Section
    content +=
        "<h3 style='color: #8e24aa; border-bottom: 2px solid #8e24aa; padding-bottom: 5px;'>📊 Cell Information</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4(content, TrKey::DRV_DETECTED_CELL_COUNT, String(datalayer.battery.info.number_of_cells));
    tr_h4(content, TrKey::DRV_MAX_CELL_DESIGN_VOLTAGE, String(datalayer.battery.info.max_cell_voltage_mV), " mV");
    tr_h4(content, TrKey::DRV_MIN_CELL_DESIGN_VOLTAGE, String(datalayer.battery.info.min_cell_voltage_mV), " mV");
    tr_h4(content, TrKey::DRV_MIN_CELL_VOLTAGE_DATA_AGE, String(datalayer_extended.bmwphev.min_cell_voltage_data_age),
          " ms");
    tr_h4(content, TrKey::DRV_MAX_CELL_VOLTAGE_DATA_AGE, String(datalayer_extended.bmwphev.max_cell_voltage_data_age),
          " ms");
    content += "</div>";

    // Balancing Status Section
    content +=
        "<h3 style='color: #5e35b1; border-bottom: 2px solid #5e35b1; padding-bottom: 5px;'>⚖️ Balancing Status</h3>";
    content += "<div style='margin-left: 15px;'>";
    content += "<p style='color: #bbb; font-style: italic; margin: 0 0 8px 0;'>" +
               TR(TrKey::DRV_BALANCING_CAN_ONLY_RUN_WHILE) +
               " "
               "contactors are OPEN and after the cells have settled at rest for ~10 min (see "
               "\"Inactive - Cells Not at Rest (Wait 10 min)\" below). It is blocked while the contactors are "
               "closed.</p>";
    tr_h4_open(content, TrKey::DRV_BALANCING);
    switch (datalayer_extended.bmwphev.balancing_status) {
      case 0:
        content += String(TR(TrKey::DRV_INACTIVE_NOT_NEEDED) + "</h4>");
        break;
      case 1:
        content += String("<span style='color: #43a047;'>" + TR(TrKey::DRV_ACTIVE_STATUS) + "</span></h4>");
        break;
      case 2:
        content += String(TR(TrKey::DRV_INACTIVE_CELLS_NOT_AT_REST_WAIT_10_MIN) + "</h4>");
        break;
      case 3:
        content += String(TR(TrKey::DRV_INACTIVE) + "</h4>");
        break;
      case 4:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    tr_h4_open(content, TrKey::DRV_BALANCING_REQUEST);
    content += datalayer.battery.settings.user_requests_balancing
                   ? String("<span style='color: #43a047;'>" + TR(TrKey::DRV_TRUE) + "</span>")
                   : String(TR(TrKey::DRV_FALSE));
    content += "</h4>";
    // Max balancing time before the safety timer auto-cancels the request (shared
    // balancing_max_time_ms, default 1h). Editable here via the existing /BalTime route, since the
    // PHEV uses supports_balancing_request() and so doesn't get the Tesla manual-balancing settings UI.
    tr_h4(content, TrKey::DRV_BALANCING_MAX_TIME,
          String(datalayer.battery.settings.balancing_max_time_ms / 60000.0f, 1) +
              " min <button onclick='editPhevBalTime()'>" + TR(TrKey::UI_EDIT) + "</button>");
    content +=
        "<script>"
        "function editPhevBalTime(){"
        "var v=prompt('" +
        TR_JS(TrKey::UI_ENTER_NEW_MAX_BALANCING_TIME_MINUTES_1_300_NOTE) +
        "');"
        "if(v===null){return;}"
        "if(v>=1&&v<=300){"
        "var x=new XMLHttpRequest();"
        "x.onload=function(){location.reload();};"
        "x.open('GET','/BalTime?value='+v,true);x.send();"
        "}else{alert('" +
        TR_JS(TrKey::UI_INVALID_VALUE_PLEASE_ENTER_A_VALUE_BETWEEN_1_AND_300) +
        "');}"
        "}"
        "</script>";
    content += "</div>";

    // Diagnostics Section
    content += "<h3 style='color: #757575; border-bottom: 2px solid #757575; padding-bottom: 5px;'>🔧 Diagnostics</h3>";
    content += "<div style='margin-left: 15px;'>";
    tr_h4(content, TrKey::DRV_CHARGING_CONDITION_DELTA,
          String(datalayer_extended.bmwphev.battery_charging_condition_delta));
    content += "</div>";

    content +=
        "<h3 style='color: #27b06c; border-bottom: 2px solid #27b06c; padding-bottom: 5px;'>🔧 Diagnostic Trouble " +
        TR(TrKey::DRV_CODES) + "</h3>";
    content += "<div style='margin-left: 15px; margin-right: 15px;'>";

    if (datalayer_extended.bmwphev.dtc_read_failed) {
      content += "<p style='color: #d32f2f;'>" + TR(TrKey::DRV_LAST_DTC_READ_FAILED_NOT_SUPPORTED) + "</p>";
    } else if (datalayer_extended.bmwphev.dtc_count == 0) {
      content += "<p style='color: #4CAF50;'>✓ No DTCs present</p>";
      if (datalayer_extended.bmwix.dtc_last_read_millis > 0) {
        content += "<p><strong>" + TR(TrKey::DRV_LAST_READ) + "</strong> " +
                   String((millis() - datalayer_extended.bmwix.dtc_last_read_millis) / 1000) + TR(TrKey::DRV_S_AGO) +
                   "</p>";
      }
    } else {
      content += "<p><strong>" + TR(TrKey::DRV_DTC_COUNT) + "</strong> " +
                 String(datalayer_extended.bmwphev.dtc_count) + "</p>";
      content += "<p><strong>" + TR(TrKey::DRV_LAST_READ) + "</strong> " +
                 String((millis() - datalayer_extended.bmwix.dtc_last_read_millis) / 1000) + TR(TrKey::DRV_S_AGO) +
                 "</p>";

      content += "<div style='overflow-x: auto; margin-top: 10px; margin-bottom: 15px;'>";
      content +=
          "<table style='width: auto; margin: 0 auto; border-collapse: separate; border-spacing: 0; border: 1px solid "
          "#ddd; border-radius: 8px; overflow: hidden;'>";

      content += "<thead>";
      content += "<tr style='background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white;'>";
      content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>DTC Code</th>";
      content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>Status</th>";
      content += "<th style='padding: 12px 15px; text-align: left; font-weight: 600;'>" + TR(TrKey::DRV_DESCRIPTION) +
                 "</th>";  // ✅ Add description column
      content += "</tr>";
      content += "</thead>";

      content += "<tbody>";

      for (int i = 0; i < datalayer_extended.bmwphev.dtc_count; i++) {
        uint32_t code = datalayer_extended.bmwix.dtc_codes[i];    //Note we re-use datalayer for iX to save space
        uint8_t status = datalayer_extended.bmwix.dtc_status[i];  //Note we re-use datalayer for iX to save space

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

      content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "bmw_phev_dtc.json");
    }

    content += "</div>";

    return content;
  }
};

#endif
