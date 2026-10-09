#ifndef _JK_ACTIVE_BALANCER_HTML_H
#define _JK_ACTIVE_BALANCER_HTML_H

#include <cstring>

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

/* Progress / result of the CAB500 CAN speed reconfiguration, published by the driver in
   ct_cab500_speed_state. Shared by the driver state machine and the status page. */
enum JKCab500SpeedState : uint8_t {
  JK_CAB500_SPD_IDLE = 0,
  JK_CAB500_SPD_DETECT = 1,        // listening for the 0x3C2 stream at 500 / 250 / 125 kbps
  JK_CAB500_SPD_WRITE = 2,         // WriteDataByIdentifier 0xF012 sent, waiting for the answer
  JK_CAB500_SPD_PAUSE = 3,         // datasheet: wait >= 1 s after the write
  JK_CAB500_SPD_RESET = 4,         // ECUReset sent
  JK_CAB500_SPD_REBOOT = 5,        // bus switched to the new speed, sensor rebooting
  JK_CAB500_SPD_VERIFY = 6,        // ReadDataByIdentifier 0xF012 sent at the new speed
  JK_CAB500_SPD_DONE = 7,          // confirmed
  JK_CAB500_SPD_NOT_FOUND = 8,     // no 0x3C2 stream at any speed
  JK_CAB500_SPD_WRITE_FAILED = 9,  // negative response or no answer to the write
  JK_CAB500_SPD_UNVERIFIED = 10,   // write accepted but nothing seen at the new speed
  JK_CAB500_SPD_REFUSED = 11,      // contactors closed
};

class JkActiveBalancerHtmlRenderer : public BatteryHtmlRenderer {
 public:
  JkActiveBalancerHtmlRenderer(DATALAYER_BATTERY_TYPE* battery_dl, DATALAYER_INFO_JK_ACTIVE_BALANCER* dl)
      : battery_dl(battery_dl), jk_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  // The CAB500 speed buttons get a heading of their own above the first one
  String get_command_prefix_html(const char* identifier) {
    String content;
    if (jk_dl && strcmp(identifier, "resetCycles") == 0) {
      content += "<h4>BMS cycles: " + String(jk_dl->BMS_Cycles) + "</h4>";
    }
    if (jk_dl && strcmp(identifier, "cab500Speed125") == 0) {
      content += "<h4>CAB500 CAN speed (new sensors ship at 500 kbps, this driver needs 250 kbps). ";
      content += "Open the contactors first; the battery bus is switched for about 10 s.</h4>";
    }
    return content;
  }

