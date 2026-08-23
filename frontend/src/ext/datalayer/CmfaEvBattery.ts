// CmfaEvBattery: 1752 bytes; base classes: UdsCanBattery@0
export const CMFA_EV_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['____vptr$Battery', '__vtbl_ptr_type*'],
  ['_sid', 'u8'],
  ['_data', 'u8', 16],
  ['___defaultRenderer', 'BatteryDefaultRenderer'],
  ['____vptr$Transmitter', '__vtbl_ptr_type*'],
  ['____vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['__can_interface', 'CAN_Interface'],
  ['_len', 'u8'],
  ['_timeout_ticks', 'u16'],
  ['__initial_speed', 'CAN_Speed'],
  ['_retries', 'u8'],
  ['_max_retries', 'u8'],
  ['_priority', 'UdsPriority'],
  ['__idx', 'i32'],
  ['__bs', 'u8'],
  ['___vptr$IsoTp', '__vtbl_ptr_type*'],
  ['__stmin', 'u8'],
  ['__len', 'i32'],
  ['___rx', 'TpCon'],
  ['__state', 'i32'],
  ['__bs_2', 'u8'],
  ['__sn', 'u8'],
  ['__buf', 'u8', 512],
  ['___tx', 'TpCon'],
  ['___rxfc', 'FcOpts'],
  ['___txfc', 'FcOpts'],
  ['___tx_wft', 'u8'],
  ['___tatype', 'isotp_tatype'],
  ['___rxtimer', 'u32'],
  ['___txtimer', 'u32'],
  ['___tx_id', 'u32'],
  ['___addrmode', 'isotp_addrmode'],
  ['___tx_addr', 'u8'],
  ['___rx_addr', 'u8'],
  ['_dtc', 'DATALAYER_BATTERY_DTC_TYPE*'],
  ['_uds_address', 'u16'],
  ['_uds_response_address', 'u16'],
  ['_uds_current_response_address', 'u16'],
  ['_seq_msg', '7787'],
  ['_previousUdsMillis100', 'u32'],
  ['_pid_list', '36b9*'],
  ['_pid_list_len', 'u16'],
  ['_pid_scan_index', 'u16'],
  ['_next_pid', 'u16'],
  ['_pending_pid', 'u16'],
  ['_pid_retries', 'u32'],
  ['_seq_state', 'u16'],
  ['_pending_seq_state', 'atomic<short unsigned int>'],
  ['_seq_pause_ticks', 'u16'],
  ['_seq_pause_level', 'UdsPriority'],
  ['_uds_transaction_timeout', 'i32'],
  ['_uds_renderer', 'UdsBatteryHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['CMFA_1EA', 'CAN_frame'],
  ['CMFA_125', 'CAN_frame'],
  ['CMFA_134', 'CAN_frame'],
  ['CMFA_135', 'CAN_frame'],
  ['CMFA_3D3', 'CAN_frame'],
  ['CMFA_59B', 'CAN_frame'],
  ['end_of_charge', 'b'],
  ['interlock_flag', 'b'],
  ['soc_z', 'u16'],
  ['soc_u', 'u16'],
  ['max_regen_power', 'u16'],
  ['max_discharge_power', 'u16'],
  ['average_temperature', 'i16'],
  ['minimum_temperature', 'i16'],
  ['maximum_temperature', 'i16'],
  ['maximum_charge_power', 'u16'],
  ['SOH_available_power', 'u16'],
  ['SOH_generated_power', 'u16'],
  ['average_voltage_of_cells', 'u32'],
  ['highest_cell_voltage_mv', 'u16'],
  ['lowest_cell_voltage_mv', 'u16'],
  ['lead_acid_voltage', 'u16'],
  ['highest_cell_voltage_number', 'u8'],
  ['lowest_cell_voltage_number', 'u8'],
  ['cumulative_energy_when_discharging', 'u32'],
  ['cumulative_energy_when_charging', 'u32'],
  ['cumulative_energy_in_regen', 'u32'],
  ['soh_average', 'u16'],
  ['counter_10ms', 'u8'],
  ['content_125', 'u8', 16],
  ['content_135', 'u8', 16],
  ['previousMillis100ms', 'u32'],
  ['previousMillis10ms', 'u32'],
  ['heartbeat', 'u8'],
  ['heartbeat2', 'u8'],
  ['SOC_raw', 'u32'],
  ['SOH', 'u16'],
  ['current_raw', 'i16'],
  ['pack_voltage', 'u16'],
  ['highest_cell_temperature', 'i16'],
  ['lowest_cell_temperature', 'i16'],
  ['discharge_power_w', 'u32'],
  ['charge_power_w', 'u32'],
];

export const ____vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const _sid: [number, string] = [0, 'u8'];
export const _data: [number, string, number] = [1, 'u8', 16];
export const ___defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ____vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ____vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const __can_interface: [number, string] = [16, 'CAN_Interface'];
export const _len: [number, string] = [17, 'u8'];
export const _timeout_ticks: [number, string] = [18, 'u16'];
export const __initial_speed: [number, string] = [20, 'CAN_Speed'];
export const _retries: [number, string] = [20, 'u8'];
export const _max_retries: [number, string] = [21, 'u8'];
export const _priority: [number, string] = [22, 'UdsPriority'];
export const __idx: [number, string] = [24, 'i32'];
export const __bs: [number, string] = [24, 'u8'];
export const ___vptr$IsoTp: [number, string] = [24, '__vtbl_ptr_type*'];
export const __stmin: [number, string] = [25, 'u8'];
export const __len: [number, string] = [28, 'i32'];
export const ___rx: [number, string] = [28, 'TpCon'];
export const __state: [number, string] = [32, 'i32'];
export const __bs_2: [number, string] = [36, 'u8'];
export const __sn: [number, string] = [37, 'u8'];
export const __buf: [number, string, number] = [38, 'u8', 512];
export const ___tx: [number, string] = [556, 'TpCon'];
export const ___rxfc: [number, string] = [1084, 'FcOpts'];
export const ___txfc: [number, string] = [1086, 'FcOpts'];
export const ___tx_wft: [number, string] = [1088, 'u8'];
export const ___tatype: [number, string] = [1092, 'isotp_tatype'];
export const ___rxtimer: [number, string] = [1096, 'u32'];
export const ___txtimer: [number, string] = [1100, 'u32'];
export const ___tx_id: [number, string] = [1104, 'u32'];
export const ___addrmode: [number, string] = [1108, 'isotp_addrmode'];
export const ___tx_addr: [number, string] = [1112, 'u8'];
export const ___rx_addr: [number, string] = [1113, 'u8'];
export const _dtc: [number, string] = [1116, 'DATALAYER_BATTERY_DTC_TYPE*'];
export const _uds_address: [number, string] = [1120, 'u16'];
export const _uds_response_address: [number, string] = [1122, 'u16'];
export const _uds_current_response_address: [number, string] = [1124, 'u16'];
export const _seq_msg: [number, string] = [1126, '7787'];
export const _previousUdsMillis100: [number, string] = [1152, 'u32'];
export const _pid_list: [number, string] = [1156, '36b9*'];
export const _pid_list_len: [number, string] = [1160, 'u16'];
export const _pid_scan_index: [number, string] = [1162, 'u16'];
export const _next_pid: [number, string] = [1164, 'u16'];
export const _pending_pid: [number, string] = [1166, 'u16'];
export const _pid_retries: [number, string] = [1168, 'u32'];
export const _seq_state: [number, string] = [1172, 'u16'];
export const _pending_seq_state: [number, string] = [1174, 'atomic<short unsigned int>'];
export const _seq_pause_ticks: [number, string] = [1176, 'u16'];
export const _seq_pause_level: [number, string] = [1178, 'UdsPriority'];
export const _uds_transaction_timeout: [number, string] = [1180, 'i32'];
export const _uds_renderer: [number, string] = [1184, 'UdsBatteryHtmlRenderer'];
export const datalayer_battery: [number, string] = [1192, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [1196, 'bool*'];
export const CMFA_1EA: [number, string] = [1200, 'CAN_frame'];
export const CMFA_125: [number, string] = [1272, 'CAN_frame'];
export const CMFA_134: [number, string] = [1344, 'CAN_frame'];
export const CMFA_135: [number, string] = [1416, 'CAN_frame'];
export const CMFA_3D3: [number, string] = [1488, 'CAN_frame'];
export const CMFA_59B: [number, string] = [1560, 'CAN_frame'];
export const end_of_charge: [number, string] = [1632, 'b'];
export const interlock_flag: [number, string] = [1633, 'b'];
export const soc_z: [number, string] = [1634, 'u16'];
export const soc_u: [number, string] = [1636, 'u16'];
export const max_regen_power: [number, string] = [1638, 'u16'];
export const max_discharge_power: [number, string] = [1640, 'u16'];
export const average_temperature: [number, string] = [1642, 'i16'];
export const minimum_temperature: [number, string] = [1644, 'i16'];
export const maximum_temperature: [number, string] = [1646, 'i16'];
export const maximum_charge_power: [number, string] = [1648, 'u16'];
export const SOH_available_power: [number, string] = [1650, 'u16'];
export const SOH_generated_power: [number, string] = [1652, 'u16'];
export const average_voltage_of_cells: [number, string] = [1656, 'u32'];
export const highest_cell_voltage_mv: [number, string] = [1660, 'u16'];
export const lowest_cell_voltage_mv: [number, string] = [1662, 'u16'];
export const lead_acid_voltage: [number, string] = [1664, 'u16'];
export const highest_cell_voltage_number: [number, string] = [1666, 'u8'];
export const lowest_cell_voltage_number: [number, string] = [1667, 'u8'];
export const cumulative_energy_when_discharging: [number, string] = [1668, 'u32'];
export const cumulative_energy_when_charging: [number, string] = [1672, 'u32'];
export const cumulative_energy_in_regen: [number, string] = [1676, 'u32'];
export const soh_average: [number, string] = [1680, 'u16'];
export const counter_10ms: [number, string] = [1682, 'u8'];
export const content_125: [number, string, number] = [1683, 'u8', 16];
export const content_135: [number, string, number] = [1699, 'u8', 16];
export const previousMillis100ms: [number, string] = [1716, 'u32'];
export const previousMillis10ms: [number, string] = [1720, 'u32'];
export const heartbeat: [number, string] = [1724, 'u8'];
export const heartbeat2: [number, string] = [1725, 'u8'];
export const SOC_raw: [number, string] = [1728, 'u32'];
export const SOH: [number, string] = [1732, 'u16'];
export const current_raw: [number, string] = [1734, 'i16'];
export const pack_voltage: [number, string] = [1736, 'u16'];
export const highest_cell_temperature: [number, string] = [1738, 'i16'];
export const lowest_cell_temperature: [number, string] = [1740, 'i16'];
export const discharge_power_w: [number, string] = [1744, 'u32'];
export const charge_power_w: [number, string] = [1748, 'u32'];
