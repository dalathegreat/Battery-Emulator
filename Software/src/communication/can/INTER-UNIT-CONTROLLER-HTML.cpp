#include "INTER-UNIT-CONTROLLER-HTML.h"

#include <Arduino.h>
#include <IPAddress.h>

#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "INTER-UNIT-PROTOCOL.h"

String InterUnitControllerHtmlRenderer::get_status_html() {
  String content;

  content += "<style>";
  content += ".iu-grid{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-top:10px;}";
  content += ".iu-card{padding:12px;border-radius:14px;min-width:180px;max-width:240px;}";
  content += ".iu-ok{color:#00E000;}";
  content += ".iu-err{color:#FF4444;}";
  content +=
      ".iu-ota{display:inline-block;margin-top:8px;padding:10px 20px;border-radius:10px;"
      "background:#505E67;color:white;text-decoration:none;cursor:pointer;}";
  content += ".iu-ota:hover{background:#3A4A52;}";
  content += "</style>";

  content += "<h4 style='margin:0.4em 0;'>Inter-Unit Battery Nodes - Extended Info</h4>";

  // Find the reference battery_type_id (first online node that has sent IDENT)
  uint16_t ref_btype = 0;
  bool ref_btype_found = false;
  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[i];
    if (n.online && n.ident_received) {
      ref_btype = n.battery_type_id;
      ref_btype_found = true;
      break;
    }
  }

  bool any_online = false;
  content += "<div class='iu-grid'>";

  for (uint8_t i = 0; i < MAX_BATTERY_NODES; i++) {
    const BATTERY_NODE_TYPE& n = datalayer.system.battery_nodes[i];
    if (!n.online) {
      continue;
    }
    any_online = true;

    // FW / battery-type identity check. A mismatch blocks the contactor on the
    // controller side but does not set a fault_flag bit, so include it here so the
    // whole card turns red (not just the per-line ✗). Only meaningful once IDENT
    // has been received — an un-identified node shows "waiting", not an error.
    bool fw_ok = true;
    bool btype_ok = true;
    if (n.ident_received) {
      fw_ok = (n.fw_version_num == iu_fw_version_num());
      btype_ok = !ref_btype_found || (n.battery_type_id == ref_btype);
    }
    bool identity_mismatch = !fw_ok || !btype_ok;

    // A stale node (STATUS not refreshing) is blocked on the controller side but sets no fault_flag
    // bit, so include it here too so the whole card turns red — it does not raise a global fault.
    bool is_stale = n.status_stale_seconds > IU_STATUS_STALE_SECONDS;

    bool is_error = ((n.fault_flags & IU_FAULT_ERROR_MASK) != 0) || identity_mismatch || is_stale;
    bool is_warning = !is_error && (n.fault_flags & IU_FAULT_WARNING_MASK) != 0;
    const char* bg = is_error ? "#5A0606" : (is_warning ? "#4A3A00" : "#1E3020");

    content += "<div class='iu-card' style='background:" + String(bg) + ";'>";

    // Card title — link to node IP if known
    if (n.ip_address != 0) {
      IPAddress ip(n.ip_address);
      content += "<h4 style='margin:2px 0;color:white;'><a href='http://" + ip.toString() +
                 "' target='_blank' style='color:white;'>Node " + String(i + 1) + " &#8599;</a></h4>";
    } else {
      content += "<h4 style='margin:2px 0;color:white;'>Node " + String(i + 1) + "</h4>";
    }

    // ---- Firmware version ----------------------------------------
    if (n.ident_received) {
      uint8_t fw_maj = (uint8_t)(n.fw_version_num >> 8);
      uint8_t fw_min = (uint8_t)(n.fw_version_num & 0xFFu);
      content += "<h4 style='margin:2px 0;'>FW: " + String(fw_maj) + "." + String(fw_min);
      if (fw_ok) {
        content += " <span class='iu-ok'>&#10003;</span>";
      } else {
        uint16_t exp_fw = iu_fw_version_num();
        content += " <span class='iu-err'>&#10007; (exp " + String((uint8_t)(exp_fw >> 8)) + "." +
                   String((uint8_t)(exp_fw & 0xFFu)) + ")</span>";
      }
      content += "</h4>";

      // ---- Battery protocol ----------------------------------------
      const char* btype_name = name_for_battery_type((BatteryType)n.battery_type_id);
      content += "<h4 style='margin:2px 0;'>Type: " + String(btype_name);
      if (btype_ok) {
        content += " <span class='iu-ok'>&#10003;</span>";
      } else {
        content += " <span class='iu-err'>&#10007; (ref: " + String(name_for_battery_type((BatteryType)ref_btype)) +
                   ")</span>";
      }
      content += "</h4>";
    } else {
      content += "<h4 style='margin:2px 0;color:gray;'>FW: waiting...</h4>";
      content += "<h4 style='margin:2px 0;color:gray;'>Type: waiting...</h4>";
    }

    // ---- Cell min/max voltage ------------------------------------
    if (n.cell_min_voltage_mV > 0 || n.cell_max_voltage_mV > 0) {
      content += "<h4 style='margin:2px 0;'>Cell min: " + String(n.cell_min_voltage_mV) + " mV</h4>";
      content += "<h4 style='margin:2px 0;'>Cell max: " + String(n.cell_max_voltage_mV) + " mV</h4>";
    }

    // ---- Total capacity & SOH ------------------------------------
    if (n.total_capacity_Wh > 0) {
      content += "<h4 style='margin:2px 0;'>Capacity: " + String(n.total_capacity_Wh / 1000.0f, 1) + " kWh</h4>";
    }
    if (n.soh_pptt > 0) {
      content += "<h4 style='margin:2px 0;'>SOH: " + String(n.soh_pptt / 100.0f, 1) + " %</h4>";
    }

    // ---- Fault flags detail --------------------------------------
    const uint8_t fault_bits =
        (uint8_t)(n.fault_flags & (uint8_t)(~(uint8_t)(IU_FLAG_CONTACTOR_ENGAGED | IU_FLAG_STATUS_TOGGLE)));
    if (fault_bits) {
      String faults = "";
      if (fault_bits & IU_FAULT_BMS_FAULT)
        faults += "BMS fault, ";
      if (fault_bits & IU_FAULT_CELL_OVERVOLTAGE)
        faults += "Cell OV, ";
      if (fault_bits & IU_FAULT_CELL_UNDERVOLTAGE)
        faults += "Cell UV, ";
      if (fault_bits & IU_FAULT_OVERTEMPERATURE)
        faults += "Over-temp, ";
      if (fault_bits & IU_FAULT_CONTACTOR_FAILED)
        faults += "Contactor fail, ";
      if (fault_bits & IU_FAULT_BATTERY_TIMEOUT)
        faults += "Battery timeout, ";
      if (faults.length() > 2) {
        faults.remove(faults.length() - 2);  // trim trailing ", "
      }
      content += "<h4 style='margin:2px 0;color:#FF8888;'>&#9888; " + faults + "</h4>";
    }

    // ---- OTA button ----------------------------------------------
    if (n.ip_address != 0) {
      IPAddress otaIp(n.ip_address);
      content +=
          "<a class='iu-ota' href='http://" + otaIp.toString() + "/update' target='_blank'>&#8593; OTA Update</a>";
    }

    content += "</div>";  // end card
  }

  if (!any_online) {
    content += "<h4 style='color:gray;'>No nodes online</h4>";
  }

  content += "</div>";  // end grid
  return content;
}
