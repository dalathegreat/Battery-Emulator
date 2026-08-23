// RivianBattery: 520 bytes; base classes: CanBattery@0
export const RIVIAN_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'RivianHtmlRenderer'],
  ['', ' ', 4],
  ['BMS_state', 'u8'],
  ['', ' '],
  ['pre_contactor_voltage', 'u16'],
  ['main_contactor_voltage', 'u16'],
  ['voltage_reference', 'u16'],
  ['DCFC_contactor_voltage', 'u16'],
  ['battery_SOC_average', 'u16'],
  ['battery_SOC_max', 'u16'],
  ['battery_SOC_min', 'u16'],
  ['battery_current', 'i32'],
  ['kWh_available_total', 'u16'],
  ['kWh_available_max', 'u16'],
  ['battery_min_temperature', 'i16'],
  ['battery_max_temperature', 'i16'],
  ['battery_discharge_limit_amp', 'u16'],
  ['battery_charge_limit_amp', 'u16'],
  ['cell_min_voltage_mV', 'u16'],
  ['cell_max_voltage_mV', 'u16'],
  ['error_flags_from_BMS', 'u8'],
  ['contactor_state', 'u8'],
  ['HVIL', 'u8'],
  ['HMI_part1', 'u8'],
  ['HMI_part2', 'u8'],
  ['isolation_fault_status', 'u8'],
  ['system_safe_state', 'u8'],
  ['error_relay_open', 'b'],
  ['IsolationMeasurementOngoing', 'b'],
  ['battery_thermal_runaway', 'b'],
  ['puncture_fault', 'b'],
  ['liquid_fault', 'b'],
  ['contactor_DCFC_welded', 'b'],
  ['NACS_charger_detected', 'b'],
  ['slewrate_potential_violation', 'b'],
  ['minimum_power_potential_violation', 'b'],
  ['operation_limit_violation_warning', 'b'],
  ['', ' ', 3],
  ['previousMillis10', 'u32'],
  ['RIVIAN_150', 'CAN_frame'],
  ['RIVIAN_420', 'CAN_frame'],
  ['RIVIAN_41F', 'CAN_frame'],
  ['RIVIAN_245', 'CAN_frame'],
  ['RIVIAN_200', 'CAN_frame'],
  ['RIVIAN_207', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'RivianHtmlRenderer'];
export const BMS_state: [number, string] = [28, 'u8'];
export const pre_contactor_voltage: [number, string] = [30, 'u16'];
export const main_contactor_voltage: [number, string] = [32, 'u16'];
export const voltage_reference: [number, string] = [34, 'u16'];
export const DCFC_contactor_voltage: [number, string] = [36, 'u16'];
export const battery_SOC_average: [number, string] = [38, 'u16'];
export const battery_SOC_max: [number, string] = [40, 'u16'];
export const battery_SOC_min: [number, string] = [42, 'u16'];
export const battery_current: [number, string] = [44, 'i32'];
export const kWh_available_total: [number, string] = [48, 'u16'];
export const kWh_available_max: [number, string] = [50, 'u16'];
export const battery_min_temperature: [number, string] = [52, 'i16'];
export const battery_max_temperature: [number, string] = [54, 'i16'];
export const battery_discharge_limit_amp: [number, string] = [56, 'u16'];
export const battery_charge_limit_amp: [number, string] = [58, 'u16'];
export const cell_min_voltage_mV: [number, string] = [60, 'u16'];
export const cell_max_voltage_mV: [number, string] = [62, 'u16'];
export const error_flags_from_BMS: [number, string] = [64, 'u8'];
export const contactor_state: [number, string] = [65, 'u8'];
export const HVIL: [number, string] = [66, 'u8'];
export const HMI_part1: [number, string] = [67, 'u8'];
export const HMI_part2: [number, string] = [68, 'u8'];
export const isolation_fault_status: [number, string] = [69, 'u8'];
export const system_safe_state: [number, string] = [70, 'u8'];
export const error_relay_open: [number, string] = [71, 'b'];
export const IsolationMeasurementOngoing: [number, string] = [72, 'b'];
export const battery_thermal_runaway: [number, string] = [73, 'b'];
export const puncture_fault: [number, string] = [74, 'b'];
export const liquid_fault: [number, string] = [75, 'b'];
export const contactor_DCFC_welded: [number, string] = [76, 'b'];
export const NACS_charger_detected: [number, string] = [77, 'b'];
export const slewrate_potential_violation: [number, string] = [78, 'b'];
export const minimum_power_potential_violation: [number, string] = [79, 'b'];
export const operation_limit_violation_warning: [number, string] = [80, 'b'];
export const previousMillis10: [number, string] = [84, 'u32'];
export const RIVIAN_150: [number, string] = [88, 'CAN_frame'];
export const RIVIAN_420: [number, string] = [160, 'CAN_frame'];
export const RIVIAN_41F: [number, string] = [232, 'CAN_frame'];
export const RIVIAN_245: [number, string] = [304, 'CAN_frame'];
export const RIVIAN_200: [number, string] = [376, 'CAN_frame'];
export const RIVIAN_207: [number, string] = [448, 'CAN_frame'];
