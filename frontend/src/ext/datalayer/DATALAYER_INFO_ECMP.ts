export const DATALAYER_INFO_ECMP_FIELDS: ([string, string] | [string, string, number])[] = [
  ['pid_insulation_res_neg', 'u32'],
  ['pid_insulation_res_pos', 'u32'],
  ['pid_max_current_10s', 'u32'],
  ['pid_max_discharge_10s', 'u32'],
  ['pid_max_discharge_30s', 'u32'],
  ['pid_max_charge_10s', 'u32'],
  ['pid_max_charge_30s', 'u32'],
  ['pid_energy_capacity', 'u32'],
  ['pid_insulation_res', 'u32'],
  ['pid_crash_counter', 'u32'],
  ['pid_history_data', 'u32'],
  ['pid_last_can_failure_detail', 'u32'],
  ['pid_hw_version_num', 'u32'],
  ['pid_sw_version_num', 'u32'],
  ['pid_current_time', 'u32'],
  ['pid_time_sent_by_car', 'u32'],
  ['pid_vehicle_speed', 'u32'],
  ['pid_time_spent_over_55c', 'u32'],
  ['pid_contactor_closing_counter', 'u32'],
  ['pid_date_of_manufacture', 'u32'],
  ['pid_current', 'i32'],
  ['pid_most_critical_fault', 'u16'],
  ['HV_BATT_FC_INSU_MINUS_RES', 'u16'],
  ['HV_BATT_FC_INSU_PLUS_RES', 'u16'],
  ['HV_BATT_FC_VHL_INSU_PLUS_RES', 'u16'],
  ['HV_BATT_ONLY_INSU_MINUS_RES', 'u16'],
  ['InsulationResistance', 'u16'],
  ['pid_avg_cell_voltage', 'u16'],
  ['pid_lowsoc_counter', 'u16'],
  ['pid_sum_of_cells', 'u16'],
  ['pid_cell_min_capacity', 'u16'],
  ['pid_pack_voltage', 'u16'],
  ['pid_high_cell_voltage', 'u16'],
  ['pid_low_cell_voltage', 'u16'],
  ['pid_SOH_cell_1', 'u16'],
  ['pid_12v', 'u16'],
  ['pid_hvil_in_voltage', 'u16'],
  ['pid_hvil_out_voltage', 'u16'],
  ['pid_bms_state', 'u8'],
  ['pid_hvil_state', 'u8'],
  ['pid_mainfuse_state', 'u8'],
  ['pid_precharge_short_circuit', 'u8'],
  ['pid_eservice_plug_state', 'u8'],
  ['pid_battery_state', 'u8'],
  ['pid_aux_fuse_state', 'u8'],
  ['pid_12v_abnormal', 'u8'],
  ['InsulationDiag', 'u8'],
  ['MainConnectorState', 'u8'],
  ['CONTACTOR_OPENING_REASON', 'u8'],
  ['TBMU_FAULT_TYPE', 'u8'],
  ['CONTACTORS_STATE', 'u8'],
  ['pid_factory_mode_control', 'u8'],
  ['pid_welding_detection', 'u8'],
  ['pid_reason_open', 'u8'],
  ['pid_contactor_status', 'u8'],
  ['pid_negative_contactor_control', 'u8'],
  ['pid_negative_contactor_status', 'u8'],
  ['pid_positive_contactor_control', 'u8'],
  ['pid_positive_contactor_status', 'u8'],
  ['pid_contactor_negative', 'u8'],
  ['pid_contactor_positive', 'u8'],
  ['pid_precharge_relay_control', 'u8'],
  ['pid_precharge_relay_status', 'u8'],
  ['pid_recharge_status', 'u8'],
  ['pid_coldest_module', 'u8'],
  ['pid_hottest_module', 'u8'],
  ['pid_battery_energy', 'u8'],
  ['pid_wire_crash', 'u8'],
  ['pid_CAN_crash', 'u8'],
  ['pid_highest_cell_voltage_num', 'u8'],
  ['pid_lowest_cell_voltage_num', 'u8'],
  ['pid_cell_voltage_measurement_status', 'u8'],
  ['pid_delta_temperature', 'i8'],
  ['pid_lowest_temperature', 'i8'],
  ['pid_average_temperature', 'i8'],
  ['pid_highest_temperature', 'i8'],
  ['MysteryVan', 'b'],
  ['CrashMemorized', 'b'],
  ['InterlockOpen', 'b'],
  ['ALERT_CELL_POOR_CONSIST', 'b'],
  ['ALERT_OVERCHARGE', 'b'],
  ['ALERT_BATT', 'b'],
  ['ALERT_LOW_SOC', 'b'],
  ['ALERT_HIGH_SOC', 'b'],
  ['ALERT_SOC_JUMP', 'b'],
  ['ALERT_TEMP_DIFF', 'b'],
  ['ALERT_HIGH_TEMP', 'b'],
  ['ALERT_OVERVOLTAGE', 'b'],
  ['ALERT_CELL_OVERVOLTAGE', 'b'],
  ['ALERT_CELL_UNDERVOLTAGE', 'b'],
  ['pid_battery_serial', 'u8', 13],
];

