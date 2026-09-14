#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl, const String& sfx = "") : byd_datalayer(dl), s(sfx) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    content.reserve(16000);

    const auto& dl_bat = s.length() ? datalayer.battery2 : datalayer.battery;
    content += "<h4>Detected cells: " + String(dl_bat.info.number_of_cells) + "</h4>";
    content += "<h4>BE contactor state: ";
    switch (byd_datalayer->contactor_control_state) {
      case 0:
        content += "Closing</h4>";
        break;
      case 1:
        content += "Closed (live)</h4>";
        break;
      case 2:
        content += "Preparing to open</h4>";
        break;
      case 3:
        content += "Opening</h4>";
        break;
      case 4:
        content += "Standby / idle</h4>";
        break;
      case 5:
        content += "Open requested</h4>";
        break;
      case 6:
        content += "Open (settling)</h4>";
        break;
      case 7:
        content += "Held open (fault / e-stop / startup)</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>Main contactors: ";
    content +=
        byd_datalayer->contactor_main_closed ? "Closed &mdash; battery connected" : "Open &mdash; battery disconnected";
    content += "</h4>";
    content += "<h4>Precharge state: ";
    content += byd_datalayer->contactor_precharging ? "Active" : "Idle";
    content += "</h4>";
    // Bit2 (0x04) = car on/off (clear during car-off AC charging even though HV is live),
    // not literal HV-bus energisation.
    content += "<h4>HV active: ";
    content += byd_datalayer->contactor_hv_active ? "Yes" : "No";
    content += "</h4>";
    // Pack mode read straight from the 0x344 byte0 state table (not re-derived per-bit).
    content += "<h4>BMS pack mode: ";
    switch (byd_datalayer->contactor_feedback) {
      case 0x00:
        content += "Disconnected</h4>";
        break;
      case 0x02:
        content += "Open standby</h4>";
        break;
      case 0x42:
        content += "Precharging</h4>";
        break;
      case 0x80:
        content += "Closed, HV inactive</h4>";
        break;
      case 0x84:
        content += "Closed idle, HV active</h4>";
        break;
      case 0x81:
        content += "Charging, car off</h4>";
        break;
      case 0x85:
        content += "Charging, HV active</h4>";
        break;
      case 0x82:
        content += "Drive-ready pending</h4>";
        break;
      case 0x86:
        content += "Drive ready</h4>";
        break;
      default: {
        if (!(byd_datalayer->contactor_feedback & 0x80)) {
          content += "Disconnected";
        } else if (byd_datalayer->contactor_feedback & 0x01) {
          content += "Charging";
        } else if (byd_datalayer->contactor_feedback & 0x02) {
          content += "Drive";
        } else {
          content += "Closed idle";
        }
        char modeStr[10];
        snprintf(modeStr, sizeof(modeStr), " (0x%02X)", byd_datalayer->contactor_feedback);
        content += modeStr;
        content += "</h4>";
      }
    }
    char feedbackStr[5];
    snprintf(feedbackStr, sizeof(feedbackStr), "0x%02X", byd_datalayer->contactor_feedback);
    content += "<h4>BMS raw status: mode ";
    content += feedbackStr;
    content += ", state ";

    // 0x344 byte1 low nibble: a BMS state code whose meaning is unconfirmed (reads 1 in
    // idle/drive/discharge alike). byte0 is the real charge/drive truth, so show this raw.
    content += String(byd_datalayer->discharge_status);
    content += "</h4>";

    float soc_measured = static_cast<float>(byd_datalayer->SOC_highprec) * 0.1f;
    float BMS_maxChargePower = static_cast<float>(byd_datalayer->chargePower) * 0.1f;
    float BMS_maxDischargePower = static_cast<float>(byd_datalayer->dischargePower) * 0.1f;

    content += "<h4>SOC measured: " + String(soc_measured) + "&percnt;</h4>";
    content += "<h4>SOC 0x444: " + String(byd_datalayer->SOC_polled) + "&percnt;</h4>";
    content += "<h4>Pack voltage: ";
    if (byd_datalayer->pack_voltage_dV > 0) {
      content += String(byd_datalayer->pack_voltage_dV / 10.0f, 1) + " V</h4>";
    } else {
      content += "Not received</h4>";
    }
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

    // Shared geometry for the three calibration panels below, so they line up with each other.
    // Percent widths are written as &percnt; so no literal '%' reaches the template engine, which would
    // otherwise treat pairs of '%' as placeholder markers and delete the content between them.
    const char* label_td = "<td style='padding:3px 14px 3px 0;color:#d8dee4;width:50&percnt;;text-align:right'>";
    const char* value_td = "<td style='padding:3px 0;color:white;font-weight:bold;text-align:left'>";
    const char* panel_table =
        "<table style='margin:0 auto;border-collapse:collapse;font-size:0.95em;text-align:left;color:white;"
        "width:100&percnt;;max-width:460px;table-layout:fixed'>";

    // Native BMS termination. On by default. The battery only enters a charge session when it is not
    // reporting an insulation fault; the isolation-monitor-disable option (also on by default) normally
    // keeps that clear, so a pack out of a car does not need its case isolated from earth for this.
    // Primary battery only - the inverter charge limit follows battery 1, so a session on a second
    // battery could not stop the bank when it terminates.
    if (s.length() > 0) {
      content += "<hr>";
      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content +=
          "<h4 style='margin:0 0 8px 0;color:white'>Native SOC calibration, charge termination &amp; balancing</h4>";
      content +=
          "<div style='margin:0 0 10px;font-size:0.9em;color:#8b949e'>Not available on the second battery: "
          "the inverter charge limit follows battery 1, so a termination here could not stop the "
          "charge.</div>";
      content += "</div>";
      content += "<hr>";
    } else {
      const char* session_text = "Idle";
      const char* session_colour = "#8b949e";  // grey while nothing is running
      bool show_elapsed = false;
      switch (byd_datalayer->charge_session_state) {
        case 1:
          session_text = "Requesting charge";
          session_colour = "#d29922";
          break;
        case 2:
          session_text = "Ready, waiting for battery";
          session_colour = "#d29922";
          break;
        case 3:
          session_text = "Charging";
          session_colour = "#3fb950";
          break;
        case 4:
          session_text = "Finishing";
          session_colour = "#d29922";
          break;
        case 5:
          session_text = "Resting";
          session_colour = "white";
          show_elapsed = true;  // rest time is the interesting quantity in this state
          break;
        default:
          break;
      }

      content += "<hr>";
      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content +=
          "<h4 style='margin:0 0 8px 0;color:white'>Native SOC calibration, charge termination &amp; balancing</h4>";
      content +=
          "<div style='margin:0 0 10px;font-size:0.9em;color:#8b949e'>Native termination lets the battery "
          "finish charging and recalibrate its own SOC and SOH, as it does in the car. Without balancing, the "
          "pack remains closed and available for discharge. With balancing enabled, the contactors open for the "
          "selected hold time before closing again &mdash; a cycle that appears to trigger balancing.</div>";
      content += panel_table;

      content += "<tr>";
      content += label_td;
      content += "Enabled:</td>";
      content += value_td;
      content += "<input type='checkbox' id='nativeTerm" + s + "' ";
      content += (byd_datalayer->native_termination_enabled ? "checked" : "");
      content += " onchange='toggleNativeTermination" + s + "()'>";
      content += "<span style='font-weight:normal;color:#8b949e'> default on</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Balancing enabled:</td>";
      content += value_td;
      content += "<input type='checkbox' id='balancingEnabled' ";
      content += (byd_datalayer->balancing_enabled ? "checked" : "");
      content += " onchange='toggleBalancingEnabled()'>";
      content += "<span style='font-weight:normal;color:#8b949e'> open/reclose after charge</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Hold for:</td>";
      content += value_td;
      content +=
          "<input type='number' style='width:4.5em;margin:0;padding:1px 4px;vertical-align:middle' "
          "id='balancingMinutes' value='";
      content += String(byd_datalayer->balancing_hold_minutes);
      content +=
          "' min='1' max='1440'> min <button style='margin:0 0 0 8px;padding:2px 12px;vertical-align:middle' "
          "onclick='setBalancingMinutes()'>Save</button>";
      content += "</td></tr>";

      if (byd_datalayer->balancing_state != 0) {
        const char* hold_text = "Idle";
        switch (byd_datalayer->balancing_state) {
          case 1:
            hold_text = "Armed";
            break;
          case 2:
            hold_text = "Opening";
            break;
          case 3:
            hold_text = "Holding open";
            break;
          case 4:
            hold_text = "Closing";
            break;
          case 5:
            hold_text = "Close failed - pack left open";
            break;
          default:
            break;
        }
        content += "<tr>";
        content += label_td;
        content += "Hold state:</td>";
        content += value_td;
        content +=
            (byd_datalayer->balancing_state == 5) ? "<span style='color:#d29922'>" : "<span style='color:white'>";
        content += hold_text;
        if (byd_datalayer->balancing_state == 3) {
          content += " (" + String(byd_datalayer->balancing_remaining_min) + " min left)";
        }
        content += "</span></td></tr>";
      }

      content += "<tr>";
      content += label_td;
      content += "Charge session:</td>";
      content += value_td;
      content += "<span style='color:";
      content += session_colour;
      content += "'>";
      content += session_text;
      content += "</span>";
      if (show_elapsed) {
        uint32_t rest_min = byd_datalayer->charge_session_seconds / 60;
        content += "<span style='font-weight:normal;color:#8b949e'> (";
        if (rest_min >= 60) {
          content += String(rest_min / 60) + "h " + String(rest_min % 60) + "m";
        } else {
          content += String(rest_min) + "m";
        }
        content += ")</span>";
      }
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Grant from battery:</td>";
      content += value_td;
      // Only zero versus non-zero is decoded, so lead with that. The raw value is kept for
      // diagnostics but means nothing on its own: it ramps and sawtooths without the charger
      // ever following it.
      if (byd_datalayer->charge_session_state == 0) {
        content += "<span style='color:#8b949e'>&mdash;</span>";
      } else if (byd_datalayer->charge_grant > 0) {
        content += "<span style='color:#3fb950'>Granted</span>";
        content +=
            "<span style='font-weight:normal;color:#8b949e'> (" + String(byd_datalayer->charge_grant) + ")</span>";
      } else {
        content += "<span style='color:#d29922'>Not granted</span>";
      }
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Last termination:</td>";
      content += value_td;
      if (byd_datalayer->termination_cell_max_mV > 0) {
        content += String(byd_datalayer->termination_cell_max_mV) + " mV top cell, " +
                   String(byd_datalayer->termination_cell_delta_mV) + " mV spread";
      } else {
        content += "<span style='color:#8b949e'>None yet</span>";
      }
      content += "</td></tr>";

      content += "</table>";
      content +=
          "<div style='margin:10px auto 0;font-size:0.9em;color:#8b949e'>The battery will not enter a "
          "charge session while it reports an insulation fault. The isolation-monitor-disable option "
          "(on by default) normally keeps that clear; otherwise the pack case must be isolated from "
          "earth.</div>";
      content += "</div>";
      content += "<hr>";
    }

    // Artificial SOC auto-calibration: settings and the live trigger criteria
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

      // Dimmed while native calibration owns the job, so the panel reads as inactive at a glance
      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white";
      content += byd_datalayer->native_termination_enabled ? ";opacity:0.45'>" : "'>";
      content += "<h4 style='margin:0 0 8px 0;color:white'>Artificial SOC auto-calibration</h4>";
      content +=
          "<div style='margin:0 0 10px;font-size:0.9em;color:#8b949e'>Battery Emulator decides the pack is full "
          "and writes 100&percnt; SOC to the battery over UDS.</div>";
      content += panel_table;

      content += "<tr>";
      content += label_td;
      content += "Enabled:</td>";
      content += value_td;
      content += "<input type='checkbox' style='margin:0;vertical-align:middle' id='autoCalEnabled" + s + "' ";
      content += (byd_datalayer->auto_calibrate_soc_enabled ? "checked" : "");
      content += " onchange='toggleAutoCalSOCEnabled" + s + "()'>";
      content += "<span style='font-weight:normal;color:#8b949e'> default on, UDS write</span>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Trigger drift:</td>";
      content += value_td;
      content +=
          "<input type='number' style='width:3.5em;margin:0;padding:1px 4px;vertical-align:middle' "
          "id='driftPercent" +
          s + "' value='";
      content += String(byd_datalayer->auto_calibrate_soc_drift_percent);
      content +=
          "' min='1' max='20'> &percnt; <button style='margin:0 0 0 8px;padding:2px 12px;vertical-align:middle' "
          "onclick='setAutoCalDriftPercent" +
          s + "()'>Save</button>";
      content += "</td></tr>";

      content += "<tr><td colspan='2' style='height:10px'></td></tr>";  // settings above, live criteria below

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
      // Outside the dimmed container, so the reason the panel is dimmed stays readable
      if (byd_datalayer->native_termination_enabled) {
        content +=
            "<div style='max-width:560px;margin:-6px auto 16px;text-align:center;font-size:0.9em;color:#d8dee4'>"
            "Overridden while native SOC calibration is enabled &mdash; the battery does it itself.</div>";
      }
    }

    // Values the manual Calibrate SOC command writes to the battery
    {
      const char* value_span = "<span style='display:inline-block;width:70px'>";
      const char* edit_button = "<button style='margin:0;padding:2px 12px;vertical-align:middle' onclick='";

      content += "<hr>";
      content += "<div style='max-width:560px;margin:16px auto;text-align:center;color:white'>";
      content += "<h4 style='margin:0 0 8px 0;color:white'>Manual SOC &amp; capacity calibration</h4>";
      content +=
          "<div style='margin:0 0 10px;font-size:0.9em;color:#8b949e'>Values used by the Calibrate SOC button "
          "below. Automatic calibration overwrites them when it runs.</div>";
      content += panel_table;

      content += "<tr>";
      content += label_td;
      content += "Target SOC:</td>";
      content += value_td;
      content += value_span;  // fixed width so both Edit buttons line up
      content += String(byd_datalayer->calibrationTargetSOC) + "&percnt;</span>";
      content += edit_button;
      content += "editCalTargetSOC" + s + "()'>Edit</button>";
      content += "</td></tr>";

      content += "<tr>";
      content += label_td;
      content += "Target capacity:</td>";
      content += value_td;
      content += value_span;
      content += String(byd_datalayer->calibrationTargetAH) + " AH</span>";
      content += edit_button;
      content += "editCalTargetAH" + s + "()'>Edit</button>";
      content += "</td></tr>";

      content += "</table>";
      content += "</div>";
      content += "<hr>";
    }

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
        content += "<button onclick='bydIsoEnable()'>Enable monitoring</button> ";
        content += "<button onclick='bydIsoDisable()'>Disable monitoring</button>";
        content += "</div>";
      }
      content += "</div>";
    }

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
    if (s.length() == 0) {  // native termination is primary-battery only
      content += "function toggleNativeTermination(){";
      content += "  var enabled = document.getElementById('nativeTerm').checked ? 1 : 0;";
      content += "  var xhr=new XMLHttpRequest();";
      content += "  xhr.onload=editComplete;";
      content += "  xhr.onerror=editError;";
      content += "  xhr.open('GET','/editBydAtto3NativeTermination?value='+enabled,true);";
      content += "  xhr.send();";
      content += "}";
      content += "function toggleBalancingEnabled(){";
      content += "  var enabled = document.getElementById('balancingEnabled').checked ? 1 : 0;";
      content += "  var xhr=new XMLHttpRequest();";
      content += "  xhr.onload=editComplete;";
      content += "  xhr.onerror=editError;";
      content += "  xhr.open('GET','/editBydAtto3BalancingEnabled?value='+enabled,true);";
      content += "  xhr.send();";
      content += "}";
      content += "function setBalancingMinutes(){";
      content += "  var v = document.getElementById('balancingMinutes').value;";
      content += "  var xhr=new XMLHttpRequest();";
      content += "  xhr.onload=editComplete;";
      content += "  xhr.onerror=editError;";
      content += "  xhr.open('GET','/editBydAtto3BalancingMinutes?value='+v,true);";
      content += "  xhr.send();";
      content += "}";
    }
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

    append_balance_time_html(content);

    auto& dtc = s.length() ? datalayer.battery2.dtc : datalayer.battery.dtc;
    content += BatteryHtmlRenderer::render_dtc_section_html(dtc, "byd_atto3_dtc.json", true);

    return content;
  }

 private:
  void append_balance_time_html(String& out) const {
    out +=
        "<h4 style='margin-top:18px'><button onclick=\"window.location.href='/bydbalance'\">"
        "&#9889; Cell Balance Timers</button></h4>";
  }

  DATALAYER_INFO_BYDATTO3* byd_datalayer;
  String s;
};

#endif
