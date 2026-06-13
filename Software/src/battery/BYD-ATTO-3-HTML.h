#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl, const String& sfx = "") : byd_datalayer(dl), s(sfx) {}

  String get_status_html() {
    String content;

    const auto& dl_bat = s.length() ? datalayer.battery2 : datalayer.battery;
    content += "<h4>Detected cells: " + String(dl_bat.info.number_of_cells) + "</h4>";
    content += "<h4>Contactor command: ";
    switch (byd_datalayer->contactor_control_state) {
      case 0:
        content += "Closing</h4>";
        break;
      case 1:
        content += "Closed</h4>";
        break;
      case 2:
        content += "Preparing to open</h4>";
        break;
      case 3:
        content += "Opening</h4>";
        break;
      case 4:
        content += "Open</h4>";
        break;
      case 5:
        content += "Opening</h4>";
        break;
      case 6:
        content += "Open</h4>";
        break;
      case 7:
        content += "Checking battery</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>Contactor state: ";
    content +=
        byd_datalayer->contactor_main_closed ? "Closed &mdash; battery connected" : "Open &mdash; battery disconnected";
    content += "</h4>";
    content += "<h4>Contactor precharge: ";
    content += byd_datalayer->contactor_precharging ? "Precharging" : "No";
    content += "</h4>";
    content += "<h4>Contactor HV active: ";
    content += byd_datalayer->contactor_hv_active ? "Yes" : "No";
    content += "</h4>";
    // Which power mode the pack is in - drive vs charge picks how it opens
    content += "<h4>Contactor mode: ";
    if (!byd_datalayer->contactor_main_closed) {
      content += "Disconnected</h4>";
    } else if (byd_datalayer->contactor_charge_flag) {
      content += "Charging</h4>";
    } else if (byd_datalayer->contactor_drive_flag) {
      content += "Active</h4>";
    } else {
      content += "Standby</h4>";
    }
    char feedbackStr[5];
    snprintf(feedbackStr, sizeof(feedbackStr), "0x%02X", byd_datalayer->contactor_feedback);
    content += "<h4>Contactor diagnostic code: ";
    content += feedbackStr;
    content += "</h4>";

    content += "<h4>BMS reported state: ";
    switch (byd_datalayer->discharge_status) {
      case 0:
        content += "Ready</h4>";
        break;
      case 1:
        content += "Charging</h4>";
        break;
      case 2:
        content += "Charge finished</h4>";
        break;
      case 3:
        content += "Discharging</h4>";
        break;
      case 4:
        content += "Charge terminated</h4>";
        break;
      case 5:
        content += "Breakdown C10</h4>";
        break;
      case 6:
        content += "Breakdown charging plug</h4>";
        break;
      case 7:
        content += "Breakdown charger</h4>";
        break;
      case 8:
        content += "Breakdown AC</h4>";
        break;
      case 9:
        content += "Schedule</h4>";
        break;
      case 10:
        content += "Discharge CBU</h4>";
        break;
      case 11:
        content += "Timeout</h4>";
        break;
      case 12:
        content += "Discharge finish</h4>";
        break;
      case 13:
        content += "Charging pause</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }

    float soc_measured = static_cast<float>(byd_datalayer->SOC_highprec) * 0.1f;
    float BMS_maxChargePower = static_cast<float>(byd_datalayer->chargePower) * 0.1f;
    float BMS_maxDischargePower = static_cast<float>(byd_datalayer->dischargePower) * 0.1f;

    content += "<h4>SOC measured: " + String(soc_measured) + "&percnt;</h4>";
    content += "<h4>SOC OBD2: " + String(byd_datalayer->SOC_polled) + "&percnt;</h4>";
    content += "<h4>Voltage periodic: " + String(byd_datalayer->voltage_periodic) + " V</h4>";
    content += "<h4>Voltage OBD2: " + String(byd_datalayer->voltage_polled) + " V</h4>";
    if (byd_datalayer->battery_temperatures[0] != 215) {
      content += "<h4>Temperature sensor 1: " + String(byd_datalayer->battery_temperatures[0]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[1] != 215) {
      content += "<h4>Temperature sensor 2: " + String(byd_datalayer->battery_temperatures[1]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[2] != 215) {
      content += "<h4>Temperature sensor 3: " + String(byd_datalayer->battery_temperatures[2]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[3] != 215) {
      content += "<h4>Temperature sensor 4: " + String(byd_datalayer->battery_temperatures[3]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[4] != 215) {
      content += "<h4>Temperature sensor 5: " + String(byd_datalayer->battery_temperatures[4]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[5] != 215) {
      content += "<h4>Temperature sensor 6: " + String(byd_datalayer->battery_temperatures[5]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[6] != 215) {
      content += "<h4>Temperature sensor 7: " + String(byd_datalayer->battery_temperatures[6]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[7] != 215) {
      content += "<h4>Temperature sensor 8: " + String(byd_datalayer->battery_temperatures[7]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[8] != 215) {
      content += "<h4>Temperature sensor 9: " + String(byd_datalayer->battery_temperatures[8]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[9] != 215) {
      content += "<h4>Temperature sensor 10: " + String(byd_datalayer->battery_temperatures[9]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[10] != 215) {
      content += "<h4>Temperature sensor 11: " + String(byd_datalayer->battery_temperatures[10]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[11] != 215) {
      content += "<h4>Temperature sensor 12: " + String(byd_datalayer->battery_temperatures[11]) + " &deg;C</h4>";
    }
    if (byd_datalayer->battery_temperatures[12] != 215) {
      content += "<h4>Temperature sensor 13: " + String(byd_datalayer->battery_temperatures[12]) + " &deg;C</h4>";
    }
    content += "<h4>Max discharge power: " + String(BMS_maxDischargePower) + " kW</h4>";
    content += "<h4>Max charge (regen) power: " + String(BMS_maxChargePower) + " kW</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_kwh) + " kWh</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_kwh) + " kWh</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_ah) + " Ah</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_ah) + " Ah</h4>";
    content += "<h4>Charge times: " + String(byd_datalayer->charge_times) + "</h4>";
    content += "<h4>Times of full power: " + String(byd_datalayer->times_full_power) + "</h4>";
    content += "<h4>Min cell voltage number: " + String(byd_datalayer->BMS_min_cell_voltage_number) + "</h4>";
    content += "<h4>Max cell voltage number: " + String(byd_datalayer->BMS_max_cell_voltage_number) + "</h4>";
    content += "<h4>Min temp module number: " + String(byd_datalayer->BMS_min_temp_module_number) + "</h4>";
    content += "<h4>Max temp module number: " + String(byd_datalayer->BMS_max_temp_module_number) + "</h4>";
    content += "<h4>Seed: " + String(byd_datalayer->seed) + " SolvedKey: " + String(byd_datalayer->solvedKey) + "</h4>";
    if (byd_datalayer->servicemode == 0) {
      content += "<h4>ServiceMode: No command ran yet</h4>";
    } else if (byd_datalayer->servicemode == 1) {
      content += "<h4>ServiceMode: REJECTED </h4>";
    } else if (byd_datalayer->servicemode == 2) {
      content += "<h4>ServiceMode: APPROVED! </h4>";
    }
    content += "<h4>Capacity orignal: " + String((byd_datalayer->BMS_capacity_original_calibration) / 100) + "AH</h4>";
    content += "<h4>Capacity current: " + String((byd_datalayer->BMS_capacity_current_calibration) / 100) + "AH</h4>";
    content += "<h4>SOC original: " + String(byd_datalayer->BMC_SOC_original_calibration) + "&percnt;</h4>";
    content += "<h4>SOC current: " + String(byd_datalayer->BMC_SOC_current_calibration) + "&percnt;</h4>";

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
      const char* current_direction = "idle";
      if (byd_datalayer->autocal_current_dA < 0) {
        current_direction = "discharge";
      } else if (byd_datalayer->autocal_current_dA > 0) {
        current_direction = "charge";
      }
      bool dwell_done = byd_datalayer->autocal_crit_dwell;
      const char* label_td = "<td style='padding:3px 14px 3px 0;color:#d8dee4'>";
      const char* value_td = "<td style='padding:3px 0;color:white;font-weight:bold'>";

      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content += "<h4 style='margin:0 0 8px 0;color:white'>Auto-calibration status</h4>";
      content += "<table style='margin:0 auto;border-collapse:collapse;font-size:0.95em;text-align:left;color:white'>";

      content += "<tr>";
      content += label_td;
      content += "Contactors:</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_contactors ? "<span style='color:#3fb950'>OK</span>"
                                                        : "<span style='color:#ff7b72'>Open</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Full / In taper?</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_taper ? "<span style='color:#3fb950'>Yes</span>"
                                                   : "<span style='color:#ff7b72'>No</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Battery current:</td>";
      content += value_td;
      content += String(autocal_current_A, 1) + " A (" + String(current_direction) + ")";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Current in range:</td>";
      content += value_td;
      if (!byd_datalayer->autocal_crit_taper) {
        content += "<span style='color:#8b949e'>Waiting for taper</span>";
      } else if (byd_datalayer->autocal_crit_low_current) {
        content += "<span style='color:#3fb950'>Yes</span>";
        content += " <span style='color:#8b949e;font-weight:normal'>(chg &le;3A, disch &le;0.5A)</span>";
      } else {
        content += "<span style='color:#d29922'>No &mdash; " + String(grace_sec) + "s / 60s</span>";
      }
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Dwell time:</td>";
      content += value_td;
      content += dwell_done ? "<span style='color:#3fb950'>" : "";
      content += String(dwell_min) + "m " + String(dwell_rem) + "s / 10m";
      content += dwell_done ? "</span>" : "";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "SOC drift:</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_drift ? "<span style='color:#3fb950'>" : "";
      content += String(byd_datalayer->autocal_drift_percent, 1) + "&percnt; / threshold " +
                 String(byd_datalayer->auto_calibrate_soc_drift_percent) + "&percnt;";
      content += byd_datalayer->autocal_crit_drift ? "</span>" : "";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Cooldown:</td>";
      content += value_td;
      content += byd_datalayer->autocal_crit_cooldown_ready ? "<span style='color:#3fb950'>Ready</span>"
                                                            : "<span style='color:#d29922'>Waiting</span>";
      content += "</td></tr>";

      content += "</table>";
      content += "</div>";
    }

    content += "<h4>Calibration target SOC: " + String(byd_datalayer->calibrationTargetSOC) +
               "&percnt; <button onclick='editCalTargetSOC" + s + "()'>Edit</button></h4>";
    content += "<h4>Calibration target capacity: " + String(byd_datalayer->calibrationTargetAH) +
               " AH <button onclick='editCalTargetAH" + s + "()'>Edit</button></h4>";

    content += "<script>";
    content += "function editComplete() {";
    content += "  alert('Update successful!');";
    content += "  setTimeout(function() { location.reload(); }, 1000);";
    content += "}";
    content += "function editError() {";
    content += "  alert('Update failed. Please try again.');";
    content += "}";
    content += "function editCalTargetSOC" + s + "(){";
    content += "  var value=prompt('Enter calibration target SOC (0 to 100):');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>=0 && numValue<=100){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetSOC" + s + "?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('Invalid value. Please enter a value between 0 and 100.');";
    content += "    }";
    content += "  }";
    content += "}";
    content += "function editCalTargetAH" + s + "(){";
    content += "  var value=prompt('Enter calibration target AH:');";
    content += "  if(value!==null){";
    content += "    var numValue=parseFloat(value);";
    content += "    if(!isNaN(numValue) && numValue>0){";
    content += "      var xhr=new XMLHttpRequest();";
    content += "      xhr.onload=editComplete;";
    content += "      xhr.onerror=editError;";
    content += "      xhr.open('GET','/editCalTargetAH" + s + "?value='+numValue,true);";
    content += "      xhr.send();";
    content += "    }else{";
    content += "      alert('Invalid value. Please enter a positive number.');";
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
    content += "</script>";

    return content;
  }

 private:
  DATALAYER_INFO_BYDATTO3* byd_datalayer;
  String s;
};

#endif
