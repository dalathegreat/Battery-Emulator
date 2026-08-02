#ifndef _ECMP_BATTERY_HTML_H
#define _ECMP_BATTERT_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class EcmpHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h4_open(content, TrKey::DRV_MAIN_CONNECTOR_STATE);
    if (datalayer_extended.stellantisECMP.MainConnectorState == 0) {
      tr_h4_end(content, TrKey::DRV_CONTACTORS_OPEN);
    } else if (datalayer_extended.stellantisECMP.MainConnectorState == 0x01) {
      tr_h4_end(content, TrKey::DRV_PRECHARGED);
    } else {
      tr_h4_end(content, TrKey::DRV_INVALID);
    }
    tr_h4(content, TrKey::DRV_INSULATION_RESISTANCE, String(datalayer_extended.stellantisECMP.InsulationResistance),
          "kOhm");
    content += "<h4>" + TR(TrKey::DRV_INTERLOCK) + ":  ";
    if (datalayer_extended.stellantisECMP.InterlockOpen == true) {
      content += "BROKEN!</h4>";
    } else {
      tr_h4_end(content, TrKey::DRV_SEATED_OK);
    }
    tr_h4_open(content, TrKey::DRV_INSULATION_DIAG);
    if (datalayer_extended.stellantisECMP.InsulationDiag == 0) {
      tr_h4_end(content, TrKey::DRV_NO_FAILURE);
    } else if (datalayer_extended.stellantisECMP.InsulationDiag == 1) {
      tr_h4_end(content, TrKey::DRV_SYMMETRIC_FAILURE);
    } else {  //4 Invalid, 5-7 illegal, wrap em under one text
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    }
    tr_h4_open(content, TrKey::DRV_CONTACTOR_WELD_CHECK);
    if (datalayer_extended.stellantisECMP.pid_welding_detection == 0) {
      tr_h4_end(content, TrKey::DRV_OK);
    } else if (datalayer_extended.stellantisECMP.pid_welding_detection == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {  //Problem
      content +=
          TR(TrKey::DRV_WELDED_ALARM) + String(datalayer_extended.stellantisECMP.pid_welding_detection) + "</h4>";
    }

    tr_h4_open(content, TrKey::DRV_CONTACTOR_OPENING_REASON);
    if (datalayer_extended.stellantisECMP.pid_reason_open == 7) {
      tr_h4_end(content, TrKey::DRV_INVALID_STATUS);
    } else if (datalayer_extended.stellantisECMP.pid_reason_open == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {  //Problem (Also status 0 might be OK?)
      content += TR(TrKey::UI_UNKNOWN) + String(datalayer_extended.stellantisECMP.pid_reason_open) + "</h4>";
    }

    tr_h4(content, TrKey::DRV_STATUS_POWER_SWITCH,
          (datalayer_extended.stellantisECMP.pid_contactor_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_contactor_status)));
    tr_h4(content, TrKey::DRV_NEGATIVE_POWER_SWITCH_CONTROL,
          (datalayer_extended.stellantisECMP.pid_negative_contactor_control == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_negative_contactor_control)));
    tr_h4(content, TrKey::DRV_NEGATIVE_POWER_SWITCH_STATUS,
          (datalayer_extended.stellantisECMP.pid_negative_contactor_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_negative_contactor_status)));
    tr_h4(content, TrKey::DRV_POSITIVE_POWER_SWITCH_CONTROL,
          (datalayer_extended.stellantisECMP.pid_positive_contactor_control == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_positive_contactor_control)));
    tr_h4(content, TrKey::DRV_POSITIVE_POWER_SWITCH_STATUS,
          (datalayer_extended.stellantisECMP.pid_positive_contactor_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_positive_contactor_status)));
    tr_h4(content, TrKey::DRV_CONTACTOR_NEGATIVE,
          (datalayer_extended.stellantisECMP.pid_contactor_negative == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_contactor_negative)));
    tr_h4(content, TrKey::DRV_CONTACTOR_POSITIVE,
          (datalayer_extended.stellantisECMP.pid_contactor_positive == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_contactor_positive)));
    tr_h4(content, TrKey::DRV_PRECHARGE_CONTROL,
          (datalayer_extended.stellantisECMP.pid_precharge_relay_control == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_precharge_relay_control)));
    tr_h4(content, TrKey::DRV_PRECHARGE_STATUS,
          (datalayer_extended.stellantisECMP.pid_precharge_relay_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_precharge_relay_status)));
    tr_h4(content, TrKey::DRV_RECHARGE_STATUS,
          (datalayer_extended.stellantisECMP.pid_recharge_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_recharge_status)));
    tr_h4(content, TrKey::DRV_DELTA_TEMPERATURE,
          (datalayer_extended.stellantisECMP.pid_delta_temperature == 127
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_delta_temperature)),
          "&deg;C");
    tr_h4(content, TrKey::DRV_LOWEST_TEMPERATURE,
          (datalayer_extended.stellantisECMP.pid_lowest_temperature == 127
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_lowest_temperature)),
          "&deg;C");
    tr_h4(content, TrKey::DRV_AVERAGE_TEMPERATURE,
          (datalayer_extended.stellantisECMP.pid_average_temperature == 127
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_average_temperature)),
          "&deg;C");
    tr_h4(content, TrKey::DRV_HIGHEST_TEMPERATURE,
          (datalayer_extended.stellantisECMP.pid_highest_temperature == 127
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_highest_temperature)),
          "&deg;C");
    tr_h4(content, TrKey::DRV_COLDEST_MODULE,
          (datalayer_extended.stellantisECMP.pid_coldest_module == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_coldest_module)));
    tr_h4(content, TrKey::DRV_HOTTEST_MODULE,
          (datalayer_extended.stellantisECMP.pid_hottest_module == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_hottest_module)));
    tr_h4(content, TrKey::DRV_AVERAGE_CELL_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_avg_cell_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_avg_cell_voltage)),
          " mV");
    tr_h4(
        content, TrKey::DRV_HIGH_PRECISION_CURRENT,
        (datalayer_extended.stellantisECMP.pid_current == 255 ? TR(TrKey::DRV_NOT_APPLICABLE)
                                                              : String(datalayer_extended.stellantisECMP.pid_current)),
        " mA");
    tr_h4(content, TrKey::DRV_INSULATION_RESISTANCE_NEG_GND,
          (datalayer_extended.stellantisECMP.pid_insulation_res_neg == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_insulation_res_neg)),
          " kOhm");
    tr_h4(content, TrKey::DRV_INSULATION_RESISTANCE_POS_GND,
          (datalayer_extended.stellantisECMP.pid_insulation_res_pos == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_insulation_res_pos)),
          " kOhm");
    tr_h4(content, TrKey::DRV_MAX_CURRENT_10S,
          (datalayer_extended.stellantisECMP.pid_max_current_10s == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_max_current_10s)));
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_POWER_10S,
          (datalayer_extended.stellantisECMP.pid_max_discharge_10s == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_max_discharge_10s)));
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_POWER_30S,
          (datalayer_extended.stellantisECMP.pid_max_discharge_30s == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_max_discharge_30s)));
    tr_h4(content, TrKey::DRV_MAX_CHARGE_POWER_10S,
          (datalayer_extended.stellantisECMP.pid_max_charge_10s == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_max_charge_10s)));
    tr_h4(content, TrKey::DRV_MAX_CHARGE_POWER_30S,
          (datalayer_extended.stellantisECMP.pid_max_charge_30s == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_max_charge_30s)));
    tr_h4(content, TrKey::DRV_ENERGY_CAPACITY,
          (datalayer_extended.stellantisECMP.pid_energy_capacity == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_energy_capacity)));
    tr_h4(content, TrKey::DRV_HIGHEST_CELL_NUMBER,
          (datalayer_extended.stellantisECMP.pid_highest_cell_voltage_num == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_highest_cell_voltage_num)));
    tr_h4(content, TrKey::DRV_LOWEST_CELL_VOLTAGE_NUMBER,
          (datalayer_extended.stellantisECMP.pid_lowest_cell_voltage_num == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_lowest_cell_voltage_num)));
    tr_h4(content, TrKey::DRV_SUM_ALL_CELL_VOLTAGES,
          (datalayer_extended.stellantisECMP.pid_sum_of_cells == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_sum_of_cells)),
          " dV");
    tr_h4(content, TrKey::DRV_CELL_MIN_CAPACITY,
          (datalayer_extended.stellantisECMP.pid_cell_min_capacity == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_cell_min_capacity)));
    tr_h4(content, TrKey::DRV_CELL_VOLTAGE_MEASUREMENT_STATUS,
          (datalayer_extended.stellantisECMP.pid_cell_voltage_measurement_status == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_cell_voltage_measurement_status)));
    tr_h4(content, TrKey::DRV_BATTERY_INSULATION_RESISTANCE,
          (datalayer_extended.stellantisECMP.pid_insulation_res == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_insulation_res)),
          " kOhm");
    tr_h4(content, TrKey::DRV_PACK_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_pack_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_pack_voltage)),
          " dV");
    tr_h4(content, TrKey::DRV_HIGHEST_CELL_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_high_cell_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_high_cell_voltage)),
          " mV");
    tr_h4(content, TrKey::DRV_LOWEST_CELL_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_low_cell_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_low_cell_voltage)),
          " mV");
    tr_h4(content, TrKey::DRV_BATTERY_ENERGY,
          (datalayer_extended.stellantisECMP.pid_battery_energy == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_battery_energy)));
    tr_h4(content, TrKey::DRV_COLLISION_INFORMATION_COUNTER,
          (datalayer_extended.stellantisECMP.pid_crash_counter == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_crash_counter)));
    tr_h4(content, TrKey::DRV_COLLISION_COUNTER_RECIEVED_BY_WIRE,
          (datalayer_extended.stellantisECMP.pid_wire_crash == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_wire_crash)));
    tr_h4(content, TrKey::DRV_COLLISION_DATA_SENT_FROM_CAR_BATTERY,
          (datalayer_extended.stellantisECMP.pid_CAN_crash == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_CAN_crash)));
    tr_h4(content, TrKey::DRV_HISTORY_DATA,
          (datalayer_extended.stellantisECMP.pid_history_data == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_history_data)));
    tr_h4(content, TrKey::DRV_LOW_SOC_COUNTER,
          (datalayer_extended.stellantisECMP.pid_lowsoc_counter == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_lowsoc_counter)));
    tr_h4(content, TrKey::DRV_LAST_CAN_FAILURE_DETAIL,
          (datalayer_extended.stellantisECMP.pid_last_can_failure_detail == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_last_can_failure_detail)));
    tr_h4(content, TrKey::DRV_HW_VERSION_NUMBER,
          (datalayer_extended.stellantisECMP.pid_hw_version_num == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_hw_version_num)));
    tr_h4(content, TrKey::DRV_SW_VERSION_NUMBER,
          (datalayer_extended.stellantisECMP.pid_sw_version_num == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_sw_version_num)));
    tr_h4(content, TrKey::DRV_FACTORY_MODE,
          (datalayer_extended.stellantisECMP.pid_factory_mode_control == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_factory_mode_control)));
    char readableSerialNumber[14];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.stellantisECMP.pid_battery_serial,
           sizeof(datalayer_extended.stellantisECMP.pid_battery_serial));
    readableSerialNumber[13] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_BATTERY_SERIAL, String(readableSerialNumber));
    uint8_t day = (datalayer_extended.stellantisECMP.pid_date_of_manufacture >> 16) & 0xFF;
    uint8_t month = (datalayer_extended.stellantisECMP.pid_date_of_manufacture >> 8) & 0xFF;
    uint8_t year = datalayer_extended.stellantisECMP.pid_date_of_manufacture & 0xFF;
    tr_h4(content, TrKey::DRV_DATE_MANUFACTURE, String(day) + "/" + String(month) + "/" + String(year));
    tr_h4(content, TrKey::DRV_AUX_FUSE_STATE,
          (datalayer_extended.stellantisECMP.pid_aux_fuse_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_aux_fuse_state)));
    tr_h4(content, TrKey::DRV_BATTERY_STATE,
          (datalayer_extended.stellantisECMP.pid_battery_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_battery_state)));
    tr_h4(content, TrKey::DRV_PRECHARGE_SHORT_CIRCUIT,
          (datalayer_extended.stellantisECMP.pid_precharge_short_circuit == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_precharge_short_circuit)));
    tr_h4(content, TrKey::DRV_SERVICE_PLUG_STATE,
          (datalayer_extended.stellantisECMP.pid_eservice_plug_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_eservice_plug_state)));
    tr_h4(content, TrKey::DRV_MAIN_FUSE_STATE,
          (datalayer_extended.stellantisECMP.pid_mainfuse_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_mainfuse_state)));
    tr_h4(content, TrKey::DRV_MOST_CRITICAL_FAULT,
          (datalayer_extended.stellantisECMP.pid_most_critical_fault == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_most_critical_fault)));
    tr_h4(content, TrKey::DRV_CURRENT_TIME,
          (datalayer_extended.stellantisECMP.pid_current_time == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_current_time)),
          " ticks");
    tr_h4(content, TrKey::DRV_TIME_SENT_BY_CAR,
          (datalayer_extended.stellantisECMP.pid_time_sent_by_car == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_time_sent_by_car)),
          " ticks");
    tr_h4_start(content, TrKey::DRV_12V);
    content += ": " +
               (datalayer_extended.stellantisECMP.pid_12v == 255 ? TR(TrKey::DRV_NOT_APPLICABLE)
                                                                 : String(datalayer_extended.stellantisECMP.pid_12v)) +
               "</h4>";
    tr_h4_open(content, TrKey::DRV_12V_ABNORMAL);
    if (datalayer_extended.stellantisECMP.pid_12v_abnormal == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else if (datalayer_extended.stellantisECMP.pid_12v_abnormal == 0) {
      tr_h4_end(content, TrKey::DRV_NO);
    } else {
      tr_h4_end(content, TrKey::DRV_YES);
    }
    tr_h4(content, TrKey::DRV_HVIL_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_hvil_in_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_hvil_in_voltage)),
          "mV");
    tr_h4(content, TrKey::DRV_HVIL_OUT_VOLTAGE,
          (datalayer_extended.stellantisECMP.pid_hvil_out_voltage == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_hvil_out_voltage)),
          "mV");
    tr_h4(content, TrKey::DRV_HVIL_STATE,
          (datalayer_extended.stellantisECMP.pid_hvil_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : (datalayer_extended.stellantisECMP.pid_hvil_state == 0
                      ? "OK"
                      : String(datalayer_extended.stellantisECMP.pid_hvil_state))));
    tr_h4(content, TrKey::DRV_BMS_STATE,
          (datalayer_extended.stellantisECMP.pid_bms_state == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : (datalayer_extended.stellantisECMP.pid_bms_state == 0
                      ? "OK"
                      : String(datalayer_extended.stellantisECMP.pid_bms_state))));
    tr_h4(content, TrKey::DRV_VEHICLE_SPEED,
          (datalayer_extended.stellantisECMP.pid_vehicle_speed == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_vehicle_speed)),
          " km/h");
    tr_h4(content, TrKey::DRV_TIME_SPENT_OVER_55C,
          (datalayer_extended.stellantisECMP.pid_time_spent_over_55c == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_time_spent_over_55c)),
          " minutes");
    tr_h4(content, TrKey::DRV_CONTACTOR_LIFETIME_CLOSING_COUNTER,
          (datalayer_extended.stellantisECMP.pid_contactor_closing_counter == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_contactor_closing_counter)),
          " cycles");
    tr_h4(content, TrKey::DRV_STATE_HEALTH_CELL_1,
          (datalayer_extended.stellantisECMP.pid_SOH_cell_1 == 255
               ? TR(TrKey::DRV_NOT_APPLICABLE)
               : String(datalayer_extended.stellantisECMP.pid_SOH_cell_1)));

    if (datalayer_extended.stellantisECMP.MysteryVan) {
      tr_h3(content, TrKey::DRV_MYSTERYVAN_PLATFORM_DETECTED);
      tr_h4_open(content, TrKey::DRV_CONTACTOR_STATUS);
      if (datalayer_extended.stellantisECMP.CONTACTORS_STATE == 0) {
        content += TR(TrKey::DRV_OPEN);
      } else if (datalayer_extended.stellantisECMP.CONTACTORS_STATE == 1) {
        content += TR(TrKey::DRV_PRECHARGE);
      } else if (datalayer_extended.stellantisECMP.CONTACTORS_STATE == 2) {
        content += TR(TrKey::DRV_CLOSED);
      }
      content += "</h4>";
      tr_h4_open(content, TrKey::DRV_CRASH_MEMORIZED);
      if (datalayer_extended.stellantisECMP.CrashMemorized) {
        tr_h4_end(content, TrKey::DRV_YES);
      } else {
        tr_h4_end(content, TrKey::DRV_NO);
      }
      tr_h4_open(content, TrKey::DRV_CONTACTOR_OPENING_REASON);
      if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 0) {
        content += TR(TrKey::DRV_NO_ERROR);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 1) {
        content += TR(TrKey::DRV_CRASH);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 2) {
        content += TR(TrKey::DRV_12V_SUPPLY_SOURCE_UNDERVOLTAGE);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 3) {
        content += TR(TrKey::DRV_12V_SUPPLY_SOURCE_OVERVOLTAGE);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 4) {
        content += TR(TrKey::DRV_BATTERY_TEMPERATURE);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 5) {
        content += TR(TrKey::DRV_INTERLOCK_LINE_OPEN);
      } else if (datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON == 6) {
        content += TR(TrKey::DRV_E_SERVICE_PLUG_DISCONNECTED);
      }
      content += "</h4>";
      tr_h4_open(content, TrKey::DRV_BATTERY_FAULT_TYPE);
      if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 0) {
        content += TR(TrKey::DRV_NO_FAULT);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 1) {
        content += TR(TrKey::DRV_FIRSTLEVELFAULT_WARNING_LAMP);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 2) {
        content += TR(TrKey::DRV_SECONDLEVELFAULT_STOP_LAMP);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 3) {
        content += TR(TrKey::DRV_THIRDLEVELFAULT_STOP_LAMP_CONTACTOR_OPENING_EPS_SHUTDOWN);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 4) {
        content += TR(TrKey::DRV_FOURTHLEVELFAULT_STOP_LAMP_ACTIVE_DISCHARGE);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 5) {
        content += TR(TrKey::DRV_INHIBITION_POWERTRAIN_ACTIVATION);
      } else if (datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE == 6) {
        content += TR(TrKey::DRV_RESERVED);
      }
      content += "</h4>";
      tr_h4(content, TrKey::DRV_FC_INSULATION_MINUS_RESISTANCE,
            String(datalayer_extended.stellantisECMP.HV_BATT_FC_INSU_MINUS_RES), " kOhm");
      tr_h4(content, TrKey::DRV_FC_INSULATION_PLUS_RESISTANCE,
            String(datalayer_extended.stellantisECMP.HV_BATT_FC_INSU_PLUS_RES), " kOhm");
      tr_h4(content, TrKey::DRV_FC_VEHICLE_INSULATION_PLUS_RESISTANCE,
            String(datalayer_extended.stellantisECMP.HV_BATT_FC_VHL_INSU_PLUS_RES), " kOhm");
      tr_h4(content, TrKey::DRV_FC_VEHICLE_INSULATION_PLUS_RESISTANCE,
            String(datalayer_extended.stellantisECMP.HV_BATT_ONLY_INSU_MINUS_RES), " kOhm");
    }
    tr_h4_open(content, TrKey::DRV_ALERT_BATTERY);
    if (datalayer_extended.stellantisECMP.ALERT_BATT) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_LOW_SOC);
    if (datalayer_extended.stellantisECMP.ALERT_LOW_SOC) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_HIGH_SOC);
    if (datalayer_extended.stellantisECMP.ALERT_HIGH_SOC) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_SOC_JUMP);
    if (datalayer_extended.stellantisECMP.ALERT_SOC_JUMP) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_OVERCHARGE);
    if (datalayer_extended.stellantisECMP.ALERT_OVERCHARGE) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_TEMP_DIFF);
    if (datalayer_extended.stellantisECMP.ALERT_TEMP_DIFF) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_TEMP_HIGH);
    if (datalayer_extended.stellantisECMP.ALERT_HIGH_TEMP) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_OVERVOLTAGE);
    if (datalayer_extended.stellantisECMP.ALERT_OVERVOLTAGE) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_CELL_OVERVOLTAGE);
    if (datalayer_extended.stellantisECMP.ALERT_CELL_OVERVOLTAGE) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_CELL_UNDERVOLTAGE);
    if (datalayer_extended.stellantisECMP.ALERT_CELL_UNDERVOLTAGE) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4_open(content, TrKey::DRV_ALERT_CELL_POOR_CONSISTENCY);
    if (datalayer_extended.stellantisECMP.ALERT_CELL_POOR_CONSIST) {
      tr_h4_end(content, TrKey::DRV_YES);
    } else {
      tr_h4_end(content, TrKey::DRV_NO);
    }
    tr_h4(content, TrKey::DRV_REMEMBER_PRESS_OPEN_CONTACTORS_FROM_MAIN_MENU_BEFORE_RUNNING_DIANOSTIC_COMMANDS_BELOW);
    return content;
  }
};

#endif
