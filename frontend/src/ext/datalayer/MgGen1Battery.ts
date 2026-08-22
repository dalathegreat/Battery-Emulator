// MgGen1Battery: 1448 bytes; base classes: UdsCanBattery@0
export const MG_GEN1_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
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
  ['batteryType', 'u32'],
  ['vehicleHardwareNumber', 'u32'],
  ['pid_f18a', 'u8', 8],
  ['pid_f120', 'u8', 16],
  ['pid_b18c', 'u8', 24],
  ['pid_fingerprint', 'u8', 10],
  ['pid_mfr_date', 'u8', 3],
  ['pid_vin', 'u8', 17],
  ['pid_vehicle_hw_number', 'u8', 5],
  ['pid_system_hw_number', 'u8', 10],
  ['pid_system_sw_number', 'u8', 10],
  ['pid_f1a2', 'u8', 8],
  ['pid_f1aa', 'u8', 5],
  ['allowed_contactor_closing', 'bool*'],
  ['announcedContactorsClosed', 'b'],
  ['contactorCloseReset', 'b'],
  ['eightAcycle', 'u8'],
  ['warmupCounter', 'u16'],
  ['previousMillis10', 'u32'],
  ['previousMillis20', 'u32'],
  ['maxChargePowerW', 'u16'],
  ['maxDischargePowerW', 'u16'],
  ['voltageAtCellMin', 'b'],
  ['voltageAtCellMax', 'b'],
  ['fastTick', 'b'],
  ['soc', 'u16'],
  ['tx_count', 'u32'],
  ['rx_count', 'u32'],
  ['cellVoltageValidTime', 'u16'],
  ['voltageValidTime', 'u16'],
  ['highestSeenCellCount', 'u16'],
  ['limit_message_counter', 'i32'],
  ['previousState', 'u8'],
  ['MG_HS_8A', 'CAN_frame'],
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
export const batteryType: [number, string] = [1196, 'u32'];
export const vehicleHardwareNumber: [number, string] = [1200, 'u32'];
export const pid_f18a: [number, string, number] = [1204, 'u8', 8];
export const pid_f120: [number, string, number] = [1212, 'u8', 16];
export const pid_b18c: [number, string, number] = [1228, 'u8', 24];
export const pid_fingerprint: [number, string, number] = [1252, 'u8', 10];
export const pid_mfr_date: [number, string, number] = [1262, 'u8', 3];
export const pid_vin: [number, string, number] = [1265, 'u8', 17];
export const pid_vehicle_hw_number: [number, string, number] = [1282, 'u8', 5];
export const pid_system_hw_number: [number, string, number] = [1287, 'u8', 10];
export const pid_system_sw_number: [number, string, number] = [1297, 'u8', 10];
export const pid_f1a2: [number, string, number] = [1307, 'u8', 8];
export const pid_f1aa: [number, string, number] = [1315, 'u8', 5];
export const allowed_contactor_closing: [number, string] = [1320, 'bool*'];
export const announcedContactorsClosed: [number, string] = [1324, 'b'];
export const contactorCloseReset: [number, string] = [1325, 'b'];
export const eightAcycle: [number, string] = [1326, 'u8'];
export const warmupCounter: [number, string] = [1328, 'u16'];
export const previousMillis10: [number, string] = [1332, 'u32'];
export const previousMillis20: [number, string] = [1336, 'u32'];
export const maxChargePowerW: [number, string] = [1340, 'u16'];
export const maxDischargePowerW: [number, string] = [1342, 'u16'];
export const voltageAtCellMin: [number, string] = [1344, 'b'];
export const voltageAtCellMax: [number, string] = [1345, 'b'];
export const fastTick: [number, string] = [1346, 'b'];
export const soc: [number, string] = [1348, 'u16'];
export const tx_count: [number, string] = [1352, 'u32'];
export const rx_count: [number, string] = [1356, 'u32'];
export const cellVoltageValidTime: [number, string] = [1360, 'u16'];
export const voltageValidTime: [number, string] = [1362, 'u16'];
export const highestSeenCellCount: [number, string] = [1364, 'u16'];
export const limit_message_counter: [number, string] = [1368, 'i32'];
export const previousState: [number, string] = [1372, 'u8'];
export const MG_HS_8A: [number, string] = [1376, 'CAN_frame'];
