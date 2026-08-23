// SamsungSdiLVBattery: 52 bytes; base classes: CanBattery@0
export const SAMSUNG_SDI_LVBATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['system_voltage', 'u16'],
  ['system_current', 'i16'],
  ['system_SOC', 'u8'],
  ['system_SOH', 'u8'],
  ['battery_charge_voltage', 'u16'],
  ['charge_current_limit', 'u16'],
  ['discharge_current_limit', 'u16'],
  ['battery_discharge_voltage', 'u16'],
  ['alarms_frame0', 'u8'],
  ['alarms_frame1', 'u8'],
  ['protection_frame2', 'u8'],
  ['protection_frame3', 'u8'],
  ['maximum_cell_voltage', 'u16'],
  ['minimum_cell_voltage', 'u16'],
  ['maximum_cell_temperature', 'i8'],
  ['minimum_cell_temperature', 'i8'],
  ['system_permanent_failure_status_dry_contact', 'u8'],
  ['system_permanent_failure_status_fuse_open', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const system_voltage: [number, string] = [24, 'u16'];
export const system_current: [number, string] = [26, 'i16'];
export const system_SOC: [number, string] = [28, 'u8'];
export const system_SOH: [number, string] = [29, 'u8'];
export const battery_charge_voltage: [number, string] = [30, 'u16'];
export const charge_current_limit: [number, string] = [32, 'u16'];
export const discharge_current_limit: [number, string] = [34, 'u16'];
export const battery_discharge_voltage: [number, string] = [36, 'u16'];
export const alarms_frame0: [number, string] = [38, 'u8'];
export const alarms_frame1: [number, string] = [39, 'u8'];
export const protection_frame2: [number, string] = [40, 'u8'];
export const protection_frame3: [number, string] = [41, 'u8'];
export const maximum_cell_voltage: [number, string] = [42, 'u16'];
export const minimum_cell_voltage: [number, string] = [44, 'u16'];
export const maximum_cell_temperature: [number, string] = [46, 'i8'];
export const minimum_cell_temperature: [number, string] = [47, 'i8'];
export const system_permanent_failure_status_dry_contact: [number, string] = [48, 'u8'];
export const system_permanent_failure_status_fuse_open: [number, string] = [49, 'u8'];
