#ifndef _CMFA_EV_HTML_H
#define _CMFA_EV_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class CmfaEvHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>SOC U: " + String(datalayer_extended.CMFAEV.soc_u) + "percent</h4>";
    content += "<h4>SOC Z: " + String(datalayer_extended.CMFAEV.soc_z) + "percent</h4>";
    content += "<h4>SOH Average: " + String(datalayer_extended.CMFAEV.soh_average) + "pptt</h4>";
    content += "<h4>12V voltage: " + String(datalayer_extended.CMFAEV.lead_acid_voltage) + "mV</h4>";
    content += "<h4>Highest cell number: " + String(datalayer_extended.CMFAEV.highest_cell_voltage_number) + "</h4>";
    content += "<h4>Lowest cell number: " + String(datalayer_extended.CMFAEV.lowest_cell_voltage_number) + "</h4>";
    content += "<h4>Max regen power: " + String(datalayer_extended.CMFAEV.max_regen_power) + "</h4>";
    content += "<h4>Max discharge power: " + String(datalayer_extended.CMFAEV.max_discharge_power) + "</h4>";
    content += "<h4>Max charge power: " + String(datalayer_extended.CMFAEV.maximum_charge_power) + "</h4>";
    content += "<h4>SOH available power: " + String(datalayer_extended.CMFAEV.SOH_available_power) + "</h4>";
    content += "<h4>SOH generated power: " + String(datalayer_extended.CMFAEV.SOH_generated_power) + "</h4>";
    content += "<h4>Average temperature: " + String(datalayer_extended.CMFAEV.average_temperature) + "dC</h4>";
    content += "<h4>Maximum temperature: " + String(datalayer_extended.CMFAEV.maximum_temperature) + "dC</h4>";
    content += "<h4>Minimum temperature: " + String(datalayer_extended.CMFAEV.minimum_temperature) + "dC</h4>";
    content +=
        "<h4>Cumulative energy discharged: " + String(datalayer_extended.CMFAEV.cumulative_energy_when_discharging) +
        "Wh</h4>";
    content += "<h4>Cumulative energy charged: " + String(datalayer_extended.CMFAEV.cumulative_energy_when_charging) +
               "Wh</h4>";
    content +=
        "<h4>Cumulative energy regen: " + String(datalayer_extended.CMFAEV.cumulative_energy_in_regen) + "Wh</h4>";

    return content;
  }
};

#endif