export const pid_insulation_res_neg: [number, string] = [0, 'u32'];
export const pid_insulation_res_pos: [number, string] = [4, 'u32'];
export const pid_max_current_10s: [number, string] = [8, 'u32'];
export const pid_max_discharge_10s: [number, string] = [12, 'u32'];
export const pid_max_discharge_30s: [number, string] = [16, 'u32'];
export const pid_max_charge_10s: [number, string] = [20, 'u32'];
export const pid_max_charge_30s: [number, string] = [24, 'u32'];
export const pid_energy_capacity: [number, string] = [28, 'u32'];
export const pid_insulation_res: [number, string] = [32, 'u32'];
export const pid_crash_counter: [number, string] = [36, 'u32'];
export const pid_history_data: [number, string] = [40, 'u32'];
export const pid_last_can_failure_detail: [number, string] = [44, 'u32'];
export const pid_hw_version_num: [number, string] = [48, 'u32'];
export const pid_sw_version_num: [number, string] = [52, 'u32'];
export const pid_current_time: [number, string] = [56, 'u32'];
export const pid_time_sent_by_car: [number, string] = [60, 'u32'];
export const pid_vehicle_speed: [number, string] = [64, 'u32'];
export const pid_time_spent_over_55c: [number, string] = [68, 'u32'];
export const pid_contactor_closing_counter: [number, string] = [72, 'u32'];
export const pid_date_of_manufacture: [number, string] = [76, 'u32'];
export const pid_current: [number, string] = [80, 'i32'];
export const pid_most_critical_fault: [number, string] = [84, 'u16'];
export const HV_BATT_FC_INSU_MINUS_RES: [number, string] = [86, 'u16'];
export const HV_BATT_FC_INSU_PLUS_RES: [number, string] = [88, 'u16'];
export const HV_BATT_FC_VHL_INSU_PLUS_RES: [number, string] = [90, 'u16'];
export const HV_BATT_ONLY_INSU_MINUS_RES: [number, string] = [92, 'u16'];
export const InsulationResistance: [number, string] = [94, 'u16'];
export const pid_avg_cell_voltage: [number, string] = [96, 'u16'];
export const pid_lowsoc_counter: [number, string] = [98, 'u16'];
export const pid_sum_of_cells: [number, string] = [100, 'u16'];
export const pid_cell_min_capacity: [number, string] = [102, 'u16'];
export const pid_pack_voltage: [number, string] = [104, 'u16'];
export const pid_high_cell_voltage: [number, string] = [106, 'u16'];
export const pid_low_cell_voltage: [number, string] = [108, 'u16'];
export const pid_SOH_cell_1: [number, string] = [110, 'u16'];
export const pid_12v: [number, string] = [112, 'u16'];
export const pid_hvil_in_voltage: [number, string] = [114, 'u16'];
export const pid_hvil_out_voltage: [number, string] = [116, 'u16'];
export const pid_bms_state: [number, string] = [118, 'u8'];
export const pid_hvil_state: [number, string] = [119, 'u8'];
export const pid_mainfuse_state: [number, string] = [120, 'u8'];
export const pid_precharge_short_circuit: [number, string] = [121, 'u8'];
export const pid_eservice_plug_state: [number, string] = [122, 'u8'];
export const pid_battery_state: [number, string] = [123, 'u8'];
export const pid_aux_fuse_state: [number, string] = [124, 'u8'];
export const pid_12v_abnormal: [number, string] = [125, 'u8'];
export const InsulationDiag: [number, string] = [126, 'u8'];
export const MainConnectorState: [number, string] = [127, 'u8'];
export const CONTACTOR_OPENING_REASON: [number, string] = [128, 'u8'];
export const TBMU_FAULT_TYPE: [number, string] = [129, 'u8'];
export const CONTACTORS_STATE: [number, string] = [130, 'u8'];
export const pid_factory_mode_control: [number, string] = [131, 'u8'];
export const pid_welding_detection: [number, string] = [132, 'u8'];
export const pid_reason_open: [number, string] = [133, 'u8'];
export const pid_contactor_status: [number, string] = [134, 'u8'];
export const pid_negative_contactor_control: [number, string] = [135, 'u8'];
export const pid_negative_contactor_status: [number, string] = [136, 'u8'];
export const pid_positive_contactor_control: [number, string] = [137, 'u8'];
export const pid_positive_contactor_status: [number, string] = [138, 'u8'];
export const pid_contactor_negative: [number, string] = [139, 'u8'];
export const pid_contactor_positive: [number, string] = [140, 'u8'];
export const pid_precharge_relay_control: [number, string] = [141, 'u8'];
export const pid_precharge_relay_status: [number, string] = [142, 'u8'];
export const pid_recharge_status: [number, string] = [143, 'u8'];
export const pid_coldest_module: [number, string] = [144, 'u8'];
export const pid_hottest_module: [number, string] = [145, 'u8'];
export const pid_battery_energy: [number, string] = [146, 'u8'];
export const pid_wire_crash: [number, string] = [147, 'u8'];
export const pid_CAN_crash: [number, string] = [148, 'u8'];
export const pid_highest_cell_voltage_num: [number, string] = [149, 'u8'];
export const pid_lowest_cell_voltage_num: [number, string] = [150, 'u8'];
export const pid_cell_voltage_measurement_status: [number, string] = [151, 'u8'];
export const pid_delta_temperature: [number, string] = [152, 'i8'];
export const pid_lowest_temperature: [number, string] = [153, 'i8'];
export const pid_average_temperature: [number, string] = [154, 'i8'];
export const pid_highest_temperature: [number, string] = [155, 'i8'];
export const MysteryVan: [number, string] = [156, 'b'];
export const CrashMemorized: [number, string] = [157, 'b'];
export const InterlockOpen: [number, string] = [158, 'b'];
export const ALERT_CELL_POOR_CONSIST: [number, string] = [159, 'b'];
export const ALERT_OVERCHARGE: [number, string] = [160, 'b'];
export const ALERT_BATT: [number, string] = [161, 'b'];
export const ALERT_LOW_SOC: [number, string] = [162, 'b'];
export const ALERT_HIGH_SOC: [number, string] = [163, 'b'];
export const ALERT_SOC_JUMP: [number, string] = [164, 'b'];
export const ALERT_TEMP_DIFF: [number, string] = [165, 'b'];
export const ALERT_HIGH_TEMP: [number, string] = [166, 'b'];
export const ALERT_OVERVOLTAGE: [number, string] = [167, 'b'];
export const ALERT_CELL_OVERVOLTAGE: [number, string] = [168, 'b'];
export const ALERT_CELL_UNDERVOLTAGE: [number, string] = [169, 'b'];
export const pid_battery_serial: [number, string, number] = [170, 'u8', 13];
