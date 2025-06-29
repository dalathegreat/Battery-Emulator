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

    static const char* SOCmethod[2] = {"Estimated from voltage", "Measured by BMS"};
    content += "<h4>SOC method used: " + String(SOCmethod[byd_datalayer->SOC_method]) + "</h4>";
    content += "<h4>SOC estimated: " + String(byd_datalayer->SOC_estimated) + "</h4>";
    content += "<h4>SOC highprec: " + String(byd_datalayer->SOC_highprec) + "</h4>";
    content += "<h4>SOC OBD2: " + String(byd_datalayer->SOC_polled) + "</h4>";
    content += "<h4>Voltage periodic: " + String(byd_datalayer->voltage_periodic) + "</h4>";
    content += "<h4>Voltage OBD2: " + String(byd_datalayer->voltage_polled) + "</h4>";
    content += "<h4>Temperature sensor 1: " + String(byd_datalayer->battery_temperatures[0]) + "</h4>";
    content += "<h4>Temperature sensor 2: " + String(byd_datalayer->battery_temperatures[1]) + "</h4>";
    content += "<h4>Temperature sensor 3: " + String(byd_datalayer->battery_temperatures[2]) + "</h4>";
    content += "<h4>Temperature sensor 4: " + String(byd_datalayer->battery_temperatures[3]) + "</h4>";
    content += "<h4>Temperature sensor 5: " + String(byd_datalayer->battery_temperatures[4]) + "</h4>";
    content += "<h4>Temperature sensor 6: " + String(byd_datalayer->battery_temperatures[5]) + "</h4>";
    content += "<h4>Temperature sensor 7: " + String(byd_datalayer->battery_temperatures[6]) + "</h4>";
    content += "<h4>Temperature sensor 8: " + String(byd_datalayer->battery_temperatures[7]) + "</h4>";
    content += "<h4>Temperature sensor 9: " + String(byd_datalayer->battery_temperatures[8]) + "</h4>";
    content += "<h4>Temperature sensor 10: " + String(byd_datalayer->battery_temperatures[9]) + "</h4>";
    content += "<h4>Unknown0: " + String(byd_datalayer->unknown0) + "</h4>";
    content += "<h4>Unknown1: " + String(byd_datalayer->unknown1) + "</h4>";
    content += "<h4>Charge power raw: " + String(byd_datalayer->chargePower) + "</h4>";
    content += "<h4>Unknown3: " + String(byd_datalayer->unknown3) + "</h4>";
    content += "<h4>Unknown4: " + String(byd_datalayer->unknown4) + "</h4>";
    content += "<h4>Total charged Ah: " + String(byd_datalayer->total_charged_ah) + "</h4>";
    content += "<h4>Total discharged Ah: " + String(byd_datalayer->total_discharged_ah) + "</h4>";
    content += "<h4>Total charged kWh: " + String(byd_datalayer->total_charged_kwh) + "</h4>";
    content += "<h4>Total discharged kWh: " + String(byd_datalayer->total_discharged_kwh) + "</h4>";
    content += "<h4>Unknown9: " + String(byd_datalayer->unknown9) + "</h4>";
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
