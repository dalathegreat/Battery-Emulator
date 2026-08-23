// RenaultKangooBattery: 384 bytes; base classes: CanBattery@0
export const RENAULT_KANGOO_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['LB_Battery_Voltage', 'u32'],
  ['LB_Charge_Power_Limit_Watts', 'u32'],
  ['LB_MaxChargeAllowed_W', 'u32'],
  ['LB_Current', 'i32'],
  ['LB_MAX_TEMPERATURE', 'i16'],
  ['LB_MIN_TEMPERATURE', 'i16'],
  ['LB_SOC', 'u16'],
  ['LB_SOH', 'u16'],
  ['LB_Discharge_Power_Limit', 'u16'],
  ['LB_Charge_Power_Limit', 'u16'],
  ['LB_kWh_Remaining', 'u16'],
  ['LB_Cell_Max_Voltage', 'u16'],
  ['LB_Cell_Min_Voltage', 'u16'],
  ['LB_Discharge_Power_Limit_Byte1', 'u8'],
  ['GVI_Pollcounter', 'u8'],
  ['LB_EOCR', 'u8'],
  ['LB_HVBUV', 'u8'],
  ['LB_HVBIR', 'u8'],
  ['LB_CUV', 'u8'],
  ['LB_COV', 'u8'],
  ['LB_HVBOV', 'u8'],
  ['LB_HVBOT', 'u8'],
  ['LB_HVBOC', 'u8'],
  ['LB_MaxInput_kW', 'u8'],
  ['LB_MaxOutput_kW', 'u8'],
  ['pollgroup', 'u8'],
  ['', ' ', 5],
  ['KANGOO_423', 'CAN_frame'],
  ['KANGOO_79B_Poll', 'CAN_frame'],
  ['KANGOO_79B_Continue', 'CAN_frame'],
  ['KANGOO_CLEAR_DTC', 'CAN_frame'],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['GVL_pause', 'u32'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const LB_Battery_Voltage: [number, string] = [28, 'u32'];
export const LB_Charge_Power_Limit_Watts: [number, string] = [32, 'u32'];
export const LB_MaxChargeAllowed_W: [number, string] = [36, 'u32'];
export const LB_Current: [number, string] = [40, 'i32'];
export const LB_MAX_TEMPERATURE: [number, string] = [44, 'i16'];
export const LB_MIN_TEMPERATURE: [number, string] = [46, 'i16'];
export const LB_SOC: [number, string] = [48, 'u16'];
export const LB_SOH: [number, string] = [50, 'u16'];
export const LB_Discharge_Power_Limit: [number, string] = [52, 'u16'];
export const LB_Charge_Power_Limit: [number, string] = [54, 'u16'];
export const LB_kWh_Remaining: [number, string] = [56, 'u16'];
export const LB_Cell_Max_Voltage: [number, string] = [58, 'u16'];
export const LB_Cell_Min_Voltage: [number, string] = [60, 'u16'];
export const LB_Discharge_Power_Limit_Byte1: [number, string] = [62, 'u8'];
export const GVI_Pollcounter: [number, string] = [63, 'u8'];
export const LB_EOCR: [number, string] = [64, 'u8'];
export const LB_HVBUV: [number, string] = [65, 'u8'];
export const LB_HVBIR: [number, string] = [66, 'u8'];
export const LB_CUV: [number, string] = [67, 'u8'];
export const LB_COV: [number, string] = [68, 'u8'];
export const LB_HVBOV: [number, string] = [69, 'u8'];
export const LB_HVBOT: [number, string] = [70, 'u8'];
export const LB_HVBOC: [number, string] = [71, 'u8'];
export const LB_MaxInput_kW: [number, string] = [72, 'u8'];
export const LB_MaxOutput_kW: [number, string] = [73, 'u8'];
export const pollgroup: [number, string] = [74, 'u8'];
export const KANGOO_423: [number, string] = [80, 'CAN_frame'];
export const KANGOO_79B_Poll: [number, string] = [152, 'CAN_frame'];
export const KANGOO_79B_Continue: [number, string] = [224, 'CAN_frame'];
export const KANGOO_CLEAR_DTC: [number, string] = [296, 'CAN_frame'];
export const previousMillis10: [number, string] = [368, 'u32'];
export const previousMillis100: [number, string] = [372, 'u32'];
export const previousMillis1000: [number, string] = [376, 'u32'];
export const GVL_pause: [number, string] = [380, 'u32'];
