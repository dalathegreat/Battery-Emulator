export const DATALAYER_INFO_CELLPOWER_FIELDS: ([string, string] | [string, string, number])[] = [
  ['system_state_discharge', 'b'],
  ['system_state_charge', 'b'],
  ['system_state_cellbalancing', 'b'],
  ['system_state_tricklecharge', 'b'],
  ['system_state_idle', 'b'],
  ['system_state_chargecompleted', 'b'],
  ['system_state_maintenancecharge', 'b'],
  ['IO_state_main_positive_relay', 'b'],
  ['IO_state_main_negative_relay', 'b'],
  ['IO_state_charge_enable', 'b'],
  ['IO_state_precharge_relay', 'b'],
  ['IO_state_discharge_enable', 'b'],
  ['IO_state_IO_6', 'b'],
  ['IO_state_IO_7', 'b'],
  ['IO_state_IO_8', 'b'],
  ['error_Cell_overvoltage', 'b'],
  ['error_Cell_undervoltage', 'b'],
  ['error_Cell_end_of_life_voltage', 'b'],
  ['error_Cell_voltage_misread', 'b'],
  ['error_Cell_over_temperature', 'b'],
  ['error_Cell_under_temperature', 'b'],
  ['error_Cell_unmanaged', 'b'],
  ['error_LMU_over_temperature', 'b'],
  ['error_LMU_under_temperature', 'b'],
  ['error_Temp_sensor_open_circuit', 'b'],
  ['error_Temp_sensor_short_circuit', 'b'],
  ['error_SUB_communication', 'b'],
  ['error_LMU_communication', 'b'],
  ['error_Over_current_IN', 'b'],
  ['error_Over_current_OUT', 'b'],
  ['error_Short_circuit', 'b'],
  ['error_Leak_detected', 'b'],
  ['error_Leak_detection_failed', 'b'],
  ['error_Voltage_difference', 'b'],
  ['error_BMCU_supply_over_voltage', 'b'],
  ['error_BMCU_supply_under_voltage', 'b'],
  ['error_Main_positive_contactor', 'b'],
  ['error_Main_negative_contactor', 'b'],
  ['error_Precharge_contactor', 'b'],
  ['error_Midpack_contactor', 'b'],
  ['error_Precharge_timeout', 'b'],
  ['error_Emergency_connector_override', 'b'],
  ['warning_High_cell_voltage', 'b'],
  ['warning_Low_cell_voltage', 'b'],
  ['warning_High_cell_temperature', 'b'],
  ['warning_Low_cell_temperature', 'b'],
  ['warning_High_LMU_temperature', 'b'],
  ['warning_Low_LMU_temperature', 'b'],
  ['warning_SUB_communication_interfered', 'b'],
  ['warning_LMU_communication_interfered', 'b'],
  ['warning_High_current_IN', 'b'],
  ['warning_High_current_OUT', 'b'],
  ['warning_Pack_resistance_difference', 'b'],
  ['warning_High_pack_resistance', 'b'],
  ['warning_Cell_resistance_difference', 'b'],
  ['warning_High_cell_resistance', 'b'],
  ['warning_High_BMCU_supply_voltage', 'b'],
  ['warning_Low_BMCU_supply_voltage', 'b'],
  ['warning_Low_SOC', 'b'],
  ['warning_Balancing_required_OCV_model', 'b'],
  ['warning_Charger_not_responding', 'b'],
];

