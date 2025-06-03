#ifndef _VOLVO_SPA_HYBRID_HTML_H
#define _VOLVO_SPA_HYBRID_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class VolvoSpaHybridHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>BECM reported SOC: " + String(datalayer_extended.VolvoHybrid.soc_bms) + "</h4>";
    content += "<h4>Calculated SOC: " + String(datalayer_extended.VolvoHybrid.soc_calc) + "</h4>";
    content += "<h4>Rescaled SOC: " + String(datalayer_extended.VolvoHybrid.soc_rescaled / 10) + "</h4>";
    content += "<h4>BECM reported SOH: " + String(datalayer_extended.VolvoHybrid.soh_bms) + "</h4>";
    content += "<h4>BECM supply voltage: " + String(datalayer_extended.VolvoHybrid.BECMsupplyVoltage) + " mV</h4>";

    content += "<h4>HV voltage: " + String(datalayer_extended.VolvoHybrid.BECMBatteryVoltage) + " V</h4>";
    content += "<h4>HV current: " + String(datalayer_extended.VolvoHybrid.BECMBatteryCurrent) + " A</h4>";
    content += "<h4>Dynamic max voltage: " + String(datalayer_extended.VolvoHybrid.BECMUDynMaxLim) + " V</h4>";
    content += "<h4>Dynamic min voltage: " + String(datalayer_extended.VolvoHybrid.BECMUDynMinLim) + " V</h4>";

    content += "<h4>Discharge power limit 1: " + String(datalayer_extended.VolvoHybrid.HvBattPwrLimDcha1) + " kW</h4>";
    content +=
        "<h4>Discharge soft power limit: " + String(datalayer_extended.VolvoHybrid.HvBattPwrLimDchaSoft) + " kW</h4>";

    content += "<h4>HV system relay status: ";
    switch (datalayer_extended.VolvoHybrid.HVSysRlySts) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("OpenAndRequestActiveDischarge");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 1: ";
    switch (datalayer_extended.VolvoHybrid.HVSysDCRlySts1) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 2: ";
    switch (datalayer_extended.VolvoHybrid.HVSysDCRlySts2) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system isolation resistance monitoring status: ";
    switch (datalayer_extended.VolvoHybrid.HVSysIsoRMonrSts) {
      case 0:
        content += String("Not valid 1");
        break;
      case 1:
        content += String("False");
        break;
      case 2:
        content += String("True");
        break;
      case 3:
        content += String("Not valid 2");
        break;
      default:
        content += String("Not valid");
    }

    return content;
  }
};

#endif
