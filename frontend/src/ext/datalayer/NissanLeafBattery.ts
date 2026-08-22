// NissanLeafBattery: 1616 bytes; base classes: CanBattery@0
export const NISSAN_LEAF_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestSOHreset', 'b'],
  ['', ' '],
  ['renderer', 'NissanLeafHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_nissan', 'DATALAYER_INFO_NISSAN_LEAF*'],
  ['allows_contactor_closing', 'bool*'],
  ['previousMillis10', 'u32'],
  ['previousMillis40', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis10s', 'u32'],
  ['mprun10r', 'u8'],
  ['mprun10', 'u8'],
  ['mprun100', 'u8'],
  ['counter_3B8', 'u8'],
  ['flip_3B8', 'b'],
  ['', ' ', 3],
  ['LEAF_1F2', 'CAN_frame'],
  ['LEAF_50B', 'CAN_frame'],
  ['LEAF_50C', 'CAN_frame'],
  ['LEAF_1D4', 'CAN_frame'],
  ['LEAF_355', 'CAN_frame'],
  ['LEAF_3B8', 'CAN_frame'],
  ['LEAF_5C5', 'CAN_frame'],
  ['LEAF_5EC', 'CAN_frame'],
  ['LEAF_626', 'CAN_frame'],
  ['PIDgroups', 'u8', 7],
  ['PIDindex', 'u8'],
  ['poll_burst_remaining', 'u8'],
  ['', ' ', 7],
  ['LEAF_GROUP_REQUEST', 'CAN_frame'],
  ['LEAF_NEXT_LINE_REQUEST', 'CAN_frame'],
  ['LEAF_CLEAR_DTC', 'CAN_frame'],
  ['LEAF_READ_DTC', 'CAN_frame'],
  ['dtc_buffer', 'u8', 131],
  ['', ' '],
  ['dtc_rx_total', 'u16'],
  ['dtc_rx_seen', 'u16'],
  ['dtc_rx_len', 'u16'],
  ['dtc_rx_active', 'b'],
  ['dtc_read_in_progress', 'b'],
  ['dtc_request_millis', 'u32'],
  ['dtc_clear_in_progress', 'b'],
  ['', ' ', 3],
  ['dtc_clear_millis', 'u32'],
  ['last_7bb_millis', 'u32'],
  ['dtc_read_retries', 'u8'],
  ['uds_busy', 'b'],
  ['', ' ', 2],
  ['uds_request_millis', 'u32'],
  ['uds_rx_remaining', 'u16'],
  ['LEAF_battery_Type', 'u8'],
  ['battery_can_alive', 'b'],
  ['battery_Discharge_Power_Limit', 'u16'],
  ['battery_Charge_Power_Limit', 'u16'],
  ['battery_MAX_POWER_FOR_CHARGER', 'i16'],
  ['battery_SOC', 'i16'],
  ['battery_TEMP', 'u16'],
  ['battery_Wh_Remaining', 'u16'],
  ['battery_GIDS', 'u16'],
  ['battery_MAX', 'u16'],
  ['battery_Max_GIDS', 'u16'],
  ['battery_StateOfHealth', 'u16'],
  ['battery_Total_Voltage2', 'u16'],
  ['battery_Current2', 'i16'],
  ['battery_HistData_Temperature_MAX', 'i16'],
  ['battery_HistData_Temperature_MIN', 'i16'],
  ['battery_AverageTemperature', 'i16'],
  ['battery_Relay_Cut_Request', 'u8'],
  ['battery_Failsafe_Status', 'u8'],
  ['battery_Interlock', 'b'],
  ['battery_Full_CHARGE_flag', 'b'],
  ['battery_MainRelayOn_flag', 'b'],
  ['battery_Capacity_Empty', 'b'],
  ['battery_HeatExist', 'b'],
  ['battery_Heating_Stop', 'b'],
  ['battery_Heating_Start', 'b'],
  ['battery_Batt_Heater_Mail_Send_Request', 'b'],
  ['battery_request_idx', 'u8'],
  ['group_7bb', 'u8'],
  ['group_7bb_length', 'u8'],
  ['stop_battery_query', 'b'],
  ['hold_off_with_polling_10seconds', 'u8'],
  ['', ' '],
  ['battery_cell_voltages', 'u16', 96],
  ['battery_balancing_shunts', 'b', 96],
  ['', ' ', 2],
  ['balancing_bitmap_prev', 'u32', 3],
  ['balancing_bitmap_valid', 'b'],
  ['', ' '],
  ['balancing_unchanged_window', 'u16'],
  ['balancing_window_fill', 'u8'],
  ['balancing_low_reads', 'u8'],
  ['balancing_frames_seen', 'u8'],
  ['balancing_data_fresh', 'b'],
  ['battery_cellcounter', 'u8'],
  ['', ' '],
  ['battery_min_max_voltage', 'u16', 2],
  ['battery_HX_pptt', 'u16'],
  ['battery_insulation', 'u16'],
  ['battery_charge_count_qc', 'u16'],
  ['battery_charge_count_l1l2', 'u16'],
  ['battery_temp_raw_1', 'u16'],
  ['battery_temp_raw_2_highnibble', 'u8'],
  ['', ' '],
  ['battery_temp_raw_2', 'u16'],
  ['battery_temp_raw_3', 'u16'],
  ['battery_temp_raw_4', 'u16'],
  ['battery_temp_raw_max', 'u16'],
  ['battery_temp_raw_min', 'u16'],
  ['battery_temp_polled_max', 'i16'],
  ['battery_temp_polled_min', 'i16'],
  ['BatterySerialNumber', 'u8', 15],
  ['BatteryPartNumber', 'u8', 7],
  ['stateMachineClearSOH', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const UserRequestDTCreadout: [number, string] = [25, 'b'];
export const UserRequestSOHreset: [number, string] = [26, 'b'];
export const renderer: [number, string] = [28, 'NissanLeafHtmlRenderer'];
export const datalayer_battery: [number, string] = [40, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_nissan: [number, string] = [44, 'DATALAYER_INFO_NISSAN_LEAF*'];
export const allows_contactor_closing: [number, string] = [48, 'bool*'];
export const previousMillis10: [number, string] = [52, 'u32'];
export const previousMillis40: [number, string] = [56, 'u32'];
export const previousMillis100: [number, string] = [60, 'u32'];
export const previousMillis500: [number, string] = [64, 'u32'];
export const previousMillis10s: [number, string] = [68, 'u32'];
export const mprun10r: [number, string] = [72, 'u8'];
export const mprun10: [number, string] = [73, 'u8'];
export const mprun100: [number, string] = [74, 'u8'];
export const counter_3B8: [number, string] = [75, 'u8'];
export const flip_3B8: [number, string] = [76, 'b'];
export const LEAF_1F2: [number, string] = [80, 'CAN_frame'];
export const LEAF_50B: [number, string] = [152, 'CAN_frame'];
export const LEAF_50C: [number, string] = [224, 'CAN_frame'];
export const LEAF_1D4: [number, string] = [296, 'CAN_frame'];
export const LEAF_355: [number, string] = [368, 'CAN_frame'];
export const LEAF_3B8: [number, string] = [440, 'CAN_frame'];
export const LEAF_5C5: [number, string] = [512, 'CAN_frame'];
export const LEAF_5EC: [number, string] = [584, 'CAN_frame'];
export const LEAF_626: [number, string] = [656, 'CAN_frame'];
export const PIDgroups: [number, string, number] = [728, 'u8', 7];
export const PIDindex: [number, string] = [735, 'u8'];
export const poll_burst_remaining: [number, string] = [736, 'u8'];
export const LEAF_GROUP_REQUEST: [number, string] = [744, 'CAN_frame'];
export const LEAF_NEXT_LINE_REQUEST: [number, string] = [816, 'CAN_frame'];
export const LEAF_CLEAR_DTC: [number, string] = [888, 'CAN_frame'];
export const LEAF_READ_DTC: [number, string] = [960, 'CAN_frame'];
export const dtc_buffer: [number, string, number] = [1032, 'u8', 131];
export const dtc_rx_total: [number, string] = [1164, 'u16'];
export const dtc_rx_seen: [number, string] = [1166, 'u16'];
export const dtc_rx_len: [number, string] = [1168, 'u16'];
export const dtc_rx_active: [number, string] = [1170, 'b'];
export const dtc_read_in_progress: [number, string] = [1171, 'b'];
export const dtc_request_millis: [number, string] = [1172, 'u32'];
export const dtc_clear_in_progress: [number, string] = [1176, 'b'];
export const dtc_clear_millis: [number, string] = [1180, 'u32'];
export const last_7bb_millis: [number, string] = [1184, 'u32'];
export const dtc_read_retries: [number, string] = [1188, 'u8'];
export const uds_busy: [number, string] = [1189, 'b'];
export const uds_request_millis: [number, string] = [1192, 'u32'];
export const uds_rx_remaining: [number, string] = [1196, 'u16'];
export const LEAF_battery_Type: [number, string] = [1198, 'u8'];
export const battery_can_alive: [number, string] = [1199, 'b'];
export const battery_Discharge_Power_Limit: [number, string] = [1200, 'u16'];
export const battery_Charge_Power_Limit: [number, string] = [1202, 'u16'];
export const battery_MAX_POWER_FOR_CHARGER: [number, string] = [1204, 'i16'];
export const battery_SOC: [number, string] = [1206, 'i16'];
export const battery_TEMP: [number, string] = [1208, 'u16'];
export const battery_Wh_Remaining: [number, string] = [1210, 'u16'];
export const battery_GIDS: [number, string] = [1212, 'u16'];
export const battery_MAX: [number, string] = [1214, 'u16'];
export const battery_Max_GIDS: [number, string] = [1216, 'u16'];
export const battery_StateOfHealth: [number, string] = [1218, 'u16'];
export const battery_Total_Voltage2: [number, string] = [1220, 'u16'];
export const battery_Current2: [number, string] = [1222, 'i16'];
export const battery_HistData_Temperature_MAX: [number, string] = [1224, 'i16'];
export const battery_HistData_Temperature_MIN: [number, string] = [1226, 'i16'];
export const battery_AverageTemperature: [number, string] = [1228, 'i16'];
export const battery_Relay_Cut_Request: [number, string] = [1230, 'u8'];
export const battery_Failsafe_Status: [number, string] = [1231, 'u8'];
export const battery_Interlock: [number, string] = [1232, 'b'];
export const battery_Full_CHARGE_flag: [number, string] = [1233, 'b'];
export const battery_MainRelayOn_flag: [number, string] = [1234, 'b'];
export const battery_Capacity_Empty: [number, string] = [1235, 'b'];
export const battery_HeatExist: [number, string] = [1236, 'b'];
export const battery_Heating_Stop: [number, string] = [1237, 'b'];
export const battery_Heating_Start: [number, string] = [1238, 'b'];
export const battery_Batt_Heater_Mail_Send_Request: [number, string] = [1239, 'b'];
export const battery_request_idx: [number, string] = [1240, 'u8'];
export const group_7bb: [number, string] = [1241, 'u8'];
export const group_7bb_length: [number, string] = [1242, 'u8'];
export const stop_battery_query: [number, string] = [1243, 'b'];
export const hold_off_with_polling_10seconds: [number, string] = [1244, 'u8'];
export const battery_cell_voltages: [number, string, number] = [1246, 'u16', 96];
export const battery_balancing_shunts: [number, string, number] = [1438, 'b', 96];
export const balancing_bitmap_prev: [number, string, number] = [1536, 'u32', 3];
export const balancing_bitmap_valid: [number, string] = [1548, 'b'];
export const balancing_unchanged_window: [number, string] = [1550, 'u16'];
export const balancing_window_fill: [number, string] = [1552, 'u8'];
export const balancing_low_reads: [number, string] = [1553, 'u8'];
export const balancing_frames_seen: [number, string] = [1554, 'u8'];
export const balancing_data_fresh: [number, string] = [1555, 'b'];
export const battery_cellcounter: [number, string] = [1556, 'u8'];
export const battery_min_max_voltage: [number, string, number] = [1558, 'u16', 2];
export const battery_HX_pptt: [number, string] = [1562, 'u16'];
export const battery_insulation: [number, string] = [1564, 'u16'];
export const battery_charge_count_qc: [number, string] = [1566, 'u16'];
export const battery_charge_count_l1l2: [number, string] = [1568, 'u16'];
export const battery_temp_raw_1: [number, string] = [1570, 'u16'];
export const battery_temp_raw_2_highnibble: [number, string] = [1572, 'u8'];
export const battery_temp_raw_2: [number, string] = [1574, 'u16'];
export const battery_temp_raw_3: [number, string] = [1576, 'u16'];
export const battery_temp_raw_4: [number, string] = [1578, 'u16'];
export const battery_temp_raw_max: [number, string] = [1580, 'u16'];
export const battery_temp_raw_min: [number, string] = [1582, 'u16'];
export const battery_temp_polled_max: [number, string] = [1584, 'i16'];
export const battery_temp_polled_min: [number, string] = [1586, 'i16'];
export const BatterySerialNumber: [number, string, number] = [1588, 'u8', 15];
export const BatteryPartNumber: [number, string, number] = [1603, 'u8', 7];
export const stateMachineClearSOH: [number, string] = [1610, 'u8'];
