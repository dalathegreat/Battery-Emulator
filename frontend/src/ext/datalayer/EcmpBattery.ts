// EcmpBattery: 1600 bytes; base classes: CanBattery@0
export const ECMP_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_ecmp', 'DATALAYER_INFO_ECMP*'],
  ['renderer', 'EcmpHtmlRenderer'],
  ['previousMillis10', 'u32'],
  ['previousMillis20', 'u32'],
  ['previousMillis50', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis250', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillis5000', 'u32'],
  ['', ' ', 4],
  ['ECMP_010', 'CAN_frame'],
  ['ECMP_0F0', 'CAN_frame'],
  ['ECMP_0F2', 'CAN_frame'],
  ['ECMP_110', 'CAN_frame'],
  ['ECMP_112', 'CAN_frame'],
  ['ECMP_17B', 'CAN_frame'],
  ['ECMP_27A', 'CAN_frame'],
  ['ECMP_3D0', 'CAN_frame'],
  ['ECMP_345', 'CAN_frame'],
  ['ECMP_3A2', 'CAN_frame'],
  ['ECMP_3A3', 'CAN_frame'],
  ['ECMP_552', 'CAN_frame'],
  ['ECMP_POLL', 'CAN_frame'],
  ['ticks_552', 'u32'],
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
  ['pid_vehicle_speed', 'u32'],
  ['pid_time_spent_over_55c', 'u32'],
  ['pid_contactor_closing_counter', 'u32'],
  ['pid_date_of_manufacture', 'u32'],
  ['pid_current_time', 'u32'],
  ['pid_time_sent_by_car', 'u32'],
  ['pid_current', 'i32'],
  ['battery_voltage', 'u16'],
  ['battery_soc', 'u16'],
  ['cellvoltages', 'u16', 108],
  ['battery_AllowedMaxChargeCurrent', 'u16'],
  ['battery_AllowedMaxDischargeCurrent', 'u16'],
  ['battery_insulationResistanceKOhm', 'u16'],
  ['pid_lowsoc_counter', 'u16'],
  ['poll_state', 'u16'],
  ['incoming_poll', 'u16'],
  ['pid_hvil_out_voltage', 'u16'],
  ['pid_most_critical_fault', 'u16'],
  ['pid_avg_cell_voltage', 'u16'],
  ['pid_sum_of_cells', 'u16'],
  ['pid_cell_min_capacity', 'u16'],
  ['pid_pack_voltage', 'u16'],
  ['pid_high_cell_voltage', 'u16'],
  ['pid_low_cell_voltage', 'u16'],
  ['pid_12v', 'u16'],
  ['pid_hvil_in_voltage', 'u16'],
  ['pid_SOH_cell_1', 'u16'],
  ['SOE_MAX_CURRENT_TEMP', 'u16'],
  ['FRONT_MACHINE_POWER_LIMIT', 'u16'],
  ['REAR_MACHINE_POWER_LIMIT', 'u16'],
  ['EVSE_INSTANT_DC_HV_CURRENT', 'u16'],
  ['HV_BATT_SOE_HD', 'u16'],
  ['HV_BATT_SOE_MAX', 'u16'],
  ['HV_BATT_STABLE_CHARGE_POWER_HD', 'u16'],
  ['HV_BATT_STABLE_DISCH_POWER_HD', 'u16'],
  ['HV_BATT_NOMINAL_DISCH_POWER_HD', 'u16'],
  ['MAX_ALLOW_DISCHRG_CURRENT', 'u16'],
  ['HV_STORAGE_MAX_I', 'u16'],
  ['HV_BATT_PEAK_DISCH_POWER_HD', 'u16'],
  ['HV_BATT_PEAK_CH_POWER_HD', 'u16'],
  ['HV_BATT_NOM_CH_POWER_HD', 'u16'],
  ['MAX_ALLOW_CHRG_CURRENT', 'u16'],
  ['HV_BATT_FC_INSU_MINUS_RES', 'u16'],
  ['HV_BATT_FC_INSU_PLUS_RES', 'u16'],
  ['HV_BATT_FC_VHL_INSU_PLUS_RES', 'u16'],
  ['HV_BATT_ONLY_INSU_MINUS_RES', 'u16'],
  ['MIN_ALLOW_DISCHRG_VOLTAGE', 'u16'],
  ['HV_BATT_NOMINAL_DISCH_CURR_HD', 'u16'],
  ['HV_BATT_PEAK_DISCH_CURR_HD', 'u16'],
  ['HV_BATT_STABLE_DISCH_CURR_HD', 'u16'],
  ['HV_BATT_NOMINAL_CHARGE_CURR_HD', 'u16'],
  ['HV_BATT_PEAK_CHARGE_CURR_HD', 'u16'],
  ['HV_BATT_STABLE_CHARGE_CURR_HD', 'u16'],
  ['HV_BATT_REAL_VOLT_HD', 'u16'],
  ['HV_BATT_NOM_CH_VOLTAGE', 'u16'],
  ['HV_BATT_REAL_POWER_HD', 'u16'],
  ['MAX_ALLOW_CHRG_POWER', 'u16'],
  ['MAX_ALLOW_DISCHRG_POWER', 'u16'],
  ['HV_BATT_SOC', 'u16'],
  ['HV_BATT_GENERATED_HEAT_RATE', 'u16'],
  ['BMS_DC_RELAY_MES_EVSE_VOLTAGE', 'u16'],
  ['HV_BATT_COP_VOLTAGE', 'u16'],
  ['HV_BATT_MAX_REAL_CURR', 'i16'],
  ['HV_BATT_REAL_CURR_HD', 'i16'],
  ['battery_current', 'i16'],
  ['battery_highestTemperature', 'i16'],
  ['battery_lowestTemperature', 'i16'],
  ['HV_BATT_COP_CURRENT', 'i16'],
  ['ContactorResetStatemachine', 'u8'],
  ['CollisionResetStatemachine', 'u8'],
  ['IsolationResetStatemachine', 'u8'],
  ['DisableIsoMonitoringStatemachine', 'u8'],
  ['timeSpentDisableIsoMonitoring', 'u8'],
  ['timeSpentContactorReset', 'u8'],
  ['timeSpentCollisionReset', 'u8'],
  ['timeSpentIsolationReset', 'u8'],
  ['countIsolationReset', 'u8'],
  ['counter_10ms', 'u8'],
  ['counter_20ms', 'u8'],
  ['counter_50ms', 'u8'],
  ['counter_100ms', 'u8'],
  ['counter_010', 'u8'],
  ['battery_MainConnectorState', 'u8'],
  ['battery_insulation_failure_diag', 'u8'],
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
  ['pid_highest_cell_voltage_num', 'u8'],
  ['pid_lowest_cell_voltage_num', 'u8'],
  ['pid_cell_voltage_measurement_status', 'u8'],
  ['pid_battery_energy', 'u8'],
  ['pid_12v_abnormal', 'u8'],
  ['pid_factory_mode_control', 'u8'],
  ['pid_battery_serial', 'u8', 14],
  ['pid_aux_fuse_state', 'u8'],
  ['pid_battery_state', 'u8'],
  ['pid_precharge_short_circuit', 'u8'],
  ['pid_eservice_plug_state', 'u8'],
  ['pid_mainfuse_state', 'u8'],
  ['pid_hvil_state', 'u8'],
  ['pid_bms_state', 'u8'],
  ['pid_wire_crash', 'u8'],
  ['pid_CAN_crash', 'u8'],
  ['EVSE_STATE', 'u8'],
  ['CHECKSUM_FRAME_314', 'u8'],
  ['CHECKSUM_FRAME_3B4', 'u8'],
  ['CHECKSUM_FRAME_554', 'u8'],
  ['CHECKSUM_FRAME_373', 'u8'],
  ['CHECKSUM_FRAME_4F4', 'u8'],
  ['CHECKSUM_FRAME_414', 'u8'],
  ['CHECKSUM_FRAME_353', 'u8'],
  ['CHECKSUM_FRAME_474', 'u8'],
  ['CHECKSUM_FRAME_4D4', 'u8'],
  ['FAST_CHARGE_CONTACTOR_STATE', 'u8'],
  ['BMS_FASTCHARGE_STATUS', 'u8'],
  ['HV_BATT_NOM_CH_CURRENT', 'u8'],
  ['TBMU_FAULT_TYPE', 'u8'],
  ['CONTACTORS_STATE', 'u8'],
  ['NUMBER_PROBE_TEMP_MAX', 'u8'],
  ['NUMBER_PROBE_TEMP_MIN', 'u8'],
  ['NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY', 'u8'],
  ['NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY', 'u8'],
  ['CONTACTOR_OPENING_REASON', 'u8'],
  ['pid_delta_temperature', 'i8'],
  ['pid_lowest_temperature', 'i8'],
  ['pid_average_temperature', 'i8'],
  ['pid_highest_temperature', 'i8'],
  ['BMS_PROBETEMP', 'i8', 7],
  ['TEMPERATURE_MINIMUM_C', 'i8'],
  ['HighPrecisionCurrentSampling', 'b'],
  ['CMD_RESET_MIL', 'b'],
  ['REQ_BLINK_STOP_AND_SERVICE_LAMP', 'b'],
  ['REQ_MIL_LAMP_CONTINOUS', 'b'],
  ['HV_BATT_CRASH_MEMORIZED', 'b'],
  ['HV_BATT_COLD_CRANK_ACK', 'b'],
  ['HV_BATT_CHARGE_NEEDED_STATE', 'b'],
  ['RC01_PERM_SYNTH_TBMU', 'b'],
  ['battery_RelayOpenRequest', 'b'],
  ['battery_InterlockOpen', 'b'],
  ['MysteryVan', 'b'],
  ['TBCU_48V_WAKEUP', 'b'],
  ['REQ_CLEAR_DTC_TBMU', 'b'],
  ['HV_BATT_DISCONT_WARNING_OPEN', 'b'],
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
  ['UserRequestDTCreset', 'b'],
  ['UserRequestContactorReset', 'b'],
  ['UserRequestCollisionReset', 'b'],
  ['UserRequestIsolationReset', 'b'],
  ['UserRequestDisableIsoMonitoring', 'b'],
  ['data_010_CRC', 'u8', 8],
  ['data_3A2_CRC', 'u8', 16],
  ['data_345_content', 'u8', 16],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const datalayer_battery: [number, string] = [24, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_ecmp: [number, string] = [28, 'DATALAYER_INFO_ECMP*'];
export const renderer: [number, string] = [32, 'EcmpHtmlRenderer'];
export const previousMillis10: [number, string] = [36, 'u32'];
export const previousMillis20: [number, string] = [40, 'u32'];
export const previousMillis50: [number, string] = [44, 'u32'];
export const previousMillis100: [number, string] = [48, 'u32'];
export const previousMillis250: [number, string] = [52, 'u32'];
export const previousMillis500: [number, string] = [56, 'u32'];
export const previousMillis1000: [number, string] = [60, 'u32'];
export const previousMillis5000: [number, string] = [64, 'u32'];
export const ECMP_010: [number, string] = [72, 'CAN_frame'];
export const ECMP_0F0: [number, string] = [144, 'CAN_frame'];
export const ECMP_0F2: [number, string] = [216, 'CAN_frame'];
export const ECMP_110: [number, string] = [288, 'CAN_frame'];
export const ECMP_112: [number, string] = [360, 'CAN_frame'];
export const ECMP_17B: [number, string] = [432, 'CAN_frame'];
export const ECMP_27A: [number, string] = [504, 'CAN_frame'];
export const ECMP_3D0: [number, string] = [576, 'CAN_frame'];
export const ECMP_345: [number, string] = [648, 'CAN_frame'];
export const ECMP_3A2: [number, string] = [720, 'CAN_frame'];
export const ECMP_3A3: [number, string] = [792, 'CAN_frame'];
export const ECMP_552: [number, string] = [864, 'CAN_frame'];
export const ECMP_POLL: [number, string] = [936, 'CAN_frame'];
export const ticks_552: [number, string] = [1008, 'u32'];
export const pid_insulation_res_neg: [number, string] = [1012, 'u32'];
export const pid_insulation_res_pos: [number, string] = [1016, 'u32'];
export const pid_max_current_10s: [number, string] = [1020, 'u32'];
export const pid_max_discharge_10s: [number, string] = [1024, 'u32'];
export const pid_max_discharge_30s: [number, string] = [1028, 'u32'];
export const pid_max_charge_10s: [number, string] = [1032, 'u32'];
export const pid_max_charge_30s: [number, string] = [1036, 'u32'];
export const pid_energy_capacity: [number, string] = [1040, 'u32'];
export const pid_insulation_res: [number, string] = [1044, 'u32'];
export const pid_crash_counter: [number, string] = [1048, 'u32'];
export const pid_history_data: [number, string] = [1052, 'u32'];
export const pid_last_can_failure_detail: [number, string] = [1056, 'u32'];
export const pid_hw_version_num: [number, string] = [1060, 'u32'];
export const pid_sw_version_num: [number, string] = [1064, 'u32'];
export const pid_vehicle_speed: [number, string] = [1068, 'u32'];
export const pid_time_spent_over_55c: [number, string] = [1072, 'u32'];
export const pid_contactor_closing_counter: [number, string] = [1076, 'u32'];
export const pid_date_of_manufacture: [number, string] = [1080, 'u32'];
export const pid_current_time: [number, string] = [1084, 'u32'];
export const pid_time_sent_by_car: [number, string] = [1088, 'u32'];
export const pid_current: [number, string] = [1092, 'i32'];
export const battery_voltage: [number, string] = [1096, 'u16'];
export const battery_soc: [number, string] = [1098, 'u16'];
export const cellvoltages: [number, string, number] = [1100, 'u16', 108];
export const battery_AllowedMaxChargeCurrent: [number, string] = [1316, 'u16'];
export const battery_AllowedMaxDischargeCurrent: [number, string] = [1318, 'u16'];
export const battery_insulationResistanceKOhm: [number, string] = [1320, 'u16'];
export const pid_lowsoc_counter: [number, string] = [1322, 'u16'];
export const poll_state: [number, string] = [1324, 'u16'];
export const incoming_poll: [number, string] = [1326, 'u16'];
export const pid_hvil_out_voltage: [number, string] = [1328, 'u16'];
export const pid_most_critical_fault: [number, string] = [1330, 'u16'];
export const pid_avg_cell_voltage: [number, string] = [1332, 'u16'];
export const pid_sum_of_cells: [number, string] = [1334, 'u16'];
export const pid_cell_min_capacity: [number, string] = [1336, 'u16'];
export const pid_pack_voltage: [number, string] = [1338, 'u16'];
export const pid_high_cell_voltage: [number, string] = [1340, 'u16'];
export const pid_low_cell_voltage: [number, string] = [1342, 'u16'];
export const pid_12v: [number, string] = [1344, 'u16'];
export const pid_hvil_in_voltage: [number, string] = [1346, 'u16'];
export const pid_SOH_cell_1: [number, string] = [1348, 'u16'];
export const SOE_MAX_CURRENT_TEMP: [number, string] = [1350, 'u16'];
export const FRONT_MACHINE_POWER_LIMIT: [number, string] = [1352, 'u16'];
export const REAR_MACHINE_POWER_LIMIT: [number, string] = [1354, 'u16'];
export const EVSE_INSTANT_DC_HV_CURRENT: [number, string] = [1356, 'u16'];
export const HV_BATT_SOE_HD: [number, string] = [1358, 'u16'];
export const HV_BATT_SOE_MAX: [number, string] = [1360, 'u16'];
export const HV_BATT_STABLE_CHARGE_POWER_HD: [number, string] = [1362, 'u16'];
export const HV_BATT_STABLE_DISCH_POWER_HD: [number, string] = [1364, 'u16'];
export const HV_BATT_NOMINAL_DISCH_POWER_HD: [number, string] = [1366, 'u16'];
export const MAX_ALLOW_DISCHRG_CURRENT: [number, string] = [1368, 'u16'];
export const HV_STORAGE_MAX_I: [number, string] = [1370, 'u16'];
export const HV_BATT_PEAK_DISCH_POWER_HD: [number, string] = [1372, 'u16'];
export const HV_BATT_PEAK_CH_POWER_HD: [number, string] = [1374, 'u16'];
export const HV_BATT_NOM_CH_POWER_HD: [number, string] = [1376, 'u16'];
export const MAX_ALLOW_CHRG_CURRENT: [number, string] = [1378, 'u16'];
export const HV_BATT_FC_INSU_MINUS_RES: [number, string] = [1380, 'u16'];
export const HV_BATT_FC_INSU_PLUS_RES: [number, string] = [1382, 'u16'];
export const HV_BATT_FC_VHL_INSU_PLUS_RES: [number, string] = [1384, 'u16'];
export const HV_BATT_ONLY_INSU_MINUS_RES: [number, string] = [1386, 'u16'];
export const MIN_ALLOW_DISCHRG_VOLTAGE: [number, string] = [1388, 'u16'];
export const HV_BATT_NOMINAL_DISCH_CURR_HD: [number, string] = [1390, 'u16'];
export const HV_BATT_PEAK_DISCH_CURR_HD: [number, string] = [1392, 'u16'];
export const HV_BATT_STABLE_DISCH_CURR_HD: [number, string] = [1394, 'u16'];
export const HV_BATT_NOMINAL_CHARGE_CURR_HD: [number, string] = [1396, 'u16'];
export const HV_BATT_PEAK_CHARGE_CURR_HD: [number, string] = [1398, 'u16'];
export const HV_BATT_STABLE_CHARGE_CURR_HD: [number, string] = [1400, 'u16'];
export const HV_BATT_REAL_VOLT_HD: [number, string] = [1402, 'u16'];
export const HV_BATT_NOM_CH_VOLTAGE: [number, string] = [1404, 'u16'];
export const HV_BATT_REAL_POWER_HD: [number, string] = [1406, 'u16'];
export const MAX_ALLOW_CHRG_POWER: [number, string] = [1408, 'u16'];
export const MAX_ALLOW_DISCHRG_POWER: [number, string] = [1410, 'u16'];
export const HV_BATT_SOC: [number, string] = [1412, 'u16'];
export const HV_BATT_GENERATED_HEAT_RATE: [number, string] = [1414, 'u16'];
export const BMS_DC_RELAY_MES_EVSE_VOLTAGE: [number, string] = [1416, 'u16'];
export const HV_BATT_COP_VOLTAGE: [number, string] = [1418, 'u16'];
export const HV_BATT_MAX_REAL_CURR: [number, string] = [1420, 'i16'];
export const HV_BATT_REAL_CURR_HD: [number, string] = [1422, 'i16'];
export const battery_current: [number, string] = [1424, 'i16'];
export const battery_highestTemperature: [number, string] = [1426, 'i16'];
export const battery_lowestTemperature: [number, string] = [1428, 'i16'];
export const HV_BATT_COP_CURRENT: [number, string] = [1430, 'i16'];
export const ContactorResetStatemachine: [number, string] = [1432, 'u8'];
export const CollisionResetStatemachine: [number, string] = [1433, 'u8'];
export const IsolationResetStatemachine: [number, string] = [1434, 'u8'];
export const DisableIsoMonitoringStatemachine: [number, string] = [1435, 'u8'];
export const timeSpentDisableIsoMonitoring: [number, string] = [1436, 'u8'];
export const timeSpentContactorReset: [number, string] = [1437, 'u8'];
export const timeSpentCollisionReset: [number, string] = [1438, 'u8'];
export const timeSpentIsolationReset: [number, string] = [1439, 'u8'];
export const countIsolationReset: [number, string] = [1440, 'u8'];
export const counter_10ms: [number, string] = [1441, 'u8'];
export const counter_20ms: [number, string] = [1442, 'u8'];
export const counter_50ms: [number, string] = [1443, 'u8'];
export const counter_100ms: [number, string] = [1444, 'u8'];
export const counter_010: [number, string] = [1445, 'u8'];
export const battery_MainConnectorState: [number, string] = [1446, 'u8'];
export const battery_insulation_failure_diag: [number, string] = [1447, 'u8'];
export const pid_welding_detection: [number, string] = [1448, 'u8'];
export const pid_reason_open: [number, string] = [1449, 'u8'];
export const pid_contactor_status: [number, string] = [1450, 'u8'];
export const pid_negative_contactor_control: [number, string] = [1451, 'u8'];
export const pid_negative_contactor_status: [number, string] = [1452, 'u8'];
export const pid_positive_contactor_control: [number, string] = [1453, 'u8'];
export const pid_positive_contactor_status: [number, string] = [1454, 'u8'];
export const pid_contactor_negative: [number, string] = [1455, 'u8'];
export const pid_contactor_positive: [number, string] = [1456, 'u8'];
export const pid_precharge_relay_control: [number, string] = [1457, 'u8'];
export const pid_precharge_relay_status: [number, string] = [1458, 'u8'];
export const pid_recharge_status: [number, string] = [1459, 'u8'];
export const pid_coldest_module: [number, string] = [1460, 'u8'];
export const pid_hottest_module: [number, string] = [1461, 'u8'];
export const pid_highest_cell_voltage_num: [number, string] = [1462, 'u8'];
export const pid_lowest_cell_voltage_num: [number, string] = [1463, 'u8'];
export const pid_cell_voltage_measurement_status: [number, string] = [1464, 'u8'];
export const pid_battery_energy: [number, string] = [1465, 'u8'];
export const pid_12v_abnormal: [number, string] = [1466, 'u8'];
export const pid_factory_mode_control: [number, string] = [1467, 'u8'];
export const pid_battery_serial: [number, string, number] = [1468, 'u8', 14];
export const pid_aux_fuse_state: [number, string] = [1482, 'u8'];
export const pid_battery_state: [number, string] = [1483, 'u8'];
export const pid_precharge_short_circuit: [number, string] = [1484, 'u8'];
export const pid_eservice_plug_state: [number, string] = [1485, 'u8'];
export const pid_mainfuse_state: [number, string] = [1486, 'u8'];
export const pid_hvil_state: [number, string] = [1487, 'u8'];
export const pid_bms_state: [number, string] = [1488, 'u8'];
export const pid_wire_crash: [number, string] = [1489, 'u8'];
export const pid_CAN_crash: [number, string] = [1490, 'u8'];
export const EVSE_STATE: [number, string] = [1491, 'u8'];
export const CHECKSUM_FRAME_314: [number, string] = [1492, 'u8'];
export const CHECKSUM_FRAME_3B4: [number, string] = [1493, 'u8'];
export const CHECKSUM_FRAME_554: [number, string] = [1494, 'u8'];
export const CHECKSUM_FRAME_373: [number, string] = [1495, 'u8'];
export const CHECKSUM_FRAME_4F4: [number, string] = [1496, 'u8'];
export const CHECKSUM_FRAME_414: [number, string] = [1497, 'u8'];
export const CHECKSUM_FRAME_353: [number, string] = [1498, 'u8'];
export const CHECKSUM_FRAME_474: [number, string] = [1499, 'u8'];
export const CHECKSUM_FRAME_4D4: [number, string] = [1500, 'u8'];
export const FAST_CHARGE_CONTACTOR_STATE: [number, string] = [1501, 'u8'];
export const BMS_FASTCHARGE_STATUS: [number, string] = [1502, 'u8'];
export const HV_BATT_NOM_CH_CURRENT: [number, string] = [1503, 'u8'];
export const TBMU_FAULT_TYPE: [number, string] = [1504, 'u8'];
export const CONTACTORS_STATE: [number, string] = [1505, 'u8'];
export const NUMBER_PROBE_TEMP_MAX: [number, string] = [1506, 'u8'];
export const NUMBER_PROBE_TEMP_MIN: [number, string] = [1507, 'u8'];
export const NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY: [number, string] = [1508, 'u8'];
export const NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY: [number, string] = [1509, 'u8'];
export const CONTACTOR_OPENING_REASON: [number, string] = [1510, 'u8'];
export const pid_delta_temperature: [number, string] = [1511, 'i8'];
export const pid_lowest_temperature: [number, string] = [1512, 'i8'];
export const pid_average_temperature: [number, string] = [1513, 'i8'];
export const pid_highest_temperature: [number, string] = [1514, 'i8'];
export const BMS_PROBETEMP: [number, string, number] = [1515, 'i8', 7];
export const TEMPERATURE_MINIMUM_C: [number, string] = [1522, 'i8'];
export const HighPrecisionCurrentSampling: [number, string] = [1523, 'b'];
export const CMD_RESET_MIL: [number, string] = [1524, 'b'];
export const REQ_BLINK_STOP_AND_SERVICE_LAMP: [number, string] = [1525, 'b'];
export const REQ_MIL_LAMP_CONTINOUS: [number, string] = [1526, 'b'];
export const HV_BATT_CRASH_MEMORIZED: [number, string] = [1527, 'b'];
export const HV_BATT_COLD_CRANK_ACK: [number, string] = [1528, 'b'];
export const HV_BATT_CHARGE_NEEDED_STATE: [number, string] = [1529, 'b'];
export const RC01_PERM_SYNTH_TBMU: [number, string] = [1530, 'b'];
export const battery_RelayOpenRequest: [number, string] = [1531, 'b'];
export const battery_InterlockOpen: [number, string] = [1532, 'b'];
export const MysteryVan: [number, string] = [1533, 'b'];
export const TBCU_48V_WAKEUP: [number, string] = [1534, 'b'];
export const REQ_CLEAR_DTC_TBMU: [number, string] = [1535, 'b'];
export const HV_BATT_DISCONT_WARNING_OPEN: [number, string] = [1536, 'b'];
export const ALERT_CELL_POOR_CONSIST: [number, string] = [1537, 'b'];
export const ALERT_OVERCHARGE: [number, string] = [1538, 'b'];
export const ALERT_BATT: [number, string] = [1539, 'b'];
export const ALERT_LOW_SOC: [number, string] = [1540, 'b'];
export const ALERT_HIGH_SOC: [number, string] = [1541, 'b'];
export const ALERT_SOC_JUMP: [number, string] = [1542, 'b'];
export const ALERT_TEMP_DIFF: [number, string] = [1543, 'b'];
export const ALERT_HIGH_TEMP: [number, string] = [1544, 'b'];
export const ALERT_OVERVOLTAGE: [number, string] = [1545, 'b'];
export const ALERT_CELL_OVERVOLTAGE: [number, string] = [1546, 'b'];
export const ALERT_CELL_UNDERVOLTAGE: [number, string] = [1547, 'b'];
export const UserRequestDTCreset: [number, string] = [1548, 'b'];
export const UserRequestContactorReset: [number, string] = [1549, 'b'];
export const UserRequestCollisionReset: [number, string] = [1550, 'b'];
export const UserRequestIsolationReset: [number, string] = [1551, 'b'];
export const UserRequestDisableIsoMonitoring: [number, string] = [1552, 'b'];
export const data_010_CRC: [number, string, number] = [1553, 'u8', 8];
export const data_3A2_CRC: [number, string, number] = [1561, 'u8', 16];
export const data_345_content: [number, string, number] = [1577, 'u8', 16];
