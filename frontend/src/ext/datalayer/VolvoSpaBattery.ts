// VolvoSpaBattery: 1208 bytes; base classes: CanBattery@0
export const VOLVO_SPA_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'VolvoSpaHtmlRenderer'],
  ['', ' ', 8],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestBECMecuReset', 'b'],
  ['dtc_buffer', 'u8', 131],
  ['dtc_rx_expected', 'u16'],
  ['dtc_rx_len', 'u16'],
  ['dtc_rx_active', 'b'],
  ['dtc_read_in_progress', 'b'],
  ['cell_voltage_read_in_progress', 'b'],
  ['', ' ', 3],
  ['dtc_request_millis', 'u32'],
  ['dtc_clear_in_progress', 'b'],
  ['', ' ', 3],
  ['dtc_clear_millis', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis1s', 'u32'],
  ['previousMillis60s', 'u32'],
  ['CHARGE_ENERGY', 'i32'],
  ['BATT_U', 'u16'],
  ['MAX_U', 'u16'],
  ['MIN_U', 'u16'],
  ['BATT_I', 'i16'],
  ['BATT_ERR_INDICATION', 'u8'],
  ['', ' '],
  ['BATT_T_MAX', 'i16'],
  ['BATT_T_MIN', 'i16'],
  ['BATT_T_AVG', 'i16'],
  ['SOC_BMS', 'u16'],
  ['CELL_U_MAX', 'u16'],
  ['CELL_U_MIN', 'u16'],
  ['CELL_ID_U_MAX', 'u8'],
  ['', ' '],
  ['BECMsupplyVoltage', 'u16'],
  ['HvBattPwrLimDchaSoft', 'u16'],
  ['HvBattPwrLimDcha1', 'u16'],
  ['HvBattPwrLimDchaSlowAgi', 'u16'],
  ['HvBattPwrLimChrgSlowAgi', 'u16'],
  ['batteryModuleNumber', 'u8'],
  ['battery_request_idx', 'u8'],
  ['rxConsecutiveFrames', 'b'],
  ['', ' '],
  ['min_max_voltage', 'u16', 2],
  ['cellcounter', 'u8'],
  ['', ' '],
  ['cell_voltages', 'u16', 108],
  ['startedUp', 'b'],
  ['DTC_reset_counter', 'u8'],
  ['incoming_poll', 'u16'],
  ['currentpoll', 'u16'],
  ['poll_index', 'u8'],
  ['', ' '],
  ['poll_commands', '9638'],
  ['', ' ', 12],
  ['VOLVO_536', 'CAN_frame'],
  ['VOLVO_140_CLOSE', 'CAN_frame'],
  ['VOLVO_140_OPEN', 'CAN_frame'],
  ['VOLVO_372', 'CAN_frame'],
  ['VOLVO_CELL_U_Req', 'CAN_frame'],
  ['VOLVO_FlowControl', 'CAN_frame'],
  ['VOLVO_Poll_frame', 'CAN_frame'],
  ['VOLVO_BECM_ECUreset', 'CAN_frame'],
  ['VOLVO_DTC_Erase', 'CAN_frame'],
  ['VOLVO_DTCreadout', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'VolvoSpaHtmlRenderer'];
export const UserRequestDTCreset: [number, string] = [32, 'b'];
export const UserRequestDTCreadout: [number, string] = [33, 'b'];
export const UserRequestBECMecuReset: [number, string] = [34, 'b'];
export const dtc_buffer: [number, string, number] = [35, 'u8', 131];
export const dtc_rx_expected: [number, string] = [166, 'u16'];
export const dtc_rx_len: [number, string] = [168, 'u16'];
export const dtc_rx_active: [number, string] = [170, 'b'];
export const dtc_read_in_progress: [number, string] = [171, 'b'];
export const cell_voltage_read_in_progress: [number, string] = [172, 'b'];
export const dtc_request_millis: [number, string] = [176, 'u32'];
export const dtc_clear_in_progress: [number, string] = [180, 'b'];
export const dtc_clear_millis: [number, string] = [184, 'u32'];
export const previousMillis100: [number, string] = [188, 'u32'];
export const previousMillis500: [number, string] = [192, 'u32'];
export const previousMillis1s: [number, string] = [196, 'u32'];
export const previousMillis60s: [number, string] = [200, 'u32'];
export const CHARGE_ENERGY: [number, string] = [204, 'i32'];
export const BATT_U: [number, string] = [208, 'u16'];
export const MAX_U: [number, string] = [210, 'u16'];
export const MIN_U: [number, string] = [212, 'u16'];
export const BATT_I: [number, string] = [214, 'i16'];
export const BATT_ERR_INDICATION: [number, string] = [216, 'u8'];
export const BATT_T_MAX: [number, string] = [218, 'i16'];
export const BATT_T_MIN: [number, string] = [220, 'i16'];
export const BATT_T_AVG: [number, string] = [222, 'i16'];
export const SOC_BMS: [number, string] = [224, 'u16'];
export const CELL_U_MAX: [number, string] = [226, 'u16'];
export const CELL_U_MIN: [number, string] = [228, 'u16'];
export const CELL_ID_U_MAX: [number, string] = [230, 'u8'];
export const BECMsupplyVoltage: [number, string] = [232, 'u16'];
export const HvBattPwrLimDchaSoft: [number, string] = [234, 'u16'];
export const HvBattPwrLimDcha1: [number, string] = [236, 'u16'];
export const HvBattPwrLimDchaSlowAgi: [number, string] = [238, 'u16'];
export const HvBattPwrLimChrgSlowAgi: [number, string] = [240, 'u16'];
export const batteryModuleNumber: [number, string] = [242, 'u8'];
export const battery_request_idx: [number, string] = [243, 'u8'];
export const rxConsecutiveFrames: [number, string] = [244, 'b'];
export const min_max_voltage: [number, string, number] = [246, 'u16', 2];
export const cellcounter: [number, string] = [250, 'u8'];
export const cell_voltages: [number, string, number] = [252, 'u16', 108];
export const startedUp: [number, string] = [468, 'b'];
export const DTC_reset_counter: [number, string] = [469, 'u8'];
export const incoming_poll: [number, string] = [470, 'u16'];
export const currentpoll: [number, string] = [472, 'u16'];
export const poll_index: [number, string] = [474, 'u8'];
export const poll_commands: [number, string] = [476, '9638'];
export const VOLVO_536: [number, string] = [488, 'CAN_frame'];
export const VOLVO_140_CLOSE: [number, string] = [560, 'CAN_frame'];
export const VOLVO_140_OPEN: [number, string] = [632, 'CAN_frame'];
export const VOLVO_372: [number, string] = [704, 'CAN_frame'];
export const VOLVO_CELL_U_Req: [number, string] = [776, 'CAN_frame'];
export const VOLVO_FlowControl: [number, string] = [848, 'CAN_frame'];
export const VOLVO_Poll_frame: [number, string] = [920, 'CAN_frame'];
export const VOLVO_BECM_ECUreset: [number, string] = [992, 'CAN_frame'];
export const VOLVO_DTC_Erase: [number, string] = [1064, 'CAN_frame'];
export const VOLVO_DTCreadout: [number, string] = [1136, 'CAN_frame'];
