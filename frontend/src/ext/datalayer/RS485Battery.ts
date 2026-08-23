// RS485Battery: 16 bytes; base classes: Battery@0, Transmitter@8, Rs485Receiver@12
export const RS485_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['__vptr$Battery', '__vtbl_ptr_type*'],
  ['_defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['__vptr$Transmitter', '__vtbl_ptr_type*'],
  ['__vptr$Rs485Receiver', '__vtbl_ptr_type*'],
];

export const __vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const _defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const __vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const __vptr$Rs485Receiver: [number, string] = [12, '__vtbl_ptr_type*'];
