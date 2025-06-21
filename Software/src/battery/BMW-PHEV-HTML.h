#ifndef _BMW_PHEV_HTML_H
#define _BMW_PHEV_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BmwPhevHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content +=
        "<h4>Battery Voltage after Contactor: " + String(datalayer_extended.bmwphev.battery_voltage_after_contactor) +
        " dV</h4>";
    content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
    content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
    content += "<h4>Balancing: ";
    switch (datalayer_extended.bmwphev.balancing_status) {
      case 0:
        content += String("0 Balancing Inactive - Balancing not needed</h4>");
        break;
      case 1:
        content += String("1 Balancing Active</h4>");
        break;
      case 2:
        content += String("2 Balancing Inactive - Cells not in rest break wait 10mins</h4>");
        break;
      case 3:
        content += String("3 Balancing Inactive</h4>");
        break;
      case 4:
        content += String("4 Unknown</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Interlock: ";
    switch (datalayer_extended.bmwphev.ST_interlock) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error! Not seated!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Isolation external: ";
    switch (datalayer_extended.bmwphev.ST_iso_ext) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Isolation internal: ";
    switch (datalayer_extended.bmwphev.ST_iso_int) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Isolation: ";
    switch (datalayer_extended.bmwphev.ST_isolation) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Cooling valve: ";
    switch (datalayer_extended.bmwphev.ST_valve_cooling) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Emergency: ";
    switch (datalayer_extended.bmwphev.ST_EMG) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("OK</h4>");
        break;
      case 2:
        content += String("Error!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Precharge: ";
    switch (datalayer_extended.bmwphev.ST_precharge) {
      case 0:
        content += String("Not Evaluated</h4>");
        break;
      case 1:
        content += String("Not active, closing not blocked</h4>");
        break;
      case 2:
        content += String("Error precharge blocked</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");  //Still unclear of enum
    }
    content += "<h4>Contactor status: ";
    switch (datalayer_extended.bmwphev.ST_DCSW) {
      case 0:
        content += String("Contactors open</h4>");
        break;
      case 1:
        content += String("Precharge ongoing</h4>");
        break;
      case 2:
        content += String("Contactors engaged</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Contactor weld: ";
    switch (datalayer_extended.bmwphev.ST_WELD) {
      case 0:
        content += String("Contactors OK</h4>");
        break;
      case 1:
        content += String("One contactor welded!</h4>");
        break;
      case 2:
        content += String("Two contactors welded!</h4>");
        break;
      case 3:
        content += String("Invalid signal</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    content += "<h4>Cold shutoff valve: ";
    switch (datalayer_extended.bmwphev.ST_cold_shutoff_valve) {
      case 0:
        content += String("OK</h4>");
        break;
      case 1:
        content += String("Short circuit to GND</h4>");
        break;
      case 2:
        content += String("Short circuit to 12V</h4>");
        break;
      case 3:
        content += String("Line break</h4>");
        break;
      case 6:
        content += String("Driver error</h4>");
        break;
      case 12:
      case 13:
        content += String("Stuck</h4>");
        break;
      default:
        content += String("Invalid Signal</h4>");
    }
    content +=
        "<h4>Min Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.min_cell_voltage_data_age) + " ms</h4>";
    content +=
        "<h4>Max Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.max_cell_voltage_data_age) + " ms</h4>";
    content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    content += "<h4>BMS Allowed Charge Amps: " + String(datalayer_extended.bmwphev.allowable_charge_amps) + " A</h4>";
    content +=
        "<h4>BMS Allowed Disharge Amps: " + String(datalayer_extended.bmwphev.allowable_discharge_amps) + " A</h4>";
    content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content += "<h4>iso_safety_int_kohm: " + String(datalayer_extended.bmwphev.iso_safety_int_kohm) + "</h4>";
    content += "<h4>iso_safety_ext_kohm: " + String(datalayer_extended.bmwphev.iso_safety_ext_kohm) + "</h4>";
    content += "<h4>iso_safety_trg_kohm: " + String(datalayer_extended.bmwphev.iso_safety_trg_kohm) + "</h4>";
    content += "<h4>iso_safety_ext_plausible: " + String(datalayer_extended.bmwphev.iso_safety_ext_plausible) + "</h4>";
    content += "<h4>iso_safety_int_plausible: " + String(datalayer_extended.bmwphev.iso_safety_int_plausible) + "</h4>";
    content += "<h4>iso_safety_trg_plausible: " + String(datalayer_extended.bmwphev.iso_safety_trg_plausible) + "</h4>";
    content += "<h4>iso_safety_kohm: " + String(datalayer_extended.bmwphev.iso_safety_kohm) + "</h4>";
    content += "<h4>iso_safety_kohm_quality: " + String(datalayer_extended.bmwphev.iso_safety_kohm_quality) + "</h4>";
    content += "<br>";
    content += "<h4>Todo";
    content += "<br>";
    content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
    content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
    content += "<h4>T30 Terminal Voltage: " + String(datalayer_extended.bmwphev.T30_Voltage) + " mV</h4>";
    content += "<br>";

    return content;
  }
};

#endif
