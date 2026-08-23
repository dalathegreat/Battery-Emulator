// SimpBmsBattery: 56 bytes; base classes: CanBattery@0
export const SIMP_BMS_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['previousMillis1000', 'u32'],
  ['celltemperature_max_dC', 'i16'],
  ['celltemperature_min_dC', 'i16'],
  ['current_mA', 'i16'],
  ['voltage_dV', 'u16'],
  ['cellvoltage_max_mV', 'u16'],
  ['cellvoltage_min_mV', 'u16'],
  ['charge_cutoff_voltage', 'u16'],
  ['discharge_cutoff_voltage', 'u16'],
  ['max_charge_current', 'i16'],
  ['max_discharge_current', 'i16'],
  ['ensemble_info_ack', 'u8'],
  ['cells_in_series', 'u8'],
  ['voltage_level', 'u8'],
  ['ah_total', 'u8'],
  ['SOC', 'u8'],
  ['SOH', 'u8'],
  ['charge_forbidden', 'u8'],
  ['discharge_forbidden', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const previousMillis1000: [number, string] = [24, 'u32'];
export const celltemperature_max_dC: [number, string] = [28, 'i16'];
export const celltemperature_min_dC: [number, string] = [30, 'i16'];
export const current_mA: [number, string] = [32, 'i16'];
export const voltage_dV: [number, string] = [34, 'u16'];
export const cellvoltage_max_mV: [number, string] = [36, 'u16'];
export const cellvoltage_min_mV: [number, string] = [38, 'u16'];
export const charge_cutoff_voltage: [number, string] = [40, 'u16'];
export const discharge_cutoff_voltage: [number, string] = [42, 'u16'];
export const max_charge_current: [number, string] = [44, 'i16'];
export const max_discharge_current: [number, string] = [46, 'i16'];
export const ensemble_info_ack: [number, string] = [48, 'u8'];
export const cells_in_series: [number, string] = [49, 'u8'];
export const voltage_level: [number, string] = [50, 'u8'];
export const ah_total: [number, string] = [51, 'u8'];
export const SOC: [number, string] = [52, 'u8'];
export const SOH: [number, string] = [53, 'u8'];
export const charge_forbidden: [number, string] = [54, 'u8'];
export const discharge_forbidden: [number, string] = [55, 'u8'];
