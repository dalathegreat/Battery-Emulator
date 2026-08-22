// VolvoSpaHybridBattery: 1104 bytes; base classes: CanBattery@0
export const VOLVO_SPA_HYBRID_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'VolvoSpaHybridHtmlRenderer'],
  ['', ' ', 4],
  ['previousMillis100', 'u32'],
  ['previousMillis1s', 'u32'],
  ['previousMillis60s', 'u32'],
  ['BATT_U', 'f'],
  ['MAX_U', 'f'],
  ['MIN_U', 'f'],
  ['BATT_I', 'f'],
  ['CHARGE_ENERGY', 'i32'],
  ['BATT_ERR_INDICATION', 'u8'],
  ['', ' ', 3],
  ['BATT_T_MAX', 'f'],
  ['BATT_T_MIN', 'f'],
  ['BATT_T_AVG', 'f'],
  ['SOC_BMS', 'u16'],
  ['SOC_CALC', 'u16'],
  ['CELL_U_MAX', 'u16'],
  ['CELL_U_MIN', 'u16'],
  ['CELL_ID_U_MAX', 'u8'],
  ['', ' '],
  ['HvBattPwrLimDchaSoft', 'u16'],
  ['HvBattPwrLimDcha1', 'u16'],
  ['battery_request_idx', 'u8'],
  ['rxConsecutiveFrames', 'u8'],
  ['min_max_voltage', 'u16', 2],
  ['cellcounter', 'u8'],
  ['', ' ', 3],
  ['remaining_capacity', 'u32'],
  ['cell_voltages', 'u16', 102],
  ['startedUp', 'b'],
  ['DTC_reset_counter', 'u8'],
  ['', ' ', 2],
  ['VOLVO_536', 'CAN_frame'],
  ['VOLVO_140_CLOSE', 'CAN_frame'],
  ['VOLVO_140_OPEN', 'CAN_frame'],
  ['VOLVO_372', 'CAN_frame'],
  ['VOLVO_CELL_U_Req', 'CAN_frame'],
  ['VOLVO_FlowControl', 'CAN_frame'],
  ['VOLVO_SOH_Req', 'CAN_frame'],
  ['VOLVO_BECMsupplyVoltage_Req', 'CAN_frame'],
  ['VOLVO_DTC_Erase', 'CAN_frame'],
  ['VOLVO_BECM_ECUreset', 'CAN_frame'],
  ['VOLVO_DTCreadout', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'VolvoSpaHybridHtmlRenderer'];
export const previousMillis100: [number, string] = [28, 'u32'];
export const previousMillis1s: [number, string] = [32, 'u32'];
export const previousMillis60s: [number, string] = [36, 'u32'];
export const BATT_U: [number, string] = [40, 'f'];
export const MAX_U: [number, string] = [44, 'f'];
export const MIN_U: [number, string] = [48, 'f'];
export const BATT_I: [number, string] = [52, 'f'];
export const CHARGE_ENERGY: [number, string] = [56, 'i32'];
export const BATT_ERR_INDICATION: [number, string] = [60, 'u8'];
export const BATT_T_MAX: [number, string] = [64, 'f'];
export const BATT_T_MIN: [number, string] = [68, 'f'];
export const BATT_T_AVG: [number, string] = [72, 'f'];
export const SOC_BMS: [number, string] = [76, 'u16'];
export const SOC_CALC: [number, string] = [78, 'u16'];
export const CELL_U_MAX: [number, string] = [80, 'u16'];
export const CELL_U_MIN: [number, string] = [82, 'u16'];
export const CELL_ID_U_MAX: [number, string] = [84, 'u8'];
export const HvBattPwrLimDchaSoft: [number, string] = [86, 'u16'];
export const HvBattPwrLimDcha1: [number, string] = [88, 'u16'];
export const battery_request_idx: [number, string] = [90, 'u8'];
export const rxConsecutiveFrames: [number, string] = [91, 'u8'];
export const min_max_voltage: [number, string, number] = [92, 'u16', 2];
export const cellcounter: [number, string] = [96, 'u8'];
export const remaining_capacity: [number, string] = [100, 'u32'];
export const cell_voltages: [number, string, number] = [104, 'u16', 102];
export const startedUp: [number, string] = [308, 'b'];
export const DTC_reset_counter: [number, string] = [309, 'u8'];
export const VOLVO_536: [number, string] = [312, 'CAN_frame'];
export const VOLVO_140_CLOSE: [number, string] = [384, 'CAN_frame'];
export const VOLVO_140_OPEN: [number, string] = [456, 'CAN_frame'];
export const VOLVO_372: [number, string] = [528, 'CAN_frame'];
export const VOLVO_CELL_U_Req: [number, string] = [600, 'CAN_frame'];
export const VOLVO_FlowControl: [number, string] = [672, 'CAN_frame'];
export const VOLVO_SOH_Req: [number, string] = [744, 'CAN_frame'];
export const VOLVO_BECMsupplyVoltage_Req: [number, string] = [816, 'CAN_frame'];
export const VOLVO_DTC_Erase: [number, string] = [888, 'CAN_frame'];
export const VOLVO_BECM_ECUreset: [number, string] = [960, 'CAN_frame'];
export const VOLVO_DTCreadout: [number, string] = [1032, 'CAN_frame'];