  String get_status_html() {
    CheckedHtml content;
    if (!jk_dl) {
      return content.take();
    }
    content.reserve(1200 + jk_dl->number_of_balancers * 700);

    if (jk_dl->config_error != 0) {
      static const char* cfg_err[] = {
          "",
          "Bridge mode and low-voltage mode cannot both be enabled",
          "Low-voltage mode: number of cells must be a multiple of cells per balancer",
          "Too many balancers needed for this cell count (max 15)",
          "Number of cells or cells per balancer out of range",
          "Firmware version must be 1, 2 or 3",
          "Voltage limits out of order (need min cell < empty on < empty off < full off < full on < max cell, "
          "and pack min < pack max)",
          "SOC window invalid (SOC min must be below SOC max)"};
      const char* txt = (jk_dl->config_error < 8) ? cfg_err[jk_dl->config_error] : "Unknown";
      content += "<div style='background-color: #7A1F1F; padding: 10px; margin-bottom: 10px; border-radius: 20px;'>";
      content += "<h4>BATTERY NOT RUNNING - JK settings are inconsistent: ";
      content += txt;
      content += ". Correct the JK settings on the Settings page and reboot.</h4></div>";
    }

    for (int i = 0; i < jk_dl->number_of_balancers && i < MAX_BALANCERS_WEB; i++) {
      content += "<div style='background-color: #404E57; padding: 10px; margin-bottom: 10px; border-radius: 20px;'>";
      content += "<h4>Balancer #" + String(i + 1) + "</h4>";
      content += "<h4>Status: ";
      content += jk_dl->is_connected[i] ? "<span style='color:lightgreen;'>Connected</span>"
                                        : "<span style='color:red;'>Disconnected</span>";
      content += "</h4><h4>Pack voltage: " + String(jk_dl->balancer_total_voltage_10mV[i] / 100.0, 2) + " V</h4>";
      content += "<h4>Cells: " + String(jk_dl->balancer_identified_cells[i]) + " identified / " +
                 String(jk_dl->balancer_set_cells[i]) + " set in the balancer</h4>";
      content += "<h4>Cell delta: " + String(jk_dl->balancer_max_delta_mV[i]) + " mV</h4>";
      content += "<h4>Temperature: " + String(jk_dl->balancer_temperatures_dC[i] / 10.0, 1) + " &deg;C</h4>";
      content += "<h4>Balancing: ";
      if (jk_dl->charge_balancing_status[i]) {
        content += "Charge Balancing";
      } else if (jk_dl->discharge_balancing_status[i]) {
        content += "Discharge Balancing";
      } else {
        content += "Idle";
      }
      content += "</h4><h4>Balancing current: " + String(jk_dl->balance_current_mA[i]) + " mA</h4>";
      content += "<h4>Balance Switch: ";
      content += jk_dl->balance_switch_status[i] ? "ON" : "OFF";
      content += "</h4><h4>Max Balance Current: " + String(jk_dl->max_balance_current_mA[i]) + " mA</h4>";
      content += "<h4>Communication Faults: " + String(jk_dl->fault_counters[i]) + "</h4>";
      content += "<h4>Cell Count Mismatch: ";
      content += jk_dl->cell_count_mismatch_alarm[i]
                     ? "<span style='color:red;'>ALARM (latched, reboot to clear)</span>"
                     : "<span style='color:lightgreen;'>OK</span>";
      content += "</h4><h4>Wire Resistance: ";
      content += jk_dl->wire_resistance_alarm[i] ? "<span style='color:red;'>ALARM (latched, reboot to clear)</span>"
                                                 : "<span style='color:lightgreen;'>OK</span>";
      content += "</h4></div>";
    }

    // --- CAB500 CAN current sensor ---
    content += "<div style='background-color: #404E57; padding: 10px; margin-bottom: 10px; border-radius: 20px;'>";
    content += "<h4>CAB500 CT Sensor (CAN 0x" + String(jk_dl->ct_cab500_can_id, HEX) + ")</h4><h4>Status: ";
    content += jk_dl->ct_cab500_is_connected ? "<span style='color:lightgreen;'>Connected</span>"
                                             : "<span style='color:red;'>Disconnected</span>";
    content += "</h4><h4>Communication: ";
    content += jk_dl->ct_cab500_comms_fault ? "<span style='color:red;'>FAULT (latched, reboot to clear)</span>"
                                            : "<span style='color:lightgreen;'>OK</span>";
    content += "</h4><h4>Communication Faults: " + String(jk_dl->ct_cab500_fault_counter) + "</h4>";
    content += "<h4>Hardware: ";
    content += jk_dl->ct_cab500_hardware_error ? "<span style='color:red;'>ERROR (latched)</span>"
                                               : "<span style='color:lightgreen;'>OK</span>";
    content += "</h4>";
    if (jk_dl->ct_cab500_hardware_error) {
      const char* dtc_text;
      switch (jk_dl->ct_cab500_error_code) {
        case 0x41:
          dtc_text = "Overcurrent Detection (Ip > ~520A)";
          break;
        case 0x42:
          dtc_text = "Closed-loop reference voltage over range";
          break;
        case 0x44:
          dtc_text = "Signal not available for more than 100ms";
          break;
        case 0x46:
          dtc_text = "Supply voltage out of range";
          break;
        default:
          dtc_text = "Unknown code";
          break;
      }
      content += "<h4>DTC: <span style='color:red;'>0x" + String(jk_dl->ct_cab500_error_code, HEX) + " - ";
      content += dtc_text;
      content += "</span></h4>";
    }
    content += "<h4>CAN speed change: ";
    content += cab500_speed_text();
    content += "</h4><h4>Capacity used for SOC: " + String(jk_dl->calc_capacity_mAh / 1000.0, 2) + " Ah (rated " +
               String(jk_dl->rated_capacity_mAh / 1000.0, 2) + " Ah from the Wh setting)</h4></div>";

    // --- Coulomb counter ---
    content += "<h4>BMS Cycles: " + String(jk_dl->BMS_Cycles) + "</h4>";
    content += "<h4>Calculated capacity: " + String(jk_dl->Calc_capacity_Wh) + " Wh</h4>";

    // Limits the driver derived from the settings at boot
    content += "<p style='color:#ccc'>In use since last boot: pack max " +
               String(battery_dl->info.max_design_voltage_dV / 10.0, 1) + " V, pack min " +
               String(battery_dl->info.min_design_voltage_dV / 10.0, 1) + " V, cell max " +
               String(battery_dl->info.max_cell_voltage_mV) + " mV, cell min " +
               String(battery_dl->info.min_cell_voltage_mV) + " mV, max deviation " +
               String(battery_dl->info.max_cell_voltage_deviation_mV) + " mV.</p>";

    return content.take();
  }

 private:
  DATALAYER_BATTERY_TYPE* battery_dl;
  DATALAYER_INFO_JK_ACTIVE_BALANCER* jk_dl;

  String cab500_speed_text() {
    String target = String(jk_dl->ct_cab500_speed_target_kbps) + " kbps";
    String found = String(jk_dl->ct_cab500_speed_found_kbps) + " kbps";
    switch (jk_dl->ct_cab500_speed_state) {
      case JK_CAB500_SPD_IDLE:
        return "not run";
      case JK_CAB500_SPD_DETECT:
        return "searching the sensor at 500 / 250 / 125 kbps, refresh the page";
      case JK_CAB500_SPD_WRITE:
      case JK_CAB500_SPD_PAUSE:
      case JK_CAB500_SPD_RESET:
      case JK_CAB500_SPD_REBOOT:
      case JK_CAB500_SPD_VERIFY:
        return "sensor found at " + found + ", setting " + target + ", refresh the page";
      case JK_CAB500_SPD_DONE:
        return "<span style='color:lightgreen;'>done, sensor now at " + target + " (was at " + found + ")</span>";
      case JK_CAB500_SPD_NOT_FOUND:
        return "<span style='color:red;'>failed, no CAB500 stream found at 500, 250 or 125 kbps</span>";
      case JK_CAB500_SPD_WRITE_FAILED:
        return "<span style='color:red;'>failed, sensor found at " + found + " but it did not accept the write</span>";
      case JK_CAB500_SPD_UNVERIFIED:
        return "<span style='color:orange;'>write accepted, but nothing seen at " + target +
               " yet. Power-cycle the sensor and check the status above</span>";
      case JK_CAB500_SPD_REFUSED:
        return "<span style='color:red;'>refused, open the contactors first</span>";
      default:
        return "unknown";
    }
  }
};

#endif
