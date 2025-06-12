#ifndef _ECMP_BATTERY_HTML_H
#define _ECMP_BATTERT_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class EcmpHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>Main Connector State: ";
    if (datalayer_extended.stellantisECMP.MainConnectorState == 0) {
      content += "Contactors open</h4>";
    } else if (datalayer_extended.stellantisECMP.MainConnectorState == 0x01) {
      content += "Precharged</h4>";
    } else {
      content += "Invalid</h4>";
    }
    content +=
        "<h4>Insulation Resistance: " + String(datalayer_extended.stellantisECMP.InsulationResistance) + "kOhm</h4>";
    content += "<h4>Interlock:  ";
    if (datalayer_extended.stellantisECMP.InterlockOpen == true) {
      content += "BROKEN!</h4>";
    } else {
      content += "Seated OK</h4>";
    }
    content += "<h4>Insulation Diag: ";
    if (datalayer_extended.stellantisECMP.InsulationDiag == 0) {
      content += "No failure</h4>";
    } else if (datalayer_extended.stellantisECMP.InsulationDiag == 1) {
      content += "Symmetric failure</h4>";
    } else {  //4 Invalid, 5-7 illegal, wrap em under one text
      content += "N/A</h4>";
    }
    content += "<h4>Contactor weld check: ";
    if (datalayer_extended.stellantisECMP.pid_welding_detection == 0) {
      content += "OK</h4>";
    } else if (datalayer_extended.stellantisECMP.pid_welding_detection == 255) {
      content += "N/A</h4>";
    } else {  //Problem
      content += "WELDED!" + String(datalayer_extended.stellantisECMP.pid_welding_detection) + "</h4>";
    }

    content += "<h4>Contactor opening reason: ";
    if (datalayer_extended.stellantisECMP.pid_reason_open == 7) {
      content += "Invalid Status</h4>";
    } else if (datalayer_extended.stellantisECMP.pid_reason_open == 255) {
      content += "N/A</h4>";
    } else {  //Problem (Also status 0 might be OK?)
      content += "Unknown" + String(datalayer_extended.stellantisECMP.pid_reason_open) + "</h4>";
    }

    content += "<h4>Status of power switch: " +
               (datalayer_extended.stellantisECMP.pid_contactor_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_contactor_status)) +
               "</h4>";
    content += "<h4>Negative power switch control: " +
               (datalayer_extended.stellantisECMP.pid_negative_contactor_control == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_negative_contactor_control)) +
               "</h4>";
    content += "<h4>Negative power switch status: " +
               (datalayer_extended.stellantisECMP.pid_negative_contactor_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_negative_contactor_status)) +
               "</h4>";
    content += "<h4>Positive power switch control: " +
               (datalayer_extended.stellantisECMP.pid_positive_contactor_control == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_positive_contactor_control)) +
               "</h4>";
    content += "<h4>Positive power switch status: " +
               (datalayer_extended.stellantisECMP.pid_positive_contactor_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_positive_contactor_status)) +
               "</h4>";
    content += "<h4>Contactor negative: " +
               (datalayer_extended.stellantisECMP.pid_contactor_negative == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_contactor_negative)) +
               "</h4>";
    content += "<h4>Contactor positive: " +
               (datalayer_extended.stellantisECMP.pid_contactor_positive == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_contactor_positive)) +
               "</h4>";
    content += "<h4>Precharge control: " +
               (datalayer_extended.stellantisECMP.pid_precharge_relay_control == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_precharge_relay_control)) +
               "</h4>";
    content += "<h4>Precharge status: " +
               (datalayer_extended.stellantisECMP.pid_precharge_relay_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_precharge_relay_status)) +
               "</h4>";
    content += "<h4>Recharge Status: " +
               (datalayer_extended.stellantisECMP.pid_recharge_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_recharge_status)) +
               "</h4>";
    content += "<h4>Delta temperature: " +
               (datalayer_extended.stellantisECMP.pid_delta_temperature == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_delta_temperature)) +
               "&deg;C</h4>";
    content += "<h4>Lowest temperature: " +
               (datalayer_extended.stellantisECMP.pid_lowest_temperature == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_lowest_temperature)) +
               "&deg;C</h4>";
    content += "<h4>Average temperature: " +
               (datalayer_extended.stellantisECMP.pid_average_temperature == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_average_temperature)) +
               "&deg;C</h4>";
    content += "<h4>Highest temperature: " +
               (datalayer_extended.stellantisECMP.pid_highest_temperature == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_highest_temperature)) +
               "&deg;C</h4>";
    content += "<h4>Coldest module: " +
               (datalayer_extended.stellantisECMP.pid_coldest_module == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_coldest_module)) +
               "</h4>";
    content += "<h4>Hottest module: " +
               (datalayer_extended.stellantisECMP.pid_hottest_module == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_hottest_module)) +
               "</h4>";
    content += "<h4>Average cell voltage: " +
               (datalayer_extended.stellantisECMP.pid_avg_cell_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_avg_cell_voltage)) +
               " mV</h4>";
    content +=
        "<h4>High precision current: " +
        (datalayer_extended.stellantisECMP.pid_current == 255 ? "N/A"
                                                              : String(datalayer_extended.stellantisECMP.pid_current)) +
        " mA</h4>";
    content += "<h4>Insulation resistance neg-gnd: " +
               (datalayer_extended.stellantisECMP.pid_insulation_res_neg == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_insulation_res_neg)) +
               " kOhm</h4>";
    content += "<h4>Insulation resistance pos-gnd: " +
               (datalayer_extended.stellantisECMP.pid_insulation_res_pos == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_insulation_res_pos)) +
               " kOhm</h4>";
    content += "<h4>Max current 10s: " +
               (datalayer_extended.stellantisECMP.pid_max_current_10s == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_max_current_10s)) +
               "</h4>";
    content += "<h4>Max discharge power 10s: " +
               (datalayer_extended.stellantisECMP.pid_max_discharge_10s == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_max_discharge_10s)) +
               "</h4>";
    content += "<h4>Max discharge power 30s: " +
               (datalayer_extended.stellantisECMP.pid_max_discharge_30s == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_max_discharge_30s)) +
               "</h4>";
    content += "<h4>Max charge power 10s: " +
               (datalayer_extended.stellantisECMP.pid_max_charge_10s == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_max_charge_10s)) +
               "</h4>";
    content += "<h4>Max charge power 30s: " +
               (datalayer_extended.stellantisECMP.pid_max_charge_30s == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_max_charge_30s)) +
               "</h4>";
    content += "<h4>Energy capacity: " +
               (datalayer_extended.stellantisECMP.pid_energy_capacity == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_energy_capacity)) +
               "</h4>";
    content += "<h4>Highest cell number: " +
               (datalayer_extended.stellantisECMP.pid_highest_cell_voltage_num == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_highest_cell_voltage_num)) +
               "</h4>";
    content += "<h4>Lowest cell voltage number: " +
               (datalayer_extended.stellantisECMP.pid_lowest_cell_voltage_num == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_lowest_cell_voltage_num)) +
               "</h4>";
    content += "<h4>Sum of all cell voltages: " +
               (datalayer_extended.stellantisECMP.pid_sum_of_cells == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_sum_of_cells)) +
               " dV</h4>";
    content += "<h4>Cell min capacity: " +
               (datalayer_extended.stellantisECMP.pid_cell_min_capacity == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_cell_min_capacity)) +
               "</h4>";
    content += "<h4>Cell voltage measurement status: " +
               (datalayer_extended.stellantisECMP.pid_cell_voltage_measurement_status == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_cell_voltage_measurement_status)) +
               "</h4>";
    content += "<h4>Battery Insulation Resistance: " +
               (datalayer_extended.stellantisECMP.pid_insulation_res == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_insulation_res)) +
               " kOhm</h4>";
    content += "<h4>Pack voltage: " +
               (datalayer_extended.stellantisECMP.pid_pack_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_pack_voltage)) +
               " dV</h4>";
    content += "<h4>Highest cell voltage: " +
               (datalayer_extended.stellantisECMP.pid_high_cell_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_high_cell_voltage)) +
               " mV</h4>";
    content += "<h4>Lowest cell voltage: " +
               (datalayer_extended.stellantisECMP.pid_low_cell_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_low_cell_voltage)) +
               " mV</h4>";
    content += "<h4>Battery Energy: " +
               (datalayer_extended.stellantisECMP.pid_battery_energy == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_battery_energy)) +
               "</h4>";
    content += "<h4>Collision information Counter: " +
               (datalayer_extended.stellantisECMP.pid_crash_counter == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_crash_counter)) +
               "</h4>";
    content += "<h4>Collision Counter recieved by Wire: " +
               (datalayer_extended.stellantisECMP.pid_wire_crash == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_wire_crash)) +
               "</h4>";
    content += "<h4>Collision data sent from car to battery: " +
               (datalayer_extended.stellantisECMP.pid_CAN_crash == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_CAN_crash)) +
               "</h4>";
    content += "<h4>History data: " +
               (datalayer_extended.stellantisECMP.pid_history_data == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_history_data)) +
               "</h4>";
    content += "<h4>Low SOC counter: " +
               (datalayer_extended.stellantisECMP.pid_lowsoc_counter == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_lowsoc_counter)) +
               "</h4>";
    content += "<h4>Last CAN failure detail: " +
               (datalayer_extended.stellantisECMP.pid_last_can_failure_detail == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_last_can_failure_detail)) +
               "</h4>";
    content += "<h4>HW version number: " +
               (datalayer_extended.stellantisECMP.pid_hw_version_num == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_hw_version_num)) +
               "</h4>";
    content += "<h4>SW version number: " +
               (datalayer_extended.stellantisECMP.pid_sw_version_num == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_sw_version_num)) +
               "</h4>";
    content += "<h4>Factory mode: " +
               (datalayer_extended.stellantisECMP.pid_factory_mode_control == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_factory_mode_control)) +
               "</h4>";
    char readableSerialNumber[14];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.stellantisECMP.pid_battery_serial,
           sizeof(datalayer_extended.stellantisECMP.pid_battery_serial));
    readableSerialNumber[13] = '\0';  // Null terminate the string
    content += "<h4>Battery serial: " + String(readableSerialNumber) + "</h4>";
    uint8_t day = (datalayer_extended.stellantisECMP.pid_date_of_manufacture >> 16) & 0xFF;
    uint8_t month = (datalayer_extended.stellantisECMP.pid_date_of_manufacture >> 8) & 0xFF;
    uint8_t year = datalayer_extended.stellantisECMP.pid_date_of_manufacture & 0xFF;
    content += "<h4>Date of manufacture: " + String(day) + "/" + String(month) + "/" + String(year) + "</h4>";
    content += "<h4>Aux fuse state: " +
               (datalayer_extended.stellantisECMP.pid_aux_fuse_state == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_aux_fuse_state)) +
               "</h4>";
    content += "<h4>Battery state: " +
               (datalayer_extended.stellantisECMP.pid_battery_state == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_battery_state)) +
               "</h4>";
    content += "<h4>Precharge short circuit: " +
               (datalayer_extended.stellantisECMP.pid_precharge_short_circuit == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_precharge_short_circuit)) +
               "</h4>";
    content += "<h4>Service plug state: " +
               (datalayer_extended.stellantisECMP.pid_eservice_plug_state == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_eservice_plug_state)) +
               "</h4>";
    content += "<h4>Main fuse state: " +
               (datalayer_extended.stellantisECMP.pid_mainfuse_state == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_mainfuse_state)) +
               "</h4>";
    content += "<h4>Most critical fault: " +
               (datalayer_extended.stellantisECMP.pid_most_critical_fault == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_most_critical_fault)) +
               "</h4>";
    content += "<h4>Current time: " +
               (datalayer_extended.stellantisECMP.pid_current_time == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_current_time)) +
               " ticks</h4>";
    content += "<h4>Time sent by car: " +
               (datalayer_extended.stellantisECMP.pid_time_sent_by_car == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_time_sent_by_car)) +
               " ticks</h4>";
    content +=
        "<h4>12V: " +
        (datalayer_extended.stellantisECMP.pid_12v == 255 ? "N/A" : String(datalayer_extended.stellantisECMP.pid_12v)) +
        "</h4>";
    content += "<h4>12V abnormal: ";
    if (datalayer_extended.stellantisECMP.pid_12v_abnormal == 255) {
      content += "N/A</h4>";
    } else if (datalayer_extended.stellantisECMP.pid_12v_abnormal == 0) {
      content += "No</h4>";
    } else {
      content += "Yes</h4>";
    }
    content += "<h4>HVIL IN Voltage: " +
               (datalayer_extended.stellantisECMP.pid_hvil_in_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_hvil_in_voltage)) +
               "mV</h4>";
    content += "<h4>HVIL Out Voltage: " +
               (datalayer_extended.stellantisECMP.pid_hvil_out_voltage == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_hvil_out_voltage)) +
               "mV</h4>";
    content += "<h4>HVIL State: " +
               (datalayer_extended.stellantisECMP.pid_hvil_state == 255
                    ? "N/A"
                    : (datalayer_extended.stellantisECMP.pid_hvil_state == 0
                           ? "OK"
                           : String(datalayer_extended.stellantisECMP.pid_hvil_state))) +
               "</h4>";
    content += "<h4>BMS State: " +
               (datalayer_extended.stellantisECMP.pid_bms_state == 255
                    ? "N/A"
                    : (datalayer_extended.stellantisECMP.pid_bms_state == 0
                           ? "OK"
                           : String(datalayer_extended.stellantisECMP.pid_bms_state))) +
               "</h4>";
    content += "<h4>Vehicle speed: " +
               (datalayer_extended.stellantisECMP.pid_vehicle_speed == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_vehicle_speed)) +
               " km/h</h4>";
    content += "<h4>Time spent over 55c: " +
               (datalayer_extended.stellantisECMP.pid_time_spent_over_55c == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_time_spent_over_55c)) +
               " minutes</h4>";
    content += "<h4>Contactor lifetime closing counter: " +
               (datalayer_extended.stellantisECMP.pid_contactor_closing_counter == 255
                    ? "N/A"
                    : String(datalayer_extended.stellantisECMP.pid_contactor_closing_counter)) +
               " cycles</h4>";

    return content;
  }
};

#endif
