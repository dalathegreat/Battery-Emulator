// SonoBattery: 200 bytes; base classes: CanBattery@0
export const SONO_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['seconds', 'u8'],
  ['functionalsafetybitmask', 'u8'],
  ['batteryVoltage', 'u16'],
  ['allowedDischargePower', 'u16'],
  ['allowedChargePower', 'u16'],
  ['CellVoltMax_mV', 'u16'],
  ['CellVoltMin_mV', 'u16'],
  ['batteryAmps', 'i16'],
  ['temperatureMin', 'i16'],
  ['temperatureMax', 'i16'],
  ['batterySOH', 'u8'],
  ['realSOC', 'u8'],
  ['', ' ', 4],
  ['SONO_400', 'CAN_frame'],
  ['SONO_401', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const previousMillis100: [number, string] = [24, 'u32'];
export const previousMillis1000: [number, string] = [28, 'u32'];
export const seconds: [number, string] = [32, 'u8'];
export const functionalsafetybitmask: [number, string] = [33, 'u8'];
export const batteryVoltage: [number, string] = [34, 'u16'];
export const allowedDischargePower: [number, string] = [36, 'u16'];
export const allowedChargePower: [number, string] = [38, 'u16'];
export const CellVoltMax_mV: [number, string] = [40, 'u16'];
export const CellVoltMin_mV: [number, string] = [42, 'u16'];
export const batteryAmps: [number, string] = [44, 'i16'];
export const temperatureMin: [number, string] = [46, 'i16'];
export const temperatureMax: [number, string] = [48, 'i16'];
export const batterySOH: [number, string] = [50, 'u8'];
export const realSOC: [number, string] = [51, 'u8'];
export const SONO_400: [number, string] = [56, 'CAN_frame'];
export const SONO_401: [number, string] = [128, 'CAN_frame'];
