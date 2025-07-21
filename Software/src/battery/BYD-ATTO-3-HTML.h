#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl) : byd_datalayer(dl) {}

  String get_status_html() {
    String content;

    float soc_estimated = static_cast<float>(byd_datalayer->SOC_estimated) * 0.01;
    float soc_measured = static_cast<float>(byd_datalayer->SOC_highprec) * 0.1;
    float BMS_maxChargePower = static_cast<float>(byd_datalayer->chargePower) * 0.1;
    float BMS_maxDischargePower = static_cast<float>(byd_datalayer->dischargePower) * 0.1;
    static const char* SOCmethod[2] = {"Estimated from voltage", "Measured by BMS"};

    content += "<h4>SOC method used: " + String(SOCmethod[byd_datalayer->SOC_method]) + "</h4>";
    content += "<h4>SOC estimated: " + String(soc_estimated) + "&percnt;</h4>";
    content += "<h4>SOC measured: " + String(soc_measured) + "&percnt;</h4>";
    content += "<h4>SOC OBD2: " + String(byd_datalayer->SOC_polled) + "&percnt;</h4>";
    content += "<h4>Voltage periodic: " + String(byd_datalayer->voltage_periodic) + " V</h4>";
    content += "<h4>Voltage OBD2: " + String(byd_datalayer->voltage_polled) + " V</h4>";
    content += "<h4>Temperature sensor 1: " + String(byd_datalayer->battery_temperatures[0]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 2: " + String(byd_datalayer->battery_temperatures[1]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 3: " + String(byd_datalayer->battery_temperatures[2]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 4: " + String(byd_datalayer->battery_temperatures[3]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 5: " + String(byd_datalayer->battery_temperatures[4]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 6: " + String(byd_datalayer->battery_temperatures[5]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 7: " + String(byd_datalayer->battery_temperatures[6]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 8: " + String(byd_datalayer->battery_temperatures[7]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 9: " + String(byd_datalayer->battery_temperatures[8]) + " &deg;C</h4>";
    content += "<h4>Temperature sensor 10: " + String(byd_datalayer->battery_temperatures[9]) + " &deg;C</h4>";
    content += "<h4>Max discharge power: " + String(BMS_maxDischargePower) + " kW</h4>";
    content += "<h4>Max charge (regen) power: " + String(BMS_maxChargePower) + " kW</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_kwh) + " kWh</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_kwh) + " kWh</h4>";
    content += "<h4>Total charged: " + String(byd_datalayer->total_charged_ah) + " Ah</h4>";
    content += "<h4>Total discharged: " + String(byd_datalayer->total_discharged_ah) + " Ah</h4>";
    content += "<h4>Charge times: " + String(byd_datalayer->charge_times) + "</h4>";
    content += "<h4>Times of full power: " + String(byd_datalayer->times_full_power) + "</h4>";
    content += "<h4>Unknown0: " + String(byd_datalayer->unknown0) + "</h4>";
    content += "<h4>Unknown1: " + String(byd_datalayer->unknown1) + "</h4>";
    content += "<h4>Unknown10: " + String(byd_datalayer->unknown10) + "</h4>";
    content += "<h4>Unknown11: " + String(byd_datalayer->unknown11) + "</h4>";
    content += "<h4>Unknown12: " + String(byd_datalayer->unknown12) + "</h4>";
    content += "<h4>Unknown13: " + String(byd_datalayer->unknown12) + "</h4>";

    return content;
  }

 private:
  DATALAYER_INFO_BYDATTO3* byd_datalayer;
};

#endif
