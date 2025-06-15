#ifndef _MEB_HTML_H
#define _MEB_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class MebHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += datalayer_extended.meb.SDSW ? "<h4>Service disconnect switch: Missing!</h4>"
                                           : "<h4>Service disconnect switch: OK</h4>";
    content += datalayer_extended.meb.pilotline ? "<h4>Pilotline: Open!</h4>" : "<h4>Pilotline: OK</h4>";
    content += datalayer_extended.meb.transportmode ? "<h4>Transportmode: Locked!</h4>" : "<h4>Transportmode: OK</h4>";
    content += datalayer_extended.meb.shutdown_active ? "<h4>Shutdown: Active!</h4>" : "<h4>Shutdown: No</h4>";
    content += datalayer_extended.meb.componentprotection ? "<h4>Component protection: Active!</h4>"
                                                          : "<h4>Component protection: No</h4>";
    content += "<h4>HVIL status: ";
    switch (datalayer_extended.meb.HVIL) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "Closed";
        break;
      case 2:
        content += "Open!";
        break;
      case 3:
        content += "Fault";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>KL30C status: ";
    switch (datalayer_extended.meb.BMS_Kl30c_Status) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "Closed";
        break;
      case 2:
        content += "Open!";
        break;
      case 3:
        content += "Fault";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>BMS mode: ";
    switch (datalayer_extended.meb.BMS_mode) {
      case 0:
        content += "HV inactive";
        break;
      case 1:
        content += "HV active";
        break;
      case 2:
        content += "Balancing";
        break;
      case 3:
        content += "Extern charging";
        break;
      case 4:
        content += "AC charging";
        break;
      case 5:
        content += "Battery error";
        break;
      case 6:
        content += "DC charging";
        break;
      case 7:
        content += "Init";
        break;
      default:
        content += "?";
    }
    content += String("</h4><h4>Charging: ") + (datalayer_extended.meb.charging_active ? "active" : "not active");
    content += String("</h4><h4>Balancing: ");
    switch (datalayer_extended.meb.balancing_active) {
      case 0:
        content += "init";
        break;
      case 1:
        content += "active";
        break;
      case 2:
        content += "inactive";
        break;
      default:
        content += "?";
    }
    content +=
        String("</h4><h4>Slow charging: ") + (datalayer_extended.meb.balancing_request ? "requested" : "not requested");
    content += "</h4><h4>Diagnostic: ";
    switch (datalayer_extended.meb.battery_diagnostic) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "Battery display";
        break;
      case 4:
        content += "Battery display OK";
        break;
      case 6:
        content += "Battery display check";
        break;
      case 7:
        content += "Fault";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>HV line status: ";
    switch (datalayer_extended.meb.status_HV_line) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "No open HV line detected";
        break;
      case 2:
        content += "Open HV line";
        break;
      case 3:
        content += "Fault";
        break;
      default:
        content += "? " + String(datalayer_extended.meb.status_HV_line);
    }
    content += "</h4>";
    content += datalayer_extended.meb.BMS_fault_performance ? "<h4>BMS fault performance: Active!</h4>"
                                                            : "<h4>BMS fault performance: Off</h4>";
    content += datalayer_extended.meb.BMS_fault_emergency_shutdown_crash
                   ? "<h4>BMS fault emergency shutdown crash: Active!</h4>"
                   : "<h4>BMS fault emergency shutdown crash: Off</h4>";
    content += datalayer_extended.meb.BMS_error_shutdown_request ? "<h4>BMS error shutdown request: Active!</h4>"
                                                                 : "<h4>BMS error shutdown request: Inactive</h4>";
    content += datalayer_extended.meb.BMS_error_shutdown ? "<h4>BMS error shutdown: Active!</h4>"
                                                         : "<h4>BMS error shutdown: Off</h4>";
    content += "<h4>Welded contactors: ";
    switch (datalayer_extended.meb.BMS_welded_contactors_status) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "No contactor welded";
        break;
      case 2:
        content += "At least 1 contactor welded";
        break;
      case 3:
        content += "Protection status detection error";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>Warning support: ";
    switch (datalayer_extended.meb.warning_support) {
      case 0:
        content += "OK";
        break;
      case 1:
        content += "Not OK";
        break;
      case 6:
        content += "Init";
        break;
      case 7:
        content += "Fault";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>Interm. Voltage (" + String(datalayer_extended.meb.BMS_voltage_intermediate_dV / 10.0, 1) +
               "V) status: ";
    switch (datalayer_extended.meb.BMS_status_voltage_free) {
      case 0:
        content += "Init";
        break;
      case 1:
        content += "BMS interm circuit voltage free (U<20V)";
        break;
      case 2:
        content += "BMS interm circuit not voltage free (U >= 25V)";
        break;
      case 3:
        content += "Error";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>BMS error status: ";
    switch (datalayer_extended.meb.BMS_error_status) {
      case 0:
        content += "Component IO";
        break;
      case 1:
        content += "Iso Error 1";
        break;
      case 2:
        content += "Iso Error 2";
        break;
      case 3:
        content += "Interlock";
        break;
      case 4:
        content += "SD";
        break;
      case 5:
        content += "Performance red";
        break;
      case 6:
        content += "No component function";
        break;
      case 7:
        content += "Init";
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>BMS voltage: " + String(datalayer_extended.meb.BMS_voltage_dV / 10.0, 1) + "</h4>";
    content += datalayer_extended.meb.BMS_OBD_MIL ? "<h4>OBD MIL: ON!</h4>" : "<h4>OBD MIL: Off</h4>";
    content +=
        datalayer_extended.meb.BMS_error_lamp_req ? "<h4>Red error lamp: ON!</h4>" : "<h4>Red error lamp: Off</h4>";
    content += datalayer_extended.meb.BMS_warning_lamp_req ? "<h4>Yellow warning lamp: ON!</h4>"
                                                           : "<h4>Yellow warning lamp: Off</h4>";
    content += "<h4>Isolation resistance: " + String(datalayer_extended.meb.isolation_resistance) + " kOhm</h4>";
    content +=
        datalayer_extended.meb.battery_heating ? "<h4>Battery heating: Active!</h4>" : "<h4>Battery heating: Off</h4>";
    const char* rt_enum[] = {"No", "Error level 1", "Error level 2", "Error level 3"};
    content += "<h4>Overcurrent: " + String(rt_enum[datalayer_extended.meb.rt_overcurrent & 0x03]) + "</h4>";
    content += "<h4>CAN fault: " + String(rt_enum[datalayer_extended.meb.rt_CAN_fault & 0x03]) + "</h4>";
    content += "<h4>Overcharged: " + String(rt_enum[datalayer_extended.meb.rt_overcharge & 0x03]) + "</h4>";
    content += "<h4>SOC too high: " + String(rt_enum[datalayer_extended.meb.rt_SOC_high & 0x03]) + "</h4>";
    content += "<h4>SOC too low: " + String(rt_enum[datalayer_extended.meb.rt_SOC_low & 0x03]) + "</h4>";
    content += "<h4>SOC jumping: " + String(rt_enum[datalayer_extended.meb.rt_SOC_jumping & 0x03]) + "</h4>";
    content += "<h4>Temp difference: " + String(rt_enum[datalayer_extended.meb.rt_temp_difference & 0x03]) + "</h4>";
    content += "<h4>Cell overtemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_overtemp & 0x03]) + "</h4>";
    content += "<h4>Cell undertemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_undertemp & 0x03]) + "</h4>";
    content +=
        "<h4>Battery overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_overvolt & 0x03]) + "</h4>";
    content +=
        "<h4>Battery undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_undervol & 0x03]) + "</h4>";
    content += "<h4>Cell overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_overvolt & 0x03]) + "</h4>";
    content += "<h4>Cell undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_undervol & 0x03]) + "</h4>";
    content += "<h4>Cell imbalance: " + String(rt_enum[datalayer_extended.meb.rt_cell_imbalance & 0x03]) + "</h4>";
    content +=
        "<h4>Battery unathorized: " + String(rt_enum[datalayer_extended.meb.rt_battery_unathorized & 0x03]) + "</h4>";
    content +=
        "<h4>Battery temperature: " + String(datalayer_extended.meb.battery_temperature_dC / 10.f, 1) + " &deg;C</h4>";
    for (int i = 0; i < 3; i++) {
      content += "<h4>Temperature points " + String(i * 6 + 1) + "-" + String(i * 6 + 6) + " :";
      for (int j = 0; j < 6; j++)
        content += " &nbsp;" + String(datalayer_extended.meb.temp_points[i * 6 + j], 1);
      content += " &deg;C</h4>";
    }
    bool temps_done = false;
    for (int i = 0; i < 7 && !temps_done; i++) {
      content += "<h4>Cell temperatures " + String(i * 8 + 1) + "-" + String(i * 8 + 8) + " :";
      for (int j = 0; j < 8; j++) {
        if (datalayer_extended.meb.celltemperature_dC[i * 8 + j] == 865) {
          temps_done = true;
          break;
        } else {
          content += " &nbsp;" + String(datalayer_extended.meb.celltemperature_dC[i * 8 + j] / 10.f, 1);
        }
      }
      content += " &deg;C</h4>";
    }
    content +=
        "<h4>Total charged: " + String(datalayer.battery.status.total_charged_battery_Wh / 1000.0, 1) + " kWh</h4>";
    content += "<h4>Total discharged: " + String(datalayer.battery.status.total_discharged_battery_Wh / 1000.0, 1) +
               " kWh</h4>";

    return content;
  }
};

#endif
