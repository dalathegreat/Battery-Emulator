#ifndef _CHADEMO_BATTERY_HTML_H
#define _CHADEMO_BATTERY_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class ChademoBatteryHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h4_open(content, TrKey::DRV_CHADEMO_STATE);
    switch (datalayer_extended.chademo.CHADEMO_Status) {
      case 0:
        content += String(TR(TrKey::DRV_FAULT) + "</h4>");
        break;
      case 1:
        content += String(TR(TrKey::UI_STOP) + "</h4>");
        break;
      case 2:
        content += String(TR(TrKey::DRV_IDLE) + "</h4>");
        break;
      case 3:
        content += String(TR(TrKey::UI_CONNECTED) + "</h4>");
        break;
      case 4:
        content += String(TR(TrKey::DRV_INIT) + "</h4>");
        break;
      case 5:
        content += String("NEGOTIATE</h4>");
        break;
      case 6:
        content += String(TR(TrKey::DRV_EV_ALLOWED) + "</h4>");
        break;
      case 7:
        content += String(TR(TrKey::DRV_EVSE_PREPARE) + "</h4>");
        break;
      case 8:
        content += String(TR(TrKey::DRV_EVSE_START) + "</h4>");
        break;
      case 9:
        content += String(TR(TrKey::DRV_EVSE_CONTACTORS_ENABLED) + "</h4>");
        break;
      case 10:
        content += String("POWERFLOW</h4>");
        break;
      default:
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
        break;
    }
    if (datalayer_extended.chademo.FaultBatteryCurrentDeviation) {
      tr_h4(content, TrKey::DRV_FAULT_BATTERY_CURRENT_DEVIATION);
    }
    if (datalayer_extended.chademo.FaultBatteryOverVoltage) {
      tr_h4(content, TrKey::DRV_FAULT_BATTERY_OVERVOLTAGE);
    }
    if (datalayer_extended.chademo.FaultBatteryUnderVoltage) {
      tr_h4(content, TrKey::DRV_FAULT_BATTERY_UNDERVOLTAGE);
    }
    if (datalayer_extended.chademo.FaultBatteryVoltageDeviation) {
      tr_h4(content, TrKey::DRV_FAULT_BATTERY_VOLTAGE_DEVIATION);
    }
    if (datalayer_extended.chademo.FaultHighBatteryTemperature) {
      tr_h4(content, TrKey::DRV_FAULT_BATTERY_TEMPERATURE);
    }
    tr_h4(content, TrKey::DRV_PROTOCOL, String(datalayer_extended.chademo.ControlProtocolNumberEV));

    //Script for refreshing page
    content += "<script>";
    content += "setTimeout(function(){ location.reload(true); }, 5000);";
    content += "</script>";

    return content;
  }
};

#endif
