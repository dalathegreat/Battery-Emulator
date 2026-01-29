#ifndef _GEELY_SEA_HTML_H
#define _GEELY_SEA_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class GeelySeaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "</h4><h4>BECM reported number of DTCs: " + String(datalayer_extended.GeelySEA.DTCcount) + "</h4>";
    content += "</h4><h4>Inhibition status (crash): " + String(datalayer_extended.GeelySEA.CrashStatus) + "</h4>";
    content += "<h4>BECM reported SOC: " + String(datalayer_extended.GeelySEA.soc_bms / 100.0) + " %</h4>";
    content += "<h4>BECM reported SOH: " + String(datalayer_extended.GeelySEA.soh_bms / 100.0) + " %</h4>";
    content += "<h4>HV voltage: " + String(datalayer_extended.GeelySEA.BECMBatteryVoltage / 100.0) + " V</h4>";
    //content += "<h4>Battery current: " + String((datalayer_extended.GeelySEA.BatteryCurrent / 10.0) - 1638) + " A</h4>";
    content += "<h4>Highest cell voltage: " + String(datalayer_extended.GeelySEA.CellVoltHighest / 1000.00) + " V</h4>";
    content += "<h4>Lowest cell voltage: " + String(datalayer_extended.GeelySEA.CellVoltLowest / 1000.00) + " V</h4>";
    content += "<h4>BECM supply voltage: " + String(datalayer_extended.GeelySEA.BECMsupplyVoltage / 1000.0) + " V</h4>";
    content += "<h4>Cell count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content +=
        "<h4>Highest cell temp: " + String((datalayer_extended.GeelySEA.CellTempHighest / 100.0) - 50.0) + " ºC</h4>";
    content +=
        "<h4>Average cell temp: " + String((datalayer_extended.GeelySEA.CellTempAverage / 100.0) - 50.0) + " ºC</h4>";
    content +=
        "<h4>Lowest cell temp: " + String((datalayer_extended.GeelySEA.CellTempLowest / 100.0) - 50.0) + " ºC</h4>";
    content += "<h4>HVIL Circuit 1 (M1+M2+FC connectors) status : ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x80) {
      case 0x80:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>HVIL Circuit 2 (LV connector pin 9-10) status: ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x40) {
      case 0x40:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>HVIL Circuit 3 (LV connector pin 8-12) status: ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x04) {
      case 0x04:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Unknow Contactor Status 1 (Negative FC?): ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x01) {
      case 0x01:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Unknown Contactor Status 2 (Positive FC?): ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x02) {
      case 0x02:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Negative Contactor Status: ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x08) {
      case 0x08:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Precharge Contactor Status: ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x10) {
      case 0x10:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Positive Contactor Status: ";
    switch (datalayer_extended.GeelySEA.Interlock & 0x20) {
      case 0x20:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>";
    return content;
  }
};

#endif
