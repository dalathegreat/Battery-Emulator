#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"
#include "../devboard/webserver/html_escape.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl, const String& sfx = "") : byd_datalayer(dl), s(sfx) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;

    const auto& dl_bat = s.length() ? datalayer.battery2 : datalayer.battery;
    tr_h4(content, TrKey::DRV_DETECTED_CELLS, String(dl_bat.info.number_of_cells));
    tr_h4_open(content, TrKey::DRV_CONTACTOR_STATE);
    switch (byd_datalayer->contactor_control_state) {
      case 0:
        tr_h4_end(content, TrKey::DRV_CLOSING);
        break;
      case 1:
        tr_h4_end(content, TrKey::DRV_CLOSED_LIVE);
        break;
      case 2:
        tr_h4_end(content, TrKey::DRV_PREPARING_OPEN);
        break;
      case 3:
        tr_h4_end(content, TrKey::DRV_OPENING);
        break;
      case 4:
        tr_h4_end(content, TrKey::DRV_STANDBY_IDLE);
        break;
      case 5:
        tr_h4_end(content, TrKey::DRV_OPEN_REQUESTED);
        break;
      case 6:
        tr_h4_end(content, TrKey::DRV_OPEN_SETTLING);
        break;
      case 7:
        tr_h4_end(content, TrKey::DRV_HELD_OPEN_FAULT_E_STOP_STARTUP);
        break;
      default:
        tr_h4_end(content, TrKey::UI_UNKNOWN);
    }
    tr_h4_open(content, TrKey::DRV_MAIN_CONTACTORS);
    content +=
        byd_datalayer->contactor_main_closed ? "Closed &mdash; battery connected" : "Open &mdash; battery disconnected";
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_PRECHARGE_STATE);
    content += byd_datalayer->contactor_precharging ? "Active" : TR(TrKey::DRV_IDLE);
    content += "</h4>";
    // Bit2 (0x04) = car on/off (clear during car-off AC charging even though HV is live),
    // not literal HV-bus energisation.
    tr_h4_colon(content, TrKey::DRV_HV_ACTIVE,
                byd_datalayer->contactor_hv_active ? TR(TrKey::DRV_YES) : TR(TrKey::DRV_NO));
    // Pack mode read straight from the 0x344 byte0 state table (not re-derived per-bit).
    tr_h4_open(content, TrKey::DRV_BMS_PACK_MODE);
    switch (byd_datalayer->contactor_feedback) {
      case 0x00:
        tr_h4_end(content, TrKey::UI_DISCONNECTED);
        break;
      case 0x02:
        tr_h4_end(content, TrKey::DRV_OPEN_STANDBY);
        break;
      case 0x42:
        tr_h4_end(content, TrKey::DRV_PRECHARGING);
        break;
      case 0x80:
        tr_h4_end(content, TrKey::DRV_CLOSED_HV_INACTIVE);
        break;
      case 0x84:
        tr_h4_end(content, TrKey::DRV_CLOSED_IDLE_HV_ACTIVE);
        break;
      case 0x81:
        tr_h4_end(content, TrKey::DRV_CHARGING_CAR_OFF);
        break;
      case 0x85:
        tr_h4_end(content, TrKey::DRV_CHARGING_HV_ACTIVE);
        break;
      case 0x82:
        tr_h4_end(content, TrKey::DRV_DRIVE_READY_PENDING);
        break;
      case 0x86:
        tr_h4_end(content, TrKey::DRV_DRIVE_READY);
        break;
      default: {
        if (!(byd_datalayer->contactor_feedback & 0x80)) {
          content += TR(TrKey::UI_DISCONNECTED);
        } else if (byd_datalayer->contactor_feedback & 0x01) {
          content += TR(TrKey::DRV_CHARGING);
        } else if (byd_datalayer->contactor_feedback & 0x02) {
          content += TR(TrKey::DRV_DRIVE);
        } else {
          content += TR(TrKey::DRV_CLOSED_IDLE);
        }
        char modeStr[10];
        snprintf(modeStr, sizeof(modeStr), " (0x%02X)", byd_datalayer->contactor_feedback);
        content += modeStr;
        content += "</h4>";
      }
    }
    char feedbackStr[5];
    snprintf(feedbackStr, sizeof(feedbackStr), "0x%02X", byd_datalayer->contactor_feedback);
    tr_h4_open(content, TrKey::DRV_BMS_RAW_STATUS_MODE);
    content += feedbackStr;
    content += TR(TrKey::DRV_STATE) + " ";

    // 0x344 byte1 low nibble: a BMS state code whose meaning is unconfirmed (reads 1 in
    // idle/drive/discharge alike). byte0 is the real charge/drive truth, so show this raw.
    content += String(byd_datalayer->discharge_status);
    content += "</h4>";

    float soc_measured = static_cast<float>(byd_datalayer->SOC_highprec) * 0.1f;
    float BMS_maxChargePower = static_cast<float>(byd_datalayer->chargePower) * 0.1f;
    float BMS_maxDischargePower = static_cast<float>(byd_datalayer->dischargePower) * 0.1f;

    tr_h4(content, TrKey::DRV_SOC_MEASURED, String(soc_measured), "&percnt;");
    tr_h4(content, TrKey::DRV_SOC_OBD2, String(byd_datalayer->SOC_polled), "&percnt;");
    tr_h4_open(content, TrKey::DRV_PACK_VOLTAGE);
    if (byd_datalayer->pack_voltage_dV > 0) {
      content += String(byd_datalayer->pack_voltage_dV / 10.0f, 1) + " V</h4>";
    } else {
      tr_h4_end(content, TrKey::DRV_NOT_RECEIVED);
    }
    // 0x43A reports Ohm/V, so scale by pack voltage to get absolute resistance
    tr_h4_open(content, TrKey::DRV_INSULATION_RESISTANCE);
    if (byd_datalayer->insulation_valid) {
      float insulation_kohm =
          static_cast<float>(byd_datalayer->insulation_ohm_per_volt) * (dl_bat.status.voltage_dV / 10.0f) / 1000.0f;
      content += String(insulation_kohm, 1) + " k&Omega; (" + String(byd_datalayer->insulation_ohm_per_volt) +
                 " &Omega;/V)</h4>";
    } else {
      tr_h4_end(content, TrKey::DRV_NOT_RECEIVED);
    }
    if (byd_datalayer->battery_temperatures[0] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_1, String(byd_datalayer->battery_temperatures[0]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[1] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_2, String(byd_datalayer->battery_temperatures[1]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[2] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_3, String(byd_datalayer->battery_temperatures[2]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[3] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_4, String(byd_datalayer->battery_temperatures[3]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[4] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_5, String(byd_datalayer->battery_temperatures[4]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[5] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_6, String(byd_datalayer->battery_temperatures[5]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[6] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_7, String(byd_datalayer->battery_temperatures[6]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[7] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_8, String(byd_datalayer->battery_temperatures[7]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[8] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_9, String(byd_datalayer->battery_temperatures[8]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[9] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_10, String(byd_datalayer->battery_temperatures[9]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[10] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_11, String(byd_datalayer->battery_temperatures[10]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[11] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_12, String(byd_datalayer->battery_temperatures[11]), " &deg;C");
    }
    if (byd_datalayer->battery_temperatures[12] != 215) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_SENSOR_13, String(byd_datalayer->battery_temperatures[12]), " &deg;C");
    }
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_POWER, String(BMS_maxDischargePower), " kW");
    tr_h4(content, TrKey::DRV_MAX_CHARGE_REGEN_POWER, String(BMS_maxChargePower), " kW");
    tr_h4(content, TrKey::DRV_TOTAL_CHARGED, String(byd_datalayer->total_charged_kwh), " kWh");
    tr_h4(content, TrKey::DRV_TOTAL_DISCHARGED, String(byd_datalayer->total_discharged_kwh), " kWh");
    tr_h4(content, TrKey::DRV_TOTAL_CHARGED, String(byd_datalayer->total_charged_ah), " Ah");
    tr_h4(content, TrKey::DRV_TOTAL_DISCHARGED, String(byd_datalayer->total_discharged_ah), " Ah");
    tr_h4(content, TrKey::DRV_CHARGE_TIMES, String(byd_datalayer->charge_times));
    tr_h4(content, TrKey::DRV_TIMES_FULL_POWER, String(byd_datalayer->times_full_power));
    tr_h4(content, TrKey::DRV_MIN_CELL_VOLTAGE_NUMBER, String(byd_datalayer->BMS_min_cell_voltage_number));
    tr_h4(content, TrKey::DRV_MAX_CELL_VOLTAGE_NUMBER, String(byd_datalayer->BMS_max_cell_voltage_number));
    tr_h4(content, TrKey::DRV_MIN_TEMP_MODULE_NUMBER, String(byd_datalayer->BMS_min_temp_module_number));
    tr_h4(content, TrKey::DRV_MAX_TEMP_MODULE_NUMBER, String(byd_datalayer->BMS_max_temp_module_number));
    tr_h4(content, TrKey::DRV_SEED,
          String(byd_datalayer->seed) + " " + TR(TrKey::DRV_SOLVEDKEY) + " " + String(byd_datalayer->solvedKey));
    if (byd_datalayer->servicemode == 0) {
      tr_h4(content, TrKey::DRV_SERVICEMODE_NO_COMMAND_RAN_YET);
    } else if (byd_datalayer->servicemode == 1) {
      tr_h4(content, TrKey::DRV_SERVICEMODE_REJECTED);
    } else if (byd_datalayer->servicemode == 2) {
      tr_h4(content, TrKey::DRV_SERVICEMODE_APPROVED);
    }
    tr_h4(content, TrKey::DRV_CAPACITY_ORIGNAL, String((byd_datalayer->BMS_capacity_original_calibration) / 100), "AH");
    tr_h4(content, TrKey::DRV_CAPACITY_CURRENT, String((byd_datalayer->BMS_capacity_current_calibration) / 100), "AH");
    tr_h4(content, TrKey::DRV_SOC_ORIGINAL, String(byd_datalayer->BMC_SOC_original_calibration), "&percnt;");
    tr_h4(content, TrKey::DRV_SOC_CURRENT, String(byd_datalayer->BMC_SOC_current_calibration), "&percnt;");

    content += "<h4>Auto-calibrate SOC to 100&percnt; when full: <input type='checkbox' id='autoCalEnabled" + s + "' ";
    content += (byd_datalayer->auto_calibrate_soc_enabled ? "checked" : "");
    content += " onchange='toggleAutoCalSOCEnabled" + s + "()'> (default ON)</h4>";
    content += "<h4>Auto-calibrate trigger drift: <input type='number' id='driftPercent" + s + "' value='";
    content += String(byd_datalayer->auto_calibrate_soc_drift_percent);
    content += "' min='1' max='20'> &percnt; <button onclick='setAutoCalDriftPercent" + s +
               "()'>Save Drift &percnt;</button></h4>";

    // Auto-calibration live status panel
    {
      uint32_t dwell_sec = byd_datalayer->autocal_dwell_accumulated_ms / 1000;
      uint32_t dwell_min = dwell_sec / 60;
      uint32_t dwell_rem = dwell_sec % 60;
      uint32_t grace_sec = byd_datalayer->autocal_grace_timer_ms / 1000;
      float autocal_current_A = static_cast<float>(byd_datalayer->autocal_current_dA) / 10.0f;
      String current_direction = TR(TrKey::DRV_IDLE);
      if (byd_datalayer->autocal_current_dA < 0) {
        current_direction = TR(TrKey::DRV_DISCHARGE);
      } else if (byd_datalayer->autocal_current_dA > 0) {
        current_direction = TR(TrKey::DRV_CHARGE);
      }
      bool dwell_done = byd_datalayer->autocal_crit_dwell;
      const char* label_td = "<td style='padding:3px 14px 3px 0;color:#d8dee4'>";
      const char* value_td = "<td style='padding:3px 0;color:white;font-weight:bold'>";

      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content += "<h4 style='margin:0 0 8px 0;color:white'>" + TR(TrKey::DRV_AUTO_CALIBRATION_STATUS) + "</h4>";
      content += "<table style='margin:0 auto;border-collapse:collapse;font-size:0.95em;text-align:left;color:white'>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_CONTACTORS) + "</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_contactors
                     ? "<span style='color:#3fb950'>OK</span>"
                     : "<span style='color:#ff7b72'>" + TR(TrKey::DRV_OPEN) + "</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_FULL_TAPER) + "</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_taper ? "<span style='color:#3fb950'>" + TR(TrKey::DRV_YES) + "</span>"
                                                   : "<span style='color:#ff7b72'>No</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_BATTERY_CURRENT) + "</td>";
      content += value_td;
      content += String(autocal_current_A, 1) + " A (" + String(current_direction) + ")";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_CURRENT_RANGE) + "</td>";
      content += value_td;
      if (!byd_datalayer->autocal_crit_taper) {
        content += "<span style='color:#8b949e'>" + TR(TrKey::DRV_WAITING_TAPER) + "</span>";
      } else if (byd_datalayer->autocal_crit_low_current) {
        content += "<span style='color:#3fb950'>" + TR(TrKey::DRV_YES) + "</span>";
        content += " <span style='color:#8b949e;font-weight:normal'>(chg &le;3A, disch &le;0.5A)</span>";
      } else {
        content += "<span style='color:#d29922'>No &mdash; " + String(grace_sec) + "s / 60s</span>";
      }
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_DWELL_TIME) + "</td>";
      content += value_td;
      content += dwell_done ? "<span style='color:#3fb950'>" : "";
      content += String(dwell_min) + "m " + String(dwell_rem) + "s / 10m";
      content += dwell_done ? "</span>" : "";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_SOC_DRIFT) + "</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_drift ? "<span style='color:#3fb950'>" : "";
      content += String(byd_datalayer->autocal_drift_percent, 1) + "&percnt; / threshold " +
                 String(byd_datalayer->auto_calibrate_soc_drift_percent) + "&percnt;";
      content += byd_datalayer->autocal_crit_drift ? "</span>" : "";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += TR(TrKey::DRV_COOLDOWN) + "</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_cooldown_ready
                     ? "<span style='color:#3fb950'>" + TR(TrKey::DRV_READY) + "</span>"
                     : "<span style='color:#d29922'>" + TR(TrKey::DRV_WAITING) + "</span>";
      content += "</td></tr>";

      content += "</table>";
      content += "</div>";
    }

    tr_h4(content, TrKey::DRV_CALIBRATION_TARGET_SOC,
          String(byd_datalayer->calibrationTargetSOC) + "&percnt; <button onclick='editCalTargetSOC" + s + "()'>" +
              TR(TrKey::UI_EDIT) + "</button>");
    tr_h4(content, TrKey::DRV_CALIBRATION_TARGET_CAPACITY,
          String(byd_datalayer->calibrationTargetAH) + " AH <button onclick='editCalTargetAH" + s + "()'>" +
              TR(TrKey::UI_EDIT) + "</button>");

    // Isolation monitor. Status is per battery; the controls are shown once and apply to both.
    {
      const char* iso_label_td = "<td style='padding:3px 14px 3px 0;color:#d8dee4'>";
      const char* iso_value_td = "<td style='padding:3px 0;color:white;font-weight:bold'>";

      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content += "<h4 style='margin:0 0 8px 0;color:white'>Isolation resistance monitor</h4>";
      content += "<table style='margin:0 auto;border-collapse:collapse;font-size:0.95em;text-align:left;color:white'>";

      // Monitoring status from 0x35E b0 bit0x80; only unambiguous when the pack is closed
      bool iso_closed = byd_datalayer->contactor_main_closed;
      bool iso_active = byd_datalayer->iso_measurement_active;
      content += "<tr>";
      content += iso_label_td;
      content += "Monitoring:</td>";
      content += iso_value_td;
      if (!byd_datalayer->iso_status_valid) {
        content += "<span style='color:#8b949e'>Unknown</span>";
      } else if (!iso_closed) {
        content += "<span style='color:#8b949e'>Inactive (pack open)</span>";
      } else if (iso_active) {
        content += "<span style='color:#3fb950'>On</span>";
      } else {
        content += "<span style='color:#ff7b72'>Off</span>";
      }
      content += "</td></tr>";

      content += "<tr>";
      content += iso_label_td;
      content += "Insulation resistance:</td>";
      content += iso_value_td;
      if (byd_datalayer->insulation_valid) {
        float iso_kohm =
            static_cast<float>(byd_datalayer->insulation_ohm_per_volt) * (dl_bat.status.voltage_dV / 10.0f) / 1000.0f;
        content += String(iso_kohm, 1) + " k&Omega; (" + String(byd_datalayer->insulation_ohm_per_volt) + " &Omega;/V)";
      } else {
        content += "<span style='color:#8b949e'>Not received</span>";
      }
      content += "</td></tr>";

      // Command feedback, only shown while something is pending or after a failure
      if (byd_datalayer->iso_command_status == 1 || byd_datalayer->iso_command_status == 3 ||
          byd_datalayer->iso_command_status == 4) {
        content += "<tr>";
        content += iso_label_td;
        content += "Last command:</td>";
        content += iso_value_td;
        if (byd_datalayer->iso_command_status == 1) {
          content += "<span style='color:#d29922'>Sending&hellip;</span>";
        } else if (byd_datalayer->iso_command_status == 3) {
          content += "<span style='color:#ff7b72'>Rejected</span>";
        } else {
          content += "<span style='color:#ff7b72'>No response</span>";
        }
        content += "</td></tr>";
      }

      if (s.length() == 0) {  // one set of controls, applied to every BYD battery
        content += "<tr>";
        content += iso_label_td;
        content += "Keep disabled at boot:</td>";
        content += iso_value_td;
        content += "<input type='checkbox' id='keepIsoOff' ";
        content += (byd_datalayer->keep_iso_disabled ? "checked" : "");
        content += " onchange='bydKeepIsoDisabled()'>";
        content += "</td></tr>";
      }

      content += "</table>";
      if (s.length() == 0) {
        content += "<div style='margin:10px 0 0'>";
        content += "<button onclick='bydIsoEnable()'>" + TR(TrKey::DRV_ENABLE_MONITORING) + "</button> ";
        content += "<button onclick='bydIsoDisable()'>" + TR(TrKey::DRV_DISABLE_MONITORING) + "</button>";
        content += "</div>";
      }
      content += "</div>";
    }

    content += "<script>";
    content += "function editComplete() {";
    content += "  alert('" + TR_JS(TrKey::DRV_ALERT_UPDATE_SUCCESSFUL) + "');";
    content += "  setTimeout(function() { location.reload(); }, 1000);";
    content += "}";
    content += "function editError() {";
    content += "  alert('" + TR_JS(TrKey::DRV_ALERT_UPDATE_FAILED_PLEASE_TRY_AGAIN) + "');";
    content += "}";
    content += "function editCalTargetSOC" + s + "(){";
    content += "  var value=prompt('" + TR_JS(TrKey::UI_ENTER_CALIBRATION_TARGET_SOC_0_TO_100) + "');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>=0 && numValue<=100){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetSOC" + s + "?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('" + TR_JS(TrKey::DRV_ALERT_INVALID_VALUE_PLEASE_ENTER_VALUE_BETWEEN_0_100) + "');";
    content += "    }";
    content += "  }";
    content += "}";
    content += "function editCalTargetAH" + s + "(){";
    content += "  var value=prompt('" + TR_JS(TrKey::UI_ENTER_CALIBRATION_TARGET_AH) + "');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>0){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetAH" + s + "?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('" + TR_JS(TrKey::DRV_ALERT_INVALID_VALUE_PLEASE_ENTER_POSITIVE_NUMBER) + "');";
    content += "    }";
    content += "  }";
    content += "}";
    content += "function toggleAutoCalSOCEnabled" + s + "(){";
    content += "  var enabled = document.getElementById('autoCalEnabled" + s + "').checked ? 1 : 0;";
    content += "  var xhr=new XMLHttpRequest();";
    content += "  xhr.onload=editComplete;";
    content += "  xhr.onerror=editError;";
    content += "  xhr.open('GET','/editBydAtto3AutoCalEnabled" + s + "?value='+enabled,true);";
    content += "  xhr.send();";
    content += "}";
    content += "function setAutoCalDriftPercent" + s + "(){";
    content += "  var percent = document.getElementById('driftPercent" + s + "').value;";
    content += "  var xhr=new XMLHttpRequest();";
    content += "  xhr.onload=editComplete;";
    content += "  xhr.onerror=editError;";
    content += "  xhr.open('GET','/editBydAtto3AutoCalDriftPercent" + s + "?value='+percent,true);";
    content += "  xhr.send();";
    content += "}";
    if (s.length() == 0) {  // isolation controls are rendered once and apply to every BYD battery
      content += "function bydIsoSend(url){";
      content += "  var xhr=new XMLHttpRequest();";
      content += "  xhr.onload=function(){ setTimeout(function(){ location.reload(); }, 2000); };";
      content += "  xhr.onerror=editError;";
      content += "  xhr.open('GET',url,true);";
      content += "  xhr.send();";
      content += "}";
      content += "function bydIsoEnable(){ bydIsoSend('/bydAtto3IsoEnable'); }";
      content += "function bydIsoDisable(){ bydIsoSend('/bydAtto3IsoDisable'); }";
      content += "function bydKeepIsoDisabled(){";
      content += "  var on = document.getElementById('keepIsoOff').checked;";
      content += "  var xhr=new XMLHttpRequest();";
      content += "  xhr.onload=editComplete;";
      content += "  xhr.onerror=editError;";
      content += "  xhr.open('GET','/bydAtto3KeepIsoDisabled?value='+(on?1:0),true);";
      content += "  xhr.send();";
      content += "}";
    }
    content += "</script>";

    auto& dtc = s.length() ? datalayer.battery2.dtc : datalayer.battery.dtc;
    content += BatteryHtmlRenderer::render_dtc_section_html(dtc, "byd_atto3_dtc.json", true);

    return content;
  }

 private:
  DATALAYER_INFO_BYDATTO3* byd_datalayer;
  String s;
};

#endif
