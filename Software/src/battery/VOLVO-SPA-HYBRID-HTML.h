#ifndef _VOLVO_SPA_HYBRID_HTML_H
#define _VOLVO_SPA_HYBRID_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class VolvoSpaHybridHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    tr_h4(content, TrKey::DRV_BECM_REPORTED_SOC, String(datalayer_extended.VolvoHybrid.soc_bms));
    tr_h4(content, TrKey::DRV_CALCULATED_SOC, String(datalayer_extended.VolvoHybrid.soc_calc));
    tr_h4(content, TrKey::DRV_RESCALED_SOC, String(datalayer_extended.VolvoHybrid.soc_rescaled / 10));
    tr_h4(content, TrKey::DRV_BECM_REPORTED_SOH, String(datalayer_extended.VolvoHybrid.soh_bms));
    tr_h4(content, TrKey::DRV_BECM_SUPPLY_VOLTAGE, String(datalayer_extended.VolvoHybrid.BECMsupplyVoltage), " mV");

    tr_h4(content, TrKey::DRV_HV_VOLTAGE, String(datalayer_extended.VolvoHybrid.BECMBatteryVoltage), " V");
    tr_h4(content, TrKey::DRV_HV_CURRENT, String(datalayer_extended.VolvoHybrid.BECMBatteryCurrent), " A");
    tr_h4(content, TrKey::DRV_DYNAMIC_MAX_VOLTAGE, String(datalayer_extended.VolvoHybrid.BECMUDynMaxLim), " V");
    tr_h4(content, TrKey::DRV_DYNAMIC_MIN_VOLTAGE, String(datalayer_extended.VolvoHybrid.BECMUDynMinLim), " V");

    tr_h4(content, TrKey::DRV_DISCHARGE_POWER_LIMIT_1, String(datalayer_extended.VolvoHybrid.HvBattPwrLimDcha1), " kW");
    tr_h4(content, TrKey::DRV_DISCHARGE_SOFT_POWER_LIMIT, String(datalayer_extended.VolvoHybrid.HvBattPwrLimDchaSoft),
          " kW");

    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS);
    switch (datalayer_extended.VolvoHybrid.HVSysRlySts) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_OPENANDREQUESTACTIVEDISCHARGE));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS_1);
    switch (datalayer_extended.VolvoHybrid.HVSysDCRlySts1) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_FAULT));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS_2);
    switch (datalayer_extended.VolvoHybrid.HVSysDCRlySts2) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_FAULT));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_ISOLATION_RESISTANCE_MONITORING_STATUS);
    switch (datalayer_extended.VolvoHybrid.HVSysIsoRMonrSts) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_VALID_1));
        break;
      case 1:
        content += String(TR(TrKey::DRV_FALSE));
        break;
      case 2:
        content += String(TR(TrKey::DRV_TRUE));
        break;
      case 3:
        content += String(TR(TrKey::DRV_NOT_VALID_2));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }

    return content;
  }
};

#endif
