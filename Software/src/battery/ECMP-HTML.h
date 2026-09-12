#ifndef _ECMP_BATTERY_HTML_H
#define _ECMP_BATTERT_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class EcmpHtmlRenderer : public BatteryHtmlRenderer {
 public:
  // Each EcmpBattery instance passes a pointer to its own extended-data struct
  // (datalayer_extended.stellantisECMP / _2 / _3) so battery 2 and 3 render
  // their own More Battery Info instead of battery 1's globals.
  EcmpHtmlRenderer(DATALAYER_INFO_ECMP* dl) : ecmp(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    if (!ecmp) {
      return content;
    }
    content += "<h4>Main Connector State: ";
    if (ecmp->MainConnectorState == 0) {
      content += "Contactors open</h4>";
    } else if (ecmp->MainConnectorState == 0x01) {
      content += "Precharged</h4>";
    } else {
      content += "Invalid</h4>";
    }
    content += "<h4>Insulation Resistance: " + String(ecmp->InsulationResistance) + "kOhm</h4>";
    content += "<h4>Interlock:  ";
    if (ecmp->InterlockOpen == true) {
      content += "BROKEN!</h4>";
    } else {
      content += "Seated OK</h4>";
    }
    content += "<h4>Insulation Diag: ";
    if (ecmp->InsulationDiag == 0) {
      content += "No failure</h4>";
    } else if (ecmp->InsulationDiag == 1) {
      content += "Symmetric failure</h4>";
    } else {  //4 Invalid, 5-7 illegal, wrap em under one text
      content += "N/A</h4>";
    }
    content += "<h4>Contactor weld check: ";
    if (ecmp->pid_welding_detection == 0) {
      content += "OK</h4>";
    } else if (ecmp->pid_welding_detection == 255) {
      content += "N/A</h4>";
    } else {  //Problem
      content += "WELDED!" + String(ecmp->pid_welding_detection) + "</h4>";
    }

    content += "<h4>Contactor opening reason: ";
    if (ecmp->pid_reason_open == 7) {
      content += "Invalid Status</h4>";
    } else if (ecmp->pid_reason_open == 255) {
      content += "N/A</h4>";
    } else {  //Problem (Also status 0 might be OK?)
      content += "Unknown" + String(ecmp->pid_reason_open) + "</h4>";
    }

    content += "<h4>Status of power switch: " +
               (ecmp->pid_contactor_status == 255 ? "N/A" : String(ecmp->pid_contactor_status)) + "</h4>";
    content += "<h4>Negative power switch control: " +
               (ecmp->pid_negative_contactor_control == 255 ? "N/A" : String(ecmp->pid_negative_contactor_control)) +
               "</h4>";
    content += "<h4>Negative power switch status: " +
               (ecmp->pid_negative_contactor_status == 255 ? "N/A" : String(ecmp->pid_negative_contactor_status)) +
               "</h4>";
    content += "<h4>Positive power switch control: " +
               (ecmp->pid_positive_contactor_control == 255 ? "N/A" : String(ecmp->pid_positive_contactor_control)) +
               "</h4>";
    content += "<h4>Positive power switch status: " +
               (ecmp->pid_positive_contactor_status == 255 ? "N/A" : String(ecmp->pid_positive_contactor_status)) +
               "</h4>";
    content += "<h4>Contactor negative: " +
               (ecmp->pid_contactor_negative == 255 ? "N/A" : String(ecmp->pid_contactor_negative)) + "</h4>";
    content += "<h4>Contactor positive: " +
               (ecmp->pid_contactor_positive == 255 ? "N/A" : String(ecmp->pid_contactor_positive)) + "</h4>";
    content += "<h4>Precharge control: " +
               (ecmp->pid_precharge_relay_control == 255 ? "N/A" : String(ecmp->pid_precharge_relay_control)) + "</h4>";
    content += "<h4>Precharge status: " +
               (ecmp->pid_precharge_relay_status == 255 ? "N/A" : String(ecmp->pid_precharge_relay_status)) + "</h4>";
    content +=
        "<h4>Recharge Status: " + (ecmp->pid_recharge_status == 255 ? "N/A" : String(ecmp->pid_recharge_status)) +
        "</h4>";
    content +=
        "<h4>Delta temperature: " + (ecmp->pid_delta_temperature == 127 ? "N/A" : String(ecmp->pid_delta_temperature)) +
        "&deg;C</h4>";
    content += "<h4>Lowest temperature: " +
               (ecmp->pid_lowest_temperature == 127 ? "N/A" : String(ecmp->pid_lowest_temperature)) + "&deg;C</h4>";
    content += "<h4>Average temperature: " +
               (ecmp->pid_average_temperature == 127 ? "N/A" : String(ecmp->pid_average_temperature)) + "&deg;C</h4>";
    content += "<h4>Highest temperature: " +
               (ecmp->pid_highest_temperature == 127 ? "N/A" : String(ecmp->pid_highest_temperature)) + "&deg;C</h4>";
    content +=
        "<h4>Coldest module: " + (ecmp->pid_coldest_module == 255 ? "N/A" : String(ecmp->pid_coldest_module)) + "</h4>";
    content +=
        "<h4>Hottest module: " + (ecmp->pid_hottest_module == 255 ? "N/A" : String(ecmp->pid_hottest_module)) + "</h4>";
    content += "<h4>Average cell voltage: " +
               (ecmp->pid_avg_cell_voltage == 255 ? "N/A" : String(ecmp->pid_avg_cell_voltage)) + " mV</h4>";
    content +=
        "<h4>High precision current: " + (ecmp->pid_current == 255 ? "N/A" : String(ecmp->pid_current)) + " mA</h4>";
    content += "<h4>Insulation resistance neg-gnd: " +
               (ecmp->pid_insulation_res_neg == 255 ? "N/A" : String(ecmp->pid_insulation_res_neg)) + " kOhm</h4>";
    content += "<h4>Insulation resistance pos-gnd: " +
               (ecmp->pid_insulation_res_pos == 255 ? "N/A" : String(ecmp->pid_insulation_res_pos)) + " kOhm</h4>";
    content +=
        "<h4>Max current 10s: " + (ecmp->pid_max_current_10s == 255 ? "N/A" : String(ecmp->pid_max_current_10s)) +
        "</h4>";
    content += "<h4>Max discharge power 10s: " +
               (ecmp->pid_max_discharge_10s == 255 ? "N/A" : String(ecmp->pid_max_discharge_10s)) + "</h4>";
    content += "<h4>Max discharge power 30s: " +
               (ecmp->pid_max_discharge_30s == 255 ? "N/A" : String(ecmp->pid_max_discharge_30s)) + "</h4>";
    content +=
        "<h4>Max charge power 10s: " + (ecmp->pid_max_charge_10s == 255 ? "N/A" : String(ecmp->pid_max_charge_10s)) +
        "</h4>";
    content +=
        "<h4>Max charge power 30s: " + (ecmp->pid_max_charge_30s == 255 ? "N/A" : String(ecmp->pid_max_charge_30s)) +
        "</h4>";
    content +=
        "<h4>Energy capacity: " + (ecmp->pid_energy_capacity == 255 ? "N/A" : String(ecmp->pid_energy_capacity)) +
        "</h4>";
    content += "<h4>Highest cell number: " +
               (ecmp->pid_highest_cell_voltage_num == 255 ? "N/A" : String(ecmp->pid_highest_cell_voltage_num)) +
               "</h4>";
    content += "<h4>Lowest cell voltage number: " +
               (ecmp->pid_lowest_cell_voltage_num == 255 ? "N/A" : String(ecmp->pid_lowest_cell_voltage_num)) + "</h4>";
    content +=
        "<h4>Sum of all cell voltages: " + (ecmp->pid_sum_of_cells == 255 ? "N/A" : String(ecmp->pid_sum_of_cells)) +
        " dV</h4>";
    content +=
        "<h4>Cell min capacity: " + (ecmp->pid_cell_min_capacity == 255 ? "N/A" : String(ecmp->pid_cell_min_capacity)) +
        "</h4>";
    content +=
        "<h4>Cell voltage measurement status: " +
        (ecmp->pid_cell_voltage_measurement_status == 255 ? "N/A" : String(ecmp->pid_cell_voltage_measurement_status)) +
        "</h4>";
    content += "<h4>Battery Insulation Resistance: " +
               (ecmp->pid_insulation_res == 255 ? "N/A" : String(ecmp->pid_insulation_res)) + " kOhm</h4>";
    content +=
        "<h4>Pack voltage: " + (ecmp->pid_pack_voltage == 255 ? "N/A" : String(ecmp->pid_pack_voltage)) + " dV</h4>";
    content += "<h4>Highest cell voltage: " +
               (ecmp->pid_high_cell_voltage == 255 ? "N/A" : String(ecmp->pid_high_cell_voltage)) + " mV</h4>";
    content +=
        "<h4>Lowest cell voltage: " + (ecmp->pid_low_cell_voltage == 255 ? "N/A" : String(ecmp->pid_low_cell_voltage)) +
        " mV</h4>";
    content +=
        "<h4>Battery Energy: " + (ecmp->pid_battery_energy == 255 ? "N/A" : String(ecmp->pid_battery_energy)) + "</h4>";
    content += "<h4>Collision information Counter: " +
               (ecmp->pid_crash_counter == 255 ? "N/A" : String(ecmp->pid_crash_counter)) + "</h4>";
    content += "<h4>Collision Counter recieved by Wire: " +
               (ecmp->pid_wire_crash == 255 ? "N/A" : String(ecmp->pid_wire_crash)) + "</h4>";
    content += "<h4>Collision data sent from car to battery: " +
               (ecmp->pid_CAN_crash == 255 ? "N/A" : String(ecmp->pid_CAN_crash)) + "</h4>";
    content +=
        "<h4>History data: " + (ecmp->pid_history_data == 255 ? "N/A" : String(ecmp->pid_history_data)) + "</h4>";
    content += "<h4>Low SOC counter: " + (ecmp->pid_lowsoc_counter == 255 ? "N/A" : String(ecmp->pid_lowsoc_counter)) +
               "</h4>";
    content += "<h4>Last CAN failure detail: " +
               (ecmp->pid_last_can_failure_detail == 255 ? "N/A" : String(ecmp->pid_last_can_failure_detail)) + "</h4>";
    content +=
        "<h4>HW version number: " + (ecmp->pid_hw_version_num == 255 ? "N/A" : String(ecmp->pid_hw_version_num)) +
        "</h4>";
    content +=
        "<h4>SW version number: " + (ecmp->pid_sw_version_num == 255 ? "N/A" : String(ecmp->pid_sw_version_num)) +
        "</h4>";
    content += "<h4>Factory mode: " +
               (ecmp->pid_factory_mode_control == 255 ? "N/A" : String(ecmp->pid_factory_mode_control)) + "</h4>";
    char readableSerialNumber[14];  // One extra space for null terminator
    memcpy(readableSerialNumber, ecmp->pid_battery_serial, sizeof(ecmp->pid_battery_serial));
    readableSerialNumber[13] = '\0';  // Null terminate the string
    content += "<h4>Battery serial: " + String(readableSerialNumber) + "</h4>";
    uint8_t day = (ecmp->pid_date_of_manufacture >> 16) & 0xFF;
    uint8_t month = (ecmp->pid_date_of_manufacture >> 8) & 0xFF;
    uint8_t year = ecmp->pid_date_of_manufacture & 0xFF;
    content += "<h4>Date of manufacture: " + String(day) + "/" + String(month) + "/" + String(year) + "</h4>";
    content +=
        "<h4>Aux fuse state: " + (ecmp->pid_aux_fuse_state == 255 ? "N/A" : String(ecmp->pid_aux_fuse_state)) + "</h4>";
    content +=
        "<h4>Battery state: " + (ecmp->pid_battery_state == 255 ? "N/A" : String(ecmp->pid_battery_state)) + "</h4>";
    content += "<h4>Precharge short circuit: " +
               (ecmp->pid_precharge_short_circuit == 255 ? "N/A" : String(ecmp->pid_precharge_short_circuit)) + "</h4>";
    content += "<h4>Service plug state: " +
               (ecmp->pid_eservice_plug_state == 255 ? "N/A" : String(ecmp->pid_eservice_plug_state)) + "</h4>";
    content += "<h4>Main fuse state: " + (ecmp->pid_mainfuse_state == 255 ? "N/A" : String(ecmp->pid_mainfuse_state)) +
               "</h4>";
    content += "<h4>Most critical fault: " +
               (ecmp->pid_most_critical_fault == 255 ? "N/A" : String(ecmp->pid_most_critical_fault)) + "</h4>";
    content +=
        "<h4>Current time: " + (ecmp->pid_current_time == 255 ? "N/A" : String(ecmp->pid_current_time)) + " ticks</h4>";
    content +=
        "<h4>Time sent by car: " + (ecmp->pid_time_sent_by_car == 255 ? "N/A" : String(ecmp->pid_time_sent_by_car)) +
        " ticks</h4>";
    content += "<h4>12V: " + (ecmp->pid_12v == 255 ? "N/A" : String(ecmp->pid_12v)) + "</h4>";
    content += "<h4>12V abnormal: ";
    if (ecmp->pid_12v_abnormal == 255) {
      content += "N/A</h4>";
    } else if (ecmp->pid_12v_abnormal == 0) {
      content += "No</h4>";
    } else {
      content += "Yes</h4>";
    }
    content +=
        "<h4>HVIL IN Voltage: " + (ecmp->pid_hvil_in_voltage == 255 ? "N/A" : String(ecmp->pid_hvil_in_voltage)) +
        "mV</h4>";
    content +=
        "<h4>HVIL Out Voltage: " + (ecmp->pid_hvil_out_voltage == 255 ? "N/A" : String(ecmp->pid_hvil_out_voltage)) +
        "mV</h4>";
    content +=
        "<h4>HVIL State: " +
        (ecmp->pid_hvil_state == 255 ? "N/A" : (ecmp->pid_hvil_state == 0 ? "OK" : String(ecmp->pid_hvil_state))) +
        "</h4>";
    content += "<h4>BMS State: " +
               (ecmp->pid_bms_state == 255 ? "N/A" : (ecmp->pid_bms_state == 0 ? "OK" : String(ecmp->pid_bms_state))) +
               "</h4>";
    content += "<h4>Vehicle speed: " + (ecmp->pid_vehicle_speed == 255 ? "N/A" : String(ecmp->pid_vehicle_speed)) +
               " km/h</h4>";
    content += "<h4>Time spent over 55c: " +
               (ecmp->pid_time_spent_over_55c == 255 ? "N/A" : String(ecmp->pid_time_spent_over_55c)) + " minutes</h4>";
    content += "<h4>Contactor lifetime closing counter: " +
               (ecmp->pid_contactor_closing_counter == 255 ? "N/A" : String(ecmp->pid_contactor_closing_counter)) +
               " cycles</h4>";
    content +=
        "<h4>State of Health Cell-1: " + (ecmp->pid_SOH_cell_1 == 255 ? "N/A" : String(ecmp->pid_SOH_cell_1)) + "</h4>";

    if (ecmp->MysteryVan) {
      content += "<h3>MysteryVan platform detected!</h3>";
      content += "<h4>Contactor State: ";
      if (ecmp->CONTACTORS_STATE == 0) {
        content += "Open";
      } else if (ecmp->CONTACTORS_STATE == 1) {
        content += "Precharge";
      } else if (ecmp->CONTACTORS_STATE == 2) {
        content += "Closed";
      }
      content += "</h4>";
      content += "<h4>Crash Memorized: ";
      if (ecmp->CrashMemorized) {
        content += "Yes</h4>";
      } else {
        content += "No</h4>";
      }
      content += "<h4>Contactor Opening Reason: ";
      if (ecmp->CONTACTOR_OPENING_REASON == 0) {
        content += "No error";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 1) {
        content += "Crash!";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 2) {
        content += "12V supply source undervoltage";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 3) {
        content += "12V supply source overvoltage";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 4) {
        content += "Battery temperature";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 5) {
        content += "Interlock line open";
      } else if (ecmp->CONTACTOR_OPENING_REASON == 6) {
        content += "e-Service plug disconnected";
      }
      content += "</h4>";
      content += "<h4>Battery fault type: ";
      if (ecmp->TBMU_FAULT_TYPE == 0) {
        content += "No fault";
      } else if (ecmp->TBMU_FAULT_TYPE == 1) {
        content += "FirstLevelFault: Warning Lamp";
      } else if (ecmp->TBMU_FAULT_TYPE == 2) {
        content += "SecondLevelFault: Stop Lamp";
      } else if (ecmp->TBMU_FAULT_TYPE == 3) {
        content += "ThirdLevelFault: Stop Lamp + contactor opening (EPS shutdown)";
      } else if (ecmp->TBMU_FAULT_TYPE == 4) {
        content += "FourthLevelFault: Stop Lamp + Active Discharge";
      } else if (ecmp->TBMU_FAULT_TYPE == 5) {
        content += "Inhibition of powertrain activation";
      } else if (ecmp->TBMU_FAULT_TYPE == 6) {
        content += "Reserved";
      }
      content += "</h4>";
      content += "<h4>FC insulation minus resistance " + String(ecmp->HV_BATT_FC_INSU_MINUS_RES) + " kOhm</h4>";
      content += "<h4>FC insulation plus resistance " + String(ecmp->HV_BATT_FC_INSU_PLUS_RES) + " kOhm</h4>";
      content +=
          "<h4>FC vehicle insulation plus resistance " + String(ecmp->HV_BATT_FC_VHL_INSU_PLUS_RES) + " kOhm</h4>";
      content +=
          "<h4>FC vehicle insulation plus resistance " + String(ecmp->HV_BATT_ONLY_INSU_MINUS_RES) + " kOhm</h4>";
    }
    content += "<h4>Alert Battery: ";
    if (ecmp->ALERT_BATT) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Low SOC: ";
    if (ecmp->ALERT_LOW_SOC) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert High SOC: ";
    if (ecmp->ALERT_HIGH_SOC) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert SOC Jump: ";
    if (ecmp->ALERT_SOC_JUMP) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Overcharge: ";
    if (ecmp->ALERT_OVERCHARGE) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Temp Diff: ";
    if (ecmp->ALERT_TEMP_DIFF) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Temp High: ";
    if (ecmp->ALERT_HIGH_TEMP) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Overvoltage: ";
    if (ecmp->ALERT_OVERVOLTAGE) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Cell Overvoltage: ";
    if (ecmp->ALERT_CELL_OVERVOLTAGE) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Cell Undervoltage: ";
    if (ecmp->ALERT_CELL_UNDERVOLTAGE) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Alert Cell Poor Consistency: ";
    if (ecmp->ALERT_CELL_POOR_CONSIST) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }
    content += "<h4>Remember to press Open Contactors from main menu before running the dianostic commands below:</h4>";
    return content;
  }

 private:
  DATALAYER_INFO_ECMP* ecmp;
};

#endif
