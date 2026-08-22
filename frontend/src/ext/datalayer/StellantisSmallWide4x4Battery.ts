// StellantisSmallWide4x4Battery: 200 bytes; base classes: CanBattery@0
export const STELLANTIS_SMALL_WIDE4X4_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['previousMillis20', 'u32'],
  ['counter212', 'u8'],
  ['', ' ', 7],
  ['SMALLWIDE_212', 'CAN_frame'],
  ['CLEAR_DTC', 'CAN_frame'],
  ['TracBat_EChrgPowLong', 'u8'],
  ['HVBatDischrgPow30sec', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const datalayer_battery: [number, string] = [24, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [28, 'bool*'];
export const UserRequestDTCreset: [number, string] = [32, 'b'];
export const previousMillis20: [number, string] = [36, 'u32'];
export const counter212: [number, string] = [40, 'u8'];
export const SMALLWIDE_212: [number, string] = [48, 'CAN_frame'];
export const CLEAR_DTC: [number, string] = [120, 'CAN_frame'];
export const TracBat_EChrgPowLong: [number, string] = [192, 'u8'];
export const HVBatDischrgPow30sec: [number, string] = [193, 'u8'];
