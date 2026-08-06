// RenaultTwizyBattery: 84 bytes; base classes: CanBattery@0
export const RENAULT_TWIZY_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['cell_temperatures_dC', 'i16', 7],
  ['current_dA', 'i16'],
  ['voltage_dV', 'u16'],
  ['cellvoltages_mV', 'i16', 14],
  ['max_discharge_power', 'i16'],
  ['max_recup_power', 'i16'],
  ['max_charge_power', 'i16'],
  ['SOC', 'u16'],
  ['SOH', 'u16'],
  ['remaining_capacity_Wh', 'u16'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const cell_temperatures_dC: [number, string, number] = [24, 'i16', 7];
export const current_dA: [number, string] = [38, 'i16'];
export const voltage_dV: [number, string] = [40, 'u16'];
export const cellvoltages_mV: [number, string, number] = [42, 'i16', 14];
export const max_discharge_power: [number, string] = [70, 'i16'];
export const max_recup_power: [number, string] = [72, 'i16'];
export const max_charge_power: [number, string] = [74, 'i16'];
export const SOC: [number, string] = [76, 'u16'];
export const SOH: [number, string] = [78, 'u16'];
export const remaining_capacity_Wh: [number, string] = [80, 'u16'];