export const system_state_discharge: [number, string] = [0, 'b'];
export const system_state_charge: [number, string] = [1, 'b'];
export const system_state_cellbalancing: [number, string] = [2, 'b'];
export const system_state_tricklecharge: [number, string] = [3, 'b'];
export const system_state_idle: [number, string] = [4, 'b'];
export const system_state_chargecompleted: [number, string] = [5, 'b'];
export const system_state_maintenancecharge: [number, string] = [6, 'b'];
export const IO_state_main_positive_relay: [number, string] = [7, 'b'];
export const IO_state_main_negative_relay: [number, string] = [8, 'b'];
export const IO_state_charge_enable: [number, string] = [9, 'b'];
export const IO_state_precharge_relay: [number, string] = [10, 'b'];
export const IO_state_discharge_enable: [number, string] = [11, 'b'];
export const IO_state_IO_6: [number, string] = [12, 'b'];
export const IO_state_IO_7: [number, string] = [13, 'b'];
export const IO_state_IO_8: [number, string] = [14, 'b'];
export const error_Cell_overvoltage: [number, string] = [15, 'b'];
export const error_Cell_undervoltage: [number, string] = [16, 'b'];
export const error_Cell_end_of_life_voltage: [number, string] = [17, 'b'];
export const error_Cell_voltage_misread: [number, string] = [18, 'b'];
export const error_Cell_over_temperature: [number, string] = [19, 'b'];
export const error_Cell_under_temperature: [number, string] = [20, 'b'];
export const error_Cell_unmanaged: [number, string] = [21, 'b'];
export const error_LMU_over_temperature: [number, string] = [22, 'b'];
export const error_LMU_under_temperature: [number, string] = [23, 'b'];
export const error_Temp_sensor_open_circuit: [number, string] = [24, 'b'];
export const error_Temp_sensor_short_circuit: [number, string] = [25, 'b'];
export const error_SUB_communication: [number, string] = [26, 'b'];
export const error_LMU_communication: [number, string] = [27, 'b'];
export const error_Over_current_IN: [number, string] = [28, 'b'];
export const error_Over_current_OUT: [number, string] = [29, 'b'];
export const error_Short_circuit: [number, string] = [30, 'b'];
export const error_Leak_detected: [number, string] = [31, 'b'];
export const error_Leak_detection_failed: [number, string] = [32, 'b'];
export const error_Voltage_difference: [number, string] = [33, 'b'];
export const error_BMCU_supply_over_voltage: [number, string] = [34, 'b'];
export const error_BMCU_supply_under_voltage: [number, string] = [35, 'b'];
export const error_Main_positive_contactor: [number, string] = [36, 'b'];
export const error_Main_negative_contactor: [number, string] = [37, 'b'];
export const error_Precharge_contactor: [number, string] = [38, 'b'];
export const error_Midpack_contactor: [number, string] = [39, 'b'];
export const error_Precharge_timeout: [number, string] = [40, 'b'];
export const error_Emergency_connector_override: [number, string] = [41, 'b'];
export const warning_High_cell_voltage: [number, string] = [42, 'b'];
export const warning_Low_cell_voltage: [number, string] = [43, 'b'];
export const warning_High_cell_temperature: [number, string] = [44, 'b'];
export const warning_Low_cell_temperature: [number, string] = [45, 'b'];
export const warning_High_LMU_temperature: [number, string] = [46, 'b'];
export const warning_Low_LMU_temperature: [number, string] = [47, 'b'];
export const warning_SUB_communication_interfered: [number, string] = [48, 'b'];
export const warning_LMU_communication_interfered: [number, string] = [49, 'b'];
export const warning_High_current_IN: [number, string] = [50, 'b'];
export const warning_High_current_OUT: [number, string] = [51, 'b'];
export const warning_Pack_resistance_difference: [number, string] = [52, 'b'];
export const warning_High_pack_resistance: [number, string] = [53, 'b'];
export const warning_Cell_resistance_difference: [number, string] = [54, 'b'];
export const warning_High_cell_resistance: [number, string] = [55, 'b'];
export const warning_High_BMCU_supply_voltage: [number, string] = [56, 'b'];
export const warning_Low_BMCU_supply_voltage: [number, string] = [57, 'b'];
export const warning_Low_SOC: [number, string] = [58, 'b'];
export const warning_Balancing_required_OCV_model: [number, string] = [59, 'b'];
export const warning_Charger_not_responding: [number, string] = [60, 'b'];
