// CanBattery: 24 bytes; base classes: Battery@0, Transmitter@8, CanReceiver@12
export const CAN_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['__vptr$Battery', '__vtbl_ptr_type*'],
  ['_defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['__vptr$Transmitter', '__vtbl_ptr_type*'],
  ['__vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['can_interface', 'CAN_Interface'],
  ['initial_speed', 'CAN_Speed'],
];

export const __vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const _defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const __vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const __vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const can_interface: [number, string] = [16, 'CAN_Interface'];
export const initial_speed: [number, string] = [20, 'CAN_Speed'];
