#ifndef _BMW_IX_HTML_H
#define _BMW_IX_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BmwIXHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content +=
        "<h4>Battery Voltage after Contactor: " + String(datalayer_extended.bmwix.battery_voltage_after_contactor) +
        " dV</h4>";
    content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
    content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
    content +=
        "<h4>Min Cell Voltage Data Age: " + String(datalayer_extended.bmwix.min_cell_voltage_data_age) + " ms</h4>";
    content +=
        "<h4>Max Cell Voltage Data Age: " + String(datalayer_extended.bmwix.max_cell_voltage_data_age) + " ms</h4>";
    content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
    content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
    content += "<h4>T30 Terminal Voltage: " + String(datalayer_extended.bmwix.T30_Voltage) + " mV</h4>";
    content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content += "<h4>Balancing: ";
    switch (datalayer_extended.bmwix.balancing_status) {
      case 0:
        content += "0 No balancing mode active</h4>";
        break;
      case 1:
        content += "1 Voltage-Controlled Balancing Mode</h4>";
        break;
      case 2:
        content += "2 Time-Controlled Balancing Mode with Demand Calculation at End of Charging</h4>";
        break;
      case 3:
        content += "3 Time-Controlled Balancing Mode with Demand Calculation at Resting Voltage</h4>";
        break;
      case 4:
        content += "4 No balancing mode active, qualifier invalid</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>HVIL Status: ";
    switch (datalayer_extended.bmwix.hvil_status) {
      case 0:
        content += "Error (Loop Open)</h4>";
        break;
      case 1:
        content += "OK (Loop Closed)</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>BMS Uptime: " + String(datalayer_extended.bmwix.bms_uptime) + " seconds</h4>";
    content += "<h4>BMS Allowed Charge Amps: " + String(datalayer_extended.bmwix.allowable_charge_amps) + " A</h4>";
    content +=
        "<h4>BMS Allowed Disharge Amps: " + String(datalayer_extended.bmwix.allowable_discharge_amps) + " A</h4>";
    content += "<br>";
    content += "<h3>HV Isolation (2147483647kOhm = maximum/invalid)</h3>";
    content += "<h4>Isolation Positive: " + String(datalayer_extended.bmwix.iso_safety_positive) + " kOhm</h4>";
    content += "<h4>Isolation Negative: " + String(datalayer_extended.bmwix.iso_safety_negative) + " kOhm</h4>";
    content += "<h4>Isolation Parallel: " + String(datalayer_extended.bmwix.iso_safety_parallel) + " kOhm</h4>";
    content += "<h4>Pyro Status PSS1: ";
    switch (datalayer_extended.bmwix.pyro_status_pss1) {
      case 0:
        content += "0 Value Invalid</h4>";
        break;
      case 1:
        content += "1 Successfully Blown</h4>";
        break;
      case 2:
        content += "2 Disconnected</h4>";
        break;
      case 3:
        content += "3 Not Activated - Pyro Intact</h4>";
        break;
      case 4:
        content += "4 Unknown</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>Pyro Status PSS4: ";
    switch (datalayer_extended.bmwix.pyro_status_pss4) {
      case 0:
        content += "0 Value Invalid</h4>";
        break;
      case 1:
        content += "1 Successfully Blown</h4>";
        break;
      case 2:
        content += "2 Disconnected</h4>";
        break;
      case 3:
        content += "3 Not Activated - Pyro Intact</h4>";
        break;
      case 4:
        content += "4 Unknown</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }
    content += "<h4>Pyro Status PSS6: ";
    switch (datalayer_extended.bmwix.pyro_status_pss6) {
      case 0:
        content += "0 Value Invalid</h4>";
        break;
      case 1:
        content += "1 Successfully Blown</h4>";
        break;
      case 2:
        content += "2 Disconnected</h4>";
        break;
      case 3:
        content += "3 Not Activated - Pyro Intact</h4>";
        break;
      case 4:
        content += "4 Unknown</h4>";
        break;
      default:
        content += "Unknown</h4>";
    }

    return content;
  }
};

#endif
