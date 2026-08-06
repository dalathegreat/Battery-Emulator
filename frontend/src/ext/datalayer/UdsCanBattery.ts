// UdsCanBattery: 1192 bytes; base classes: CanBattery@0, IsoTp@24
export const UDS_CAN_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['sid', 'u8'],
  ['data', 'u8', 16],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['len', 'u8'],
  ['timeout_ticks', 'u16'],
  ['_initial_speed', 'CAN_Speed'],
  ['retries', 'u8'],
  ['max_retries', 'u8'],
  ['priority', 'UdsPriority'],
  ['_idx', 'i32'],
  ['_bs', 'u8'],
  ['__vptr$IsoTp', '__vtbl_ptr_type*'],
  ['_stmin', 'u8'],
  ['_len', 'i32'],
  ['__rx', 'TpCon'],
  ['_state', 'i32'],
  ['_bs_2', 'u8'],
  ['_sn', 'u8'],
  ['_buf', 'u8', 512],
  ['__tx', 'TpCon'],
  ['__rxfc', 'FcOpts'],
  ['__txfc', 'FcOpts'],
  ['__tx_wft', 'u8'],
  ['__tatype', 'isotp_tatype'],
  ['__rxtimer', 'u32'],
  ['__txtimer', 'u32'],
  ['__tx_id', 'u32'],
  ['__addrmode', 'isotp_addrmode'],
  ['__tx_addr', 'u8'],
  ['__rx_addr', 'u8'],
  ['dtc', 'DATALAYER_BATTERY_DTC_TYPE*'],
  ['uds_address', 'u16'],
  ['uds_response_address', 'u16'],
  ['uds_current_response_address', 'u16'],
  ['seq_msg', '7787'],
  ['previousUdsMillis100', 'u32'],
  ['pid_list', '36b9*'],
  ['pid_list_len', 'u16'],
  ['pid_scan_index', 'u16'],
  ['next_pid', 'u16'],
  ['pending_pid', 'u16'],
  ['pid_retries', 'u32'],
  ['seq_state', 'u16'],
  ['pending_seq_state', 'atomic<short unsigned int>'],
  ['seq_pause_ticks', 'u16'],
  ['seq_pause_level', 'UdsPriority'],
  ['uds_transaction_timeout', 'i32'],
  ['uds_renderer', 'UdsBatteryHtmlRenderer'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const sid: [number, string] = [0, 'u8'];
export const data: [number, string, number] = [1, 'u8', 16];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const len: [number, string] = [17, 'u8'];
export const timeout_ticks: [number, string] = [18, 'u16'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const retries: [number, string] = [20, 'u8'];
export const max_retries: [number, string] = [21, 'u8'];
export const priority: [number, string] = [22, 'UdsPriority'];
export const _idx: [number, string] = [24, 'i32'];
export const _bs: [number, string] = [24, 'u8'];
export const __vptr$IsoTp: [number, string] = [24, '__vtbl_ptr_type*'];
export const _stmin: [number, string] = [25, 'u8'];
export const _len: [number, string] = [28, 'i32'];
export const __rx: [number, string] = [28, 'TpCon'];
export const _state: [number, string] = [32, 'i32'];
export const _bs_2: [number, string] = [36, 'u8'];
export const _sn: [number, string] = [37, 'u8'];
export const _buf: [number, string, number] = [38, 'u8', 512];
export const __tx: [number, string] = [556, 'TpCon'];
export const __rxfc: [number, string] = [1084, 'FcOpts'];
export const __txfc: [number, string] = [1086, 'FcOpts'];
export const __tx_wft: [number, string] = [1088, 'u8'];
export const __tatype: [number, string] = [1092, 'isotp_tatype'];
export const __rxtimer: [number, string] = [1096, 'u32'];
export const __txtimer: [number, string] = [1100, 'u32'];
export const __tx_id: [number, string] = [1104, 'u32'];
export const __addrmode: [number, string] = [1108, 'isotp_addrmode'];
export const __tx_addr: [number, string] = [1112, 'u8'];
export const __rx_addr: [number, string] = [1113, 'u8'];
export const dtc: [number, string] = [1116, 'DATALAYER_BATTERY_DTC_TYPE*'];
export const uds_address: [number, string] = [1120, 'u16'];
export const uds_response_address: [number, string] = [1122, 'u16'];
export const uds_current_response_address: [number, string] = [1124, 'u16'];
export const seq_msg: [number, string] = [1126, '7787'];
export const previousUdsMillis100: [number, string] = [1152, 'u32'];
export const pid_list: [number, string] = [1156, '36b9*'];
export const pid_list_len: [number, string] = [1160, 'u16'];
export const pid_scan_index: [number, string] = [1162, 'u16'];
export const next_pid: [number, string] = [1164, 'u16'];
export const pending_pid: [number, string] = [1166, 'u16'];
export const pid_retries: [number, string] = [1168, 'u32'];
export const seq_state: [number, string] = [1172, 'u16'];
export const pending_seq_state: [number, string] = [1174, 'atomic<short unsigned int>'];
export const seq_pause_ticks: [number, string] = [1176, 'u16'];
export const seq_pause_level: [number, string] = [1178, 'UdsPriority'];
export const uds_transaction_timeout: [number, string] = [1180, 'i32'];
export const uds_renderer: [number, string] = [1184, 'UdsBatteryHtmlRenderer'];
