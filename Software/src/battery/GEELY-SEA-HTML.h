#ifndef _VOLVO_SEA2_BATTERY_H
#define _VOLVO_SEA2_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class GeelySeaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>BECM reported SOC: " + String(datalayer_extended.GeelySEA.soc_bms / 500.0) + " %</h4>";
    content += "<h4>BECM reported SOH: " + String(datalayer_extended.GeelySEA.soh_bms / 100.0) + " %</h4>";
    content += "<h4>HV voltage: " + String(datalayer_extended.GeelySEA.BECMBatteryVoltage / 100.0) + " V</h4>";
    content += "<h4>BECM supply voltage: " + String(datalayer_extended.GeelySEA.BECMsupplyVoltage / 1000.0) + " V</h4>";
    content += "<h4>Highest cell temp: " + String((datalayer_extended.GeelySEA.CellTempHighest / 100.0)-50.0) + " ºC</h4>";
    content += "<h4>Lowest cell temp: " + String((datalayer_extended.GeelySEA.CellTempLowest / 100.0)-50.0) + " ºC</h4>";
    return content;
  }
};

#endif
