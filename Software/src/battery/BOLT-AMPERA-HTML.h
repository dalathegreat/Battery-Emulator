#ifndef _BOLT_AMPERA_HTML_H
#define _BOLT_AMPERA_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BoltAmperaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>5V Reference: " + String(datalayer_extended.boltampera.battery_5V_ref) + "</h4>";
    content += "<h4>Module 1 temp: " + String(datalayer_extended.boltampera.battery_module_temp_1) + "</h4>";
    content += "<h4>Module 2 temp: " + String(datalayer_extended.boltampera.battery_module_temp_2) + "</h4>";
    content += "<h4>Module 3 temp: " + String(datalayer_extended.boltampera.battery_module_temp_3) + "</h4>";
    content += "<h4>Module 4 temp: " + String(datalayer_extended.boltampera.battery_module_temp_4) + "</h4>";
    content += "<h4>Module 5 temp: " + String(datalayer_extended.boltampera.battery_module_temp_5) + "</h4>";
    content += "<h4>Module 6 temp: " + String(datalayer_extended.boltampera.battery_module_temp_6) + "</h4>";
    content +=
        "<h4>Cell average voltage: " + String(datalayer_extended.boltampera.battery_cell_average_voltage) + "</h4>";
    content +=
        "<h4>Cell average voltage 2: " + String(datalayer_extended.boltampera.battery_cell_average_voltage_2) + "</h4>";
    content += "<h4>Terminal voltage: " + String(datalayer_extended.boltampera.battery_terminal_voltage) + "</h4>";
    content +=
        "<h4>Ignition power mode: " + String(datalayer_extended.boltampera.battery_ignition_power_mode) + "</h4>";
    content += "<h4>Battery current (7E7): " + String(datalayer_extended.boltampera.battery_current_7E7) + "</h4>";
    content += "<h4>Capacity MY17-18: " + String(datalayer_extended.boltampera.battery_capacity_my17_18) + "</h4>";
    content += "<h4>Capacity MY19+: " + String(datalayer_extended.boltampera.battery_capacity_my19plus) + "</h4>";
    content += "<h4>SOC Display: " + String(datalayer_extended.boltampera.battery_SOC_display) + "</h4>";
    content += "<h4>SOC Raw highprec: " + String(datalayer_extended.boltampera.battery_SOC_raw_highprec) + "</h4>";
    content += "<h4>Max temp: " + String(datalayer_extended.boltampera.battery_max_temperature) + "</h4>";
    content += "<h4>Min temp: " + String(datalayer_extended.boltampera.battery_min_temperature) + "</h4>";
    content += "<h4>Cell max mV: " + String(datalayer_extended.boltampera.battery_max_cell_voltage) + "</h4>";
    content += "<h4>Cell min mV: " + String(datalayer_extended.boltampera.battery_min_cell_voltage) + "</h4>";
    content += "<h4>Lowest cell: " + String(datalayer_extended.boltampera.battery_lowest_cell) + "</h4>";
    content += "<h4>Highest cell: " + String(datalayer_extended.boltampera.battery_highest_cell) + "</h4>";
    content +=
        "<h4>Internal resistance: " + String(datalayer_extended.boltampera.battery_internal_resistance) + "</h4>";
    content += "<h4>Voltage: " + String(datalayer_extended.boltampera.battery_voltage_polled) + "</h4>";
    content += "<h4>Isolation Ohm: " + String(datalayer_extended.boltampera.battery_vehicle_isolation) + "</h4>";
    content += "<h4>Isolation kOhm: " + String(datalayer_extended.boltampera.battery_isolation_kohm) + "</h4>";
    content += "<h4>HV locked: " + String(datalayer_extended.boltampera.battery_HV_locked) + "</h4>";
    content += "<h4>Crash event: " + String(datalayer_extended.boltampera.battery_crash_event) + "</h4>";
    content += "<h4>HVIL: " + String(datalayer_extended.boltampera.battery_HVIL) + "</h4>";
    content += "<h4>HVIL status: " + String(datalayer_extended.boltampera.battery_HVIL_status) + "</h4>";
    content += "<h4>Current (7E4): " + String(datalayer_extended.boltampera.battery_current_7E4) + "</h4>";

    return content;
  }
};

#endif
