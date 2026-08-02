#ifndef _CMP_SMART_CAR_BATTERY_HTML_H
#define _CMP_SMART_CAR_BATTERT_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

inline String getContactorStates(int index) {
  switch (index) {
    case 0:
      return TR(TrKey::DRV_OPEN);
    case 1:
      return TR(TrKey::DRV_CLOSED);
    case 2:
      return TR(TrKey::DRV_STUCK_OPEN);
    case 3:
      return TR(TrKey::DRV_STUCK_CLOSED);
    default:
      return "";
  }
}

class CmpSmartCarHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h4_open(content, TrKey::DRV_BALANCING_ACTIVE);
    if (datalayer_extended.stellantisCMPsmart.battery_balancing_active) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_POSITIVE_CONTACTOR);
    content += getContactorStates(datalayer_extended.stellantisCMPsmart.battery_positive_contactor_state);
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_NEGATIVE_CONTACTOR);
    content += getContactorStates(datalayer_extended.stellantisCMPsmart.battery_negative_contactor_state);
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_PRECHARGE_CONTACTOR);
    content += getContactorStates(datalayer_extended.stellantisCMPsmart.battery_precharge_contactor_state);
    content += "</h4>";
    tr_h4(content, TrKey::DRV_WAKEUP_REASON, String(datalayer_extended.stellantisCMPsmart.hvbat_wakeup_state));
    tr_h4_open(content, TrKey::DRV_BATTERY_STATE);
    if (datalayer_extended.stellantisCMPsmart.battery_state == 0) {
      content += TR(TrKey::DRV_SLEEP);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 1) {
      content += TR(TrKey::DRV_INITIALIZATION);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 2) {
      content += TR(TrKey::DRV_WAIT);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 3) {
      content += TR(TrKey::DRV_READY);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 4) {
      content += TR(TrKey::DRV_PREHEAT);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 5) {
      content += TR(TrKey::DRV_DISCHARGE);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 6) {
      content += TR(TrKey::DRV_CHARGE);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 7) {
      content += TR(TrKey::DRV_FAULT);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 8) {
      content += TR(TrKey::DRV_PRE_SHUTDOWN);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 9) {
      content += TR(TrKey::DRV_SHUTDOWN);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 10) {
      content += TR(TrKey::DRV_COOLING);
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 11) {
      content += TR(TrKey::DRV_HV_BATTERY_PRECONDITION);
    }
    content += "</h4>";

    tr_h4(content, TrKey::DRV_BATTERY_FAULT_LEVEL, String(datalayer_extended.stellantisCMPsmart.battery_fault));

    tr_h4_open(content, TrKey::DRV_EPLUG_STATUS);
    if (datalayer_extended.stellantisCMPsmart.eplug_status == 0) {
      content += TR(TrKey::DRV_SEATED_OK);
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 1) {
      content += TR(TrKey::DRV_DISCONNECTED);
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 2) {
      content += TR(TrKey::DRV_OPEN_STATUS);
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 3) {
      content += TR(TrKey::DRV_INVALID);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_HVIL_STATUS);
    if (datalayer_extended.stellantisCMPsmart.HVIL_status == 0) {
      content += TR(TrKey::DRV_CLOSED_OK);
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 1) {
      content += TR(TrKey::DRV_OPEN_ALARM);
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 2) {
      content += TR(TrKey::DRV_ERROR);
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 3) {
      content += TR(TrKey::DRV_INVALID);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_EV_WARNING);
    if (datalayer_extended.stellantisCMPsmart.ev_warning == 0) {
      content += TR(TrKey::DRV_OK_NO_ALARM);
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 1) {
      content += TR(TrKey::DRV_BLINKING);
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 2) {
      content += "ON!!";
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 3) {
      content += TR(TrKey::DRV_INVALID);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_AUTHORISED_USAGE);
    if (datalayer_extended.stellantisCMPsmart.power_auth) {
      tr_h4_end(content, TrKey::DRV_NOT_AUTHORISED);
    } else {
      tr_h4_end(content, TrKey::DRV_AUTHORISED_OK);
    }

    tr_h4_open(content, TrKey::DRV_CHARGING_STATUS);
    if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 0) {
      content += TR(TrKey::DRV_NOT_INITIATED);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 1) {
      content += TR(TrKey::DRV_PROGRESS);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 2) {
      content += TR(TrKey::DRV_COMPLETED);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += TR(TrKey::DRV_FAILURE);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += TR(TrKey::DRV_STOPPED);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += TR(TrKey::DRV_FORBIDDEN);
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += TR(TrKey::DRV_PROHIBITED_SUGGEST_PREHEAT_PRECONDITION);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_INSULATION_STATUS);
    if (datalayer_extended.stellantisCMPsmart.insulation_fault == 0) {
      content += TR(TrKey::DRV_OK);
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 1) {
      content += TR(TrKey::DRV_SYMMETRICAL_FAILURE);
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 2) {
      content += TR(TrKey::DRV_ASYMMETRIC_FAILURE_HV);
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 3) {
      content += TR(TrKey::DRV_ASYMMETRIC_FAILURE_HV);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_INSULATION_CIRCUIT_STATUS);
    if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 0) {
      content += TR(TrKey::DRV_INACTIVE_INSULATION_NOT_ENABLED);
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 1) {
      content += "Active (Insulation function enable)";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 2) {
      content += TR(TrKey::DRV_FAULT_ALARM);
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 3) {
      content += TR(TrKey::DRV_INSULATION_MEASUREMENT_PROGRESS);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_HARDWARE_FAULT_STATUS);
    if (datalayer_extended.stellantisCMPsmart.hardware_fault_status == 0) {
      content += TR(TrKey::DRV_NO_FAULT);
    }
    if (datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b001) {
      content += TR(TrKey::DRV_FAULT_TEMPERATURE_SENSOR);
    }
    if ((datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b010) >> 1) {
      content += TR(TrKey::DRV_FAULT_VOLTAGE_SENSING_CIRCUIT);
    }
    if ((datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b100) >> 2) {
      content += TR(TrKey::DRV_FAULT_CURRENT_SENSOR);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_L3_FAULT);
    if (datalayer_extended.stellantisCMPsmart.l3_fault == 0) {
      content += TR(TrKey::DRV_NO_FAULT);
    }
    if (datalayer_extended.stellantisCMPsmart.l3_fault & 0b001) {
      content += TR(TrKey::DRV_CELL_UNDERVOLTAGE);
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b010) >> 1) {
      content += TR(TrKey::DRV_CELL_OVERVOLTAGE);
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b100) >> 2) {
      content += TR(TrKey::DRV_OVER_TEMPERATURE);
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b1000) >> 3) {
      content += TR(TrKey::DRV_UNDER_TEMPERATURE);
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b10000) >> 4) {
      content += TR(TrKey::DRV_OVER_DISCHARGE_CURRENT);
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b100000) >> 5) {
      content += TR(TrKey::DRV_PACK_UNDEDR_VOLTAGE);
    }
    content += "</h4>";

    tr_h4_open(content, TrKey::DRV_PLAUSIBILITY_ERROR);
    if (datalayer_extended.stellantisCMPsmart.plausibility_error == 0) {
      content += TR(TrKey::DRV_NO_ERROR);
    }
    if (datalayer_extended.stellantisCMPsmart.plausibility_error & 0b001) {
      content += TR(TrKey::DRV_MODULE_TEMPERATURE_PLAUSIBILITY_ERROR);
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b010) >> 1) {
      content += TR(TrKey::DRV_CELL_VOLTAGE_PLAUSIBILITY_ERROR);
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b100) >> 2) {
      content += TR(TrKey::DRV_BATTERY_VOLTLAGE_PLAUSIBILITY_ERROR);
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b1000) >> 3) {
      content += TR(TrKey::DRV_HVBAT_CURRENT_PLAUSIBILITY_ERROR);
    }
    content += "</h4>";

    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 > 0) ||
        (datalayer_extended.stellantisCMPsmart.alert_frame4 > 0)) {
      content += "<h4>ALERT!!! ";
    }
    if (datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b001) {
      content += TR(TrKey::DRV_CELL_UNDERVOLTAGE) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b010) >> 1) {
      content += TR(TrKey::DRV_CELL_OVERVOLTAGE) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b100) >> 1) {
      content += TR(TrKey::DRV_HIGH_SOC) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b1000) >> 1) {
      content += TR(TrKey::DRV_LOW_SOC) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b10000) >> 1) {
      content += TR(TrKey::DRV_OVERVOLTAGE) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b100000) >> 1) {
      content += TR(TrKey::DRV_HIGH_TEMPERATURE) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b01000000) >> 1) {
      content += TR(TrKey::DRV_TEMPERATURE_DELTA) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 & 0b10000000) >> 1) {
      content += TR(TrKey::DRV_BATTERY) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame4 & 0b10000) >> 1) {
      content += TR(TrKey::DRV_CONTACTOR_OPENING) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame4 & 0b100000) >> 1) {
      content += TR(TrKey::DRV_OVERCHARGE) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame4 & 0b01000000) >> 1) {
      content += TR(TrKey::DRV_CELL_POOR_CONSISTENCY) + " ";
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame4 & 0b10000000) >> 1) {
      content += TR(TrKey::DRV_SOC_JUMP);
    }
    if ((datalayer_extended.stellantisCMPsmart.alert_frame3 > 0) ||
        (datalayer_extended.stellantisCMPsmart.alert_frame4 > 0)) {
      content += "</h4>";
    }

    tr_h4_open(content, TrKey::DRV_RCD_LINE_ACTIVE);
    if (datalayer_extended.stellantisCMPsmart.rcd_line_active) {
      content += TR(TrKey::DRV_YES) + "</h4>";
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }

    tr_h4_open(content, TrKey::DRV_ACTIVE_DTC_CODE);
    content += String(datalayer_extended.stellantisCMPsmart.active_DTC_code);
    if (datalayer_extended.stellantisCMPsmart.active_DTC_code == 9) {
      content += " " + TR(TrKey::DRV_TEMPERATURE_SENSOR_MISSING_BETWEEN_PIN_21_22);
    }
    content += "</h4>";
    return content;
  }
};

#endif
