#ifndef _RENAULT_ZOE_GEN2_HTML_H
#define _RENAULT_ZOE_GEN2_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen2HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>soc: " + String(datalayer_extended.zoePH2.battery_soc) + "</h4>";
    content += "<h4>usable soc: " + String(datalayer_extended.zoePH2.battery_usable_soc) + "</h4>";
    content += "<h4>soh: " + String(datalayer_extended.zoePH2.battery_soh) + "</h4>";
    content += "<h4>pack voltage: " + String(datalayer_extended.zoePH2.battery_pack_voltage) + "</h4>";
    content += "<h4>max cell voltage: " + String(datalayer_extended.zoePH2.battery_max_cell_voltage) + "</h4>";
    content += "<h4>min cell voltage: " + String(datalayer_extended.zoePH2.battery_min_cell_voltage) + "</h4>";
    content += "<h4>12v: " + String(datalayer_extended.zoePH2.battery_12v) + "</h4>";
    content += "<h4>avg temp: " + String(datalayer_extended.zoePH2.battery_avg_temp) + "</h4>";
    content += "<h4>min temp: " + String(datalayer_extended.zoePH2.battery_min_temp) + "</h4>";
    content += "<h4>max temp: " + String(datalayer_extended.zoePH2.battery_max_temp) + "</h4>";
    content += "<h4>max power: " + String(datalayer_extended.zoePH2.battery_max_power) + "</h4>";
    content += "<h4>interlock: " + String(datalayer_extended.zoePH2.battery_interlock) + "</h4>";
    content += "<h4>kwh: " + String(datalayer_extended.zoePH2.battery_kwh) + "</h4>";
    content += "<h4>current: " + String(datalayer_extended.zoePH2.battery_current) + "</h4>";
    content += "<h4>current offset: " + String(datalayer_extended.zoePH2.battery_current_offset) + "</h4>";
    content += "<h4>max generated: " + String(datalayer_extended.zoePH2.battery_max_generated) + "</h4>";
    content += "<h4>max available: " + String(datalayer_extended.zoePH2.battery_max_available) + "</h4>";
    content += "<h4>current voltage: " + String(datalayer_extended.zoePH2.battery_current_voltage) + "</h4>";
    content += "<h4>charging status: " + String(datalayer_extended.zoePH2.battery_charging_status) + "</h4>";
    content += "<h4>remaining charge: " + String(datalayer_extended.zoePH2.battery_remaining_charge) + "</h4>";
    content +=
        "<h4>balance capacity total: " + String(datalayer_extended.zoePH2.battery_balance_capacity_total) + "</h4>";
    content += "<h4>balance time total: " + String(datalayer_extended.zoePH2.battery_balance_time_total) + "</h4>";
    content +=
        "<h4>balance capacity sleep: " + String(datalayer_extended.zoePH2.battery_balance_capacity_sleep) + "</h4>";
    content += "<h4>balance time sleep: " + String(datalayer_extended.zoePH2.battery_balance_time_sleep) + "</h4>";
    content +=
        "<h4>balance capacity wake: " + String(datalayer_extended.zoePH2.battery_balance_capacity_wake) + "</h4>";
    content += "<h4>balance time wake: " + String(datalayer_extended.zoePH2.battery_balance_time_wake) + "</h4>";
    content += "<h4>bms state: " + String(datalayer_extended.zoePH2.battery_bms_state) + "</h4>";
    content += "<h4>energy complete: " + String(datalayer_extended.zoePH2.battery_energy_complete) + "</h4>";
    content += "<h4>energy partial: " + String(datalayer_extended.zoePH2.battery_energy_partial) + "</h4>";
    content += "<h4>slave failures: " + String(datalayer_extended.zoePH2.battery_slave_failures) + "</h4>";
    content += "<h4>mileage: " + String(datalayer_extended.zoePH2.battery_mileage) + "</h4>";
    content += "<h4>fan speed: " + String(datalayer_extended.zoePH2.battery_fan_speed) + "</h4>";
    content += "<h4>fan period: " + String(datalayer_extended.zoePH2.battery_fan_period) + "</h4>";
    content += "<h4>fan control: " + String(datalayer_extended.zoePH2.battery_fan_control) + "</h4>";
    content += "<h4>fan duty: " + String(datalayer_extended.zoePH2.battery_fan_duty) + "</h4>";
    content += "<h4>temporisation: " + String(datalayer_extended.zoePH2.battery_temporisation) + "</h4>";
    content += "<h4>time: " + String(datalayer_extended.zoePH2.battery_time) + "</h4>";
    content += "<h4>pack time: " + String(datalayer_extended.zoePH2.battery_pack_time) + "</h4>";
    content += "<h4>soc min: " + String(datalayer_extended.zoePH2.battery_soc_min) + "</h4>";
    content += "<h4>soc max: " + String(datalayer_extended.zoePH2.battery_soc_max) + "</h4>";

    return content;
  }
};

#endif
