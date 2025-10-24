#ifndef _CMP_SMART_CAR_BATTERY_HTML_H
#define _CMP_SMART_CAR_BATTERT_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CmpSmartCarHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>Balancing active: ";
    if (datalayer_extended.stellantisCMPsmart.battery_balancing_active) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Positive contactor: ";
    if (datalayer_extended.stellantisCMPsmart.battery_positive_contactor_state == 0) {
      content += "Open";
    } else if (datalayer_extended.stellantisCMPsmart.battery_positive_contactor_state == 1) {
      content += "Closed";
    } else if (datalayer_extended.stellantisCMPsmart.battery_positive_contactor_state == 2) {
      content += "STUCK Open!";
    } else if (datalayer_extended.stellantisCMPsmart.battery_positive_contactor_state == 3) {
      content += "STUCK Closed!";
    }
    content += "</h4>";
    content += "<h4>Negative contactor: ";
    if (datalayer_extended.stellantisCMPsmart.battery_negative_contactor_state == 0) {
      content += "Open";
    } else if (datalayer_extended.stellantisCMPsmart.battery_negative_contactor_state == 1) {
      content += "Closed";
    } else if (datalayer_extended.stellantisCMPsmart.battery_negative_contactor_state == 2) {
      content += "STUCK Open!";
    } else if (datalayer_extended.stellantisCMPsmart.battery_negative_contactor_state == 3) {
      content += "STUCK Closed!";
    }
    content += "</h4>";
    content += "<h4>Precharge contactor: ";
    if (datalayer_extended.stellantisCMPsmart.battery_precharge_contactor_state == 0) {
      content += "Open";
    } else if (datalayer_extended.stellantisCMPsmart.battery_precharge_contactor_state == 1) {
      content += "Closed";
    } else if (datalayer_extended.stellantisCMPsmart.battery_precharge_contactor_state == 2) {
      content += "STUCK Open!";
    } else if (datalayer_extended.stellantisCMPsmart.battery_precharge_contactor_state == 3) {
      content += "STUCK Closed!";
    }
    content += "</h4>";
    content += "<h4>QC positive contactor: ";
    if (datalayer_extended.stellantisCMPsmart.qc_positive_contactor_status == 0) {
      content += "Open";
    } else if (datalayer_extended.stellantisCMPsmart.qc_positive_contactor_status == 1) {
      content += "Closed";
    } else if (datalayer_extended.stellantisCMPsmart.qc_positive_contactor_status == 2) {
      content += "Fault!";
    }
    content += "</h4>";
    content += "<h4>QC negative contactor: ";
    if (datalayer_extended.stellantisCMPsmart.qc_negative_contactor_status == 0) {
      content += "Open";
    } else if (datalayer_extended.stellantisCMPsmart.qc_negative_contactor_status == 1) {
      content += "Closed";
    } else if (datalayer_extended.stellantisCMPsmart.qc_negative_contactor_status == 2) {
      content += "Fault!";
    }
    content += "</h4>";
    content += "<h4>Wakeup reason: " + String(datalayer_extended.stellantisCMPsmart.hvbat_wakeup_state) + "</h4>";
    content += "<h4>Battery state: ";
    if (datalayer_extended.stellantisCMPsmart.battery_state == 0) {
      content += "Sleep";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 1) {
      content += "Initialization";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 2) {
      content += "Wait";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 3) {
      content += "Ready";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 4) {
      content += "Preheat";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 5) {
      content += "Discharge";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 6) {
      content += "Charge";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 7) {
      content += "Fault";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 8) {
      content += "Pre-shutdown";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 9) {
      content += "Shutdown";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 10) {
      content += "Cooling";
    } else if (datalayer_extended.stellantisCMPsmart.battery_state == 11) {
      content += "HV battery precondition";
    }
    content += "</h4>";

    content += "<h4>Battery fault level: " + String(datalayer_extended.stellantisCMPsmart.battery_fault) + "</h4>";

    content += "<h4>Eplug status: ";
    if (datalayer_extended.stellantisCMPsmart.eplug_status == 0) {
      content += "Seated OK";
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 1) {
      content += "Disconnected!";
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 2) {
      content += "Open Status";
    } else if (datalayer_extended.stellantisCMPsmart.eplug_status == 3) {
      content += "Invalid";
    }
    content += "</h4>";

    content += "<h4>HVIL status: ";
    if (datalayer_extended.stellantisCMPsmart.HVIL_status == 0) {
      content += "Closed OK";
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 1) {
      content += "OPEN!!";
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 2) {
      content += "Error";
    } else if (datalayer_extended.stellantisCMPsmart.HVIL_status == 3) {
      content += "Invalid";
    }
    content += "</h4>";

    content += "<h4>EV Warning: ";
    if (datalayer_extended.stellantisCMPsmart.ev_warning == 0) {
      content += "OK No alarm";
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 1) {
      content += "Blinking!!";
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 2) {
      content += "ON!!";
    } else if (datalayer_extended.stellantisCMPsmart.ev_warning == 3) {
      content += "Invalid";
    }
    content += "</h4>";

    content += "<h4>Authorised for usage: ";
    if (datalayer_extended.stellantisCMPsmart.power_auth) {
      content += "NOT authorised for HVBAT usage</h4>";
    } else {
      content += "Authorised for HVBAT usage OK</h4>";
    }

    content += "<h4>Charging status: ";
    if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 0) {
      content += "Not initiated";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 1) {
      content += "In progress";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 2) {
      content += "Completed";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += "Failure";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += "Stopped";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += "Forbidden";
    } else if (datalayer_extended.stellantisCMPsmart.battery_charging_status == 3) {
      content += "Prohibited, suggest preheat or precondition";
    }
    content += "</h4>";

    content += "<h4>Insulation status: ";
    if (datalayer_extended.stellantisCMPsmart.insulation_fault == 0) {
      content += "OK";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 1) {
      content += "Symmetrical failure!!";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 2) {
      content += "Asymmetric failure HV+!!";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_fault == 3) {
      content += "Asymmetric failure HV-!!";
    }
    content += "</h4>";

    content += "<h4>Insulation circuit status: ";
    if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 0) {
      content += "Inactive (Insulation function not enable)";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 1) {
      content += "Active (Insulation function enable)";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 2) {
      content += "FAULT!!";
    } else if (datalayer_extended.stellantisCMPsmart.insulation_circuit_status == 3) {
      content += "Insulation measurement in progress";
    }
    content += "</h4>";

    content += "<h4>Hardware fault status: ";
    if (datalayer_extended.stellantisCMPsmart.hardware_fault_status == 0) {
      content += "No Fault";
    }
    if (datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b001) {
      content += "FAULT! Temperature sensor!";
    }
    if ((datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b010) >> 1) {
      content += "FAULT! Voltage sensing circuit!";
    }
    if ((datalayer_extended.stellantisCMPsmart.hardware_fault_status & 0b100) >> 2) {
      content += "FAULT! Current sensor!";
    }
    content += "</h4>";

    content += "<h4>L3 Fault: ";
    if (datalayer_extended.stellantisCMPsmart.l3_fault == 0) {
      content += "No Fault";
    }
    if (datalayer_extended.stellantisCMPsmart.l3_fault & 0b001) {
      content += "Cell undervoltage";
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b010) >> 1) {
      content += "Cell overvoltage";
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b100) >> 2) {
      content += "Over temperature";
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b1000) >> 3) {
      content += "Under temperature";
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b10000) >> 4) {
      content += "Over discharge current";
    }
    if ((datalayer_extended.stellantisCMPsmart.l3_fault & 0b100000) >> 5) {
      content += "Pack undedr voltage";
    }
    content += "</h4>";

    content += "<h4>Plausibility error: ";
    if (datalayer_extended.stellantisCMPsmart.plausibility_error == 0) {
      content += "No error";
    }
    if (datalayer_extended.stellantisCMPsmart.plausibility_error & 0b001) {
      content += "Module temperature plausibility error";
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b010) >> 1) {
      content += "Cell voltage plausibility error";
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b100) >> 2) {
      content += "Battery voltlage plausibility error";
    }
    if ((datalayer_extended.stellantisCMPsmart.plausibility_error & 0b1000) >> 3) {
      content += "HVBAT Current plausibility error";
    }
    content += "</h4>";

    content += "<h4>Alert, cell undervoltage: ";
    if (datalayer_extended.stellantisCMPsmart.alert_cell_undervoltage) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, cell overvoltage: ";
    if (datalayer_extended.stellantisCMPsmart.alert_cell_overvoltage) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, high SOC: ";
    if (datalayer_extended.stellantisCMPsmart.alert_high_SOC) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, low SOC: ";
    if (datalayer_extended.stellantisCMPsmart.alert_low_SOC) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, overvoltage: ";
    if (datalayer_extended.stellantisCMPsmart.alert_overvoltage) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, overtemperature: ";
    if (datalayer_extended.stellantisCMPsmart.alert_high_temperature) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, temperature delta: ";
    if (datalayer_extended.stellantisCMPsmart.alert_temperature_delta) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, battery: ";
    if (datalayer_extended.stellantisCMPsmart.alert_battery) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, SOC jump: ";
    if (datalayer_extended.stellantisCMPsmart.alert_SOC_jump) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, poor cell consistency: ";
    if (datalayer_extended.stellantisCMPsmart.alert_cell_poor_consistency) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, overcharge: ";
    if (datalayer_extended.stellantisCMPsmart.alert_overcharge) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }
    content += "<h4>Alert, contactor opening: ";
    if (datalayer_extended.stellantisCMPsmart.alert_contactor_opening) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }

    content += "<h4>RCD line active: ";
    if (datalayer_extended.stellantisCMPsmart.rcd_line_active) {
      content += "Yes </h4>";
    } else {
      content += "No </h4>";
    }

    content += "<h4>Active DTC Code: " + String(datalayer_extended.stellantisCMPsmart.active_DTC_code) + "</h4>";
    return content;
  }
};

#endif
