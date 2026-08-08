#ifndef _CELLPOWER_HTML_H
#define _CELLPOWER_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CellpowerHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    const String falseTrue[2] = {TR(TrKey::DRV_FALSE), TR(TrKey::DRV_TRUE)};
    tr_h3(content, TrKey::DRV_STATES);
    tr_h4_colon(content, TrKey::DRV_DISCHARGE, String(falseTrue[datalayer_extended.cellpower.system_state_discharge]));
    tr_h4_colon(content, TrKey::DRV_CHARGE, String(falseTrue[datalayer_extended.cellpower.system_state_charge]));
    tr_h4(content, TrKey::DRV_CELLBALANCING,
          String(falseTrue[datalayer_extended.cellpower.system_state_cellbalancing]));
    tr_h4(content, TrKey::DRV_TRICKLECHARGING,
          String(falseTrue[datalayer_extended.cellpower.system_state_tricklecharge]));
    tr_h4_colon(content, TrKey::DRV_IDLE, String(falseTrue[datalayer_extended.cellpower.system_state_idle]));
    tr_h4(content, TrKey::DRV_CHARGE_COMPLETED,
          String(falseTrue[datalayer_extended.cellpower.system_state_chargecompleted]));
    tr_h4(content, TrKey::DRV_MAINTENANCE_CHARGE,
          String(falseTrue[datalayer_extended.cellpower.system_state_maintenancecharge]));
    content += "<h3>IO:</h3>";
    tr_h4(content, TrKey::DRV_MAIN_POSITIVE_RELAY,
          String(falseTrue[datalayer_extended.cellpower.IO_state_main_positive_relay]));
    tr_h4(content, TrKey::DRV_MAIN_NEGATIVE_RELAY,
          String(falseTrue[datalayer_extended.cellpower.IO_state_main_negative_relay]));
    tr_h4(content, TrKey::DRV_CHARGE_ENABLED, String(falseTrue[datalayer_extended.cellpower.IO_state_charge_enable]));
    tr_h4(content, TrKey::DRV_PRECHARGE_RELAY,
          String(falseTrue[datalayer_extended.cellpower.IO_state_precharge_relay]));
    tr_h4(content, TrKey::DRV_DISCHARGE_ENABLE,
          String(falseTrue[datalayer_extended.cellpower.IO_state_discharge_enable]));
    content += "<h4>IO 6: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_6]) + "</h4>";
    content += "<h4>IO 7: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_7]) + "</h4>";
    content += "<h4>IO 8: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_8]) + "</h4>";
    tr_h3(content, TrKey::DRV_ERRORS);
    tr_h4_colon(content, TrKey::DRV_CELL_OVERVOLTAGE,
                String(falseTrue[datalayer_extended.cellpower.error_Cell_overvoltage]));
    tr_h4_colon(content, TrKey::DRV_CELL_UNDERVOLTAGE,
                String(falseTrue[datalayer_extended.cellpower.error_Cell_undervoltage]));
    tr_h4(content, TrKey::DRV_CELL_END_LIFE_VOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.error_Cell_end_of_life_voltage]));
    tr_h4(content, TrKey::DRV_CELL_VOLTAGE_MISREAD,
          String(falseTrue[datalayer_extended.cellpower.error_Cell_voltage_misread]));
    tr_h4(content, TrKey::DRV_CELL_OVER_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.error_Cell_over_temperature]));
    tr_h4(content, TrKey::DRV_CELL_UNDER_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.error_Cell_under_temperature]));
    tr_h4(content, TrKey::DRV_CELL_UNMANAGED, String(falseTrue[datalayer_extended.cellpower.error_Cell_unmanaged]));
    tr_h4(content, TrKey::DRV_LMU_OVER_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.error_LMU_over_temperature]));
    tr_h4(content, TrKey::DRV_LMU_UNDER_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.error_LMU_under_temperature]));
    tr_h4(content, TrKey::DRV_TEMP_SENSOR_OPEN_CIRCUIT,
          String(falseTrue[datalayer_extended.cellpower.error_Temp_sensor_open_circuit]));
    tr_h4(content, TrKey::DRV_TEMP_SENSOR_SHORT_CIRCUIT,
          String(falseTrue[datalayer_extended.cellpower.error_Temp_sensor_short_circuit]));
    tr_h4(content, TrKey::DRV_SUB_COMM, String(falseTrue[datalayer_extended.cellpower.error_SUB_communication]));
    tr_h4(content, TrKey::DRV_LMU_COMM, String(falseTrue[datalayer_extended.cellpower.error_LMU_communication]));
    tr_h4(content, TrKey::DRV_OVER_CURRENT, String(falseTrue[datalayer_extended.cellpower.error_Over_current_IN]));
    tr_h4(content, TrKey::DRV_OVER_CURRENT_OUT, String(falseTrue[datalayer_extended.cellpower.error_Over_current_OUT]));
    tr_h4(content, TrKey::DRV_SHORT_CIRCUIT, String(falseTrue[datalayer_extended.cellpower.error_Short_circuit]));
    tr_h4_colon(content, TrKey::DRV_LEAK_DETECTED, String(falseTrue[datalayer_extended.cellpower.error_Leak_detected]));
    tr_h4(content, TrKey::DRV_LEAK_DETECTION_FAILED,
          String(falseTrue[datalayer_extended.cellpower.error_Leak_detection_failed]));
    tr_h4(content, TrKey::DRV_VOLTAGE_DIFF, String(falseTrue[datalayer_extended.cellpower.error_Voltage_difference]));
    tr_h4(content, TrKey::DRV_BMCU_SUPPLY_OVERVOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.error_BMCU_supply_over_voltage]));
    tr_h4(content, TrKey::DRV_BMCU_SUPPLY_UNDERVOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.error_BMCU_supply_under_voltage]));
    tr_h4(content, TrKey::DRV_MAIN_POSITIVE_CONTACTOR,
          String(falseTrue[datalayer_extended.cellpower.error_Main_positive_contactor]));
    tr_h4(content, TrKey::DRV_MAIN_NEGATIVE_CONTACTOR,
          String(falseTrue[datalayer_extended.cellpower.error_Main_negative_contactor]));
    tr_h4(content, TrKey::DRV_PRECHARGE_CONTACTOR,
          String(falseTrue[datalayer_extended.cellpower.error_Precharge_contactor]));
    tr_h4(content, TrKey::DRV_MIDPACK_CONTACTOR,
          String(falseTrue[datalayer_extended.cellpower.error_Midpack_contactor]));
    tr_h4(content, TrKey::DRV_PRECHARGE_TIMEOUT,
          String(falseTrue[datalayer_extended.cellpower.error_Precharge_timeout]));
    tr_h4(content, TrKey::DRV_EMG_CONNECTOR_OVERRIDE,
          String(falseTrue[datalayer_extended.cellpower.error_Emergency_connector_override]));
    tr_h3(content, TrKey::DRV_WARNINGS);
    tr_h4(content, TrKey::DRV_HIGH_CELL_VOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_cell_voltage]));
    tr_h4(content, TrKey::DRV_LOW_CELL_VOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.warning_Low_cell_voltage]));
    tr_h4(content, TrKey::DRV_HIGH_CELL_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_cell_temperature]));
    tr_h4(content, TrKey::DRV_LOW_CELL_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.warning_Low_cell_temperature]));
    tr_h4(content, TrKey::DRV_HIGH_LMU_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_LMU_temperature]));
    tr_h4(content, TrKey::DRV_LOW_LMU_TEMPERATURE,
          String(falseTrue[datalayer_extended.cellpower.warning_Low_LMU_temperature]));
    tr_h4(content, TrKey::DRV_SUB_COMM_INTERF,
          String(falseTrue[datalayer_extended.cellpower.warning_SUB_communication_interfered]));
    tr_h4(content, TrKey::DRV_LMU_COMM_INTERF,
          String(falseTrue[datalayer_extended.cellpower.warning_LMU_communication_interfered]));
    tr_h4(content, TrKey::DRV_HIGH_CURRENT, String(falseTrue[datalayer_extended.cellpower.warning_High_current_IN]));
    tr_h4(content, TrKey::DRV_HIGH_CURRENT_OUT,
          String(falseTrue[datalayer_extended.cellpower.warning_High_current_OUT]));
    tr_h4(content, TrKey::DRV_PACK_RESISTANCE_DIFF,
          String(falseTrue[datalayer_extended.cellpower.warning_Pack_resistance_difference]));
    tr_h4(content, TrKey::DRV_HIGH_PACK_RESISTANCE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_pack_resistance]));
    tr_h4(content, TrKey::DRV_CELL_RESISTANCE_DIFF,
          String(falseTrue[datalayer_extended.cellpower.warning_Cell_resistance_difference]));
    tr_h4(content, TrKey::DRV_HIGH_CELL_RESISTANCE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_cell_resistance]));
    tr_h4(content, TrKey::DRV_HIGH_BMCU_SUPPLY_VOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.warning_High_BMCU_supply_voltage]));
    tr_h4(content, TrKey::DRV_LOW_BMCU_SUPPLY_VOLTAGE,
          String(falseTrue[datalayer_extended.cellpower.warning_Low_BMCU_supply_voltage]));
    tr_h4_colon(content, TrKey::DRV_LOW_SOC, String(falseTrue[datalayer_extended.cellpower.warning_Low_SOC]));
    tr_h4(content, TrKey::DRV_BALANCING_REQUIRED,
          String(falseTrue[datalayer_extended.cellpower.warning_Balancing_required_OCV_model]));
    tr_h4(content, TrKey::DRV_CHARGER_NOT_RESPONDING,
          String(falseTrue[datalayer_extended.cellpower.warning_Charger_not_responding]));

    return content;
  }
};

#endif
