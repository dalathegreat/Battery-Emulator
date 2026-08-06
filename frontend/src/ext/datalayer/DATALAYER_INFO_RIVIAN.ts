export const DATALAYER_INFO_RIVIAN_FIELDS: ([string, string] | [string, string, number])[] = [
  ['pre_contactor_voltage', 'u16'],
  ['main_contactor_voltage', 'u16'],
  ['voltage_reference', 'u16'],
  ['DCFC_contactor_voltage', 'u16'],
  ['battery_SOC_max', 'u16'],
  ['battery_SOC_min', 'u16'],
  ['BMS_state', 'u8'],
  ['HVIL', 'u8'],
  ['error_flags_from_BMS', 'u8'],
  ['contactor_state', 'u8'],
  ['HMI_part1', 'u8'],
  ['HMI_part2', 'u8'],
  ['isolation_fault_status', 'u8'],
  ['system_safe_state', 'u8'],
  ['error_relay_open', 'b'],
  ['IsolationMeasurementOngoing', 'b'],
  ['puncture_fault', 'b'],
  ['liquid_fault', 'b'],
  ['contactor_DCFC_welded', 'b'],
  ['NACS_charger_detected', 'b'],
  ['slewrate_potential_violation', 'b'],
  ['minimum_power_potential_violation', 'b'],
  ['operation_limit_violation_warning', 'b'],
];

export const pre_contactor_voltage: [number, string] = [0, 'u16'];
export const main_contactor_voltage: [number, string] = [2, 'u16'];
export const voltage_reference: [number, string] = [4, 'u16'];
export const DCFC_contactor_voltage: [number, string] = [6, 'u16'];
export const battery_SOC_max: [number, string] = [8, 'u16'];
export const battery_SOC_min: [number, string] = [10, 'u16'];
export const BMS_state: [number, string] = [12, 'u8'];
export const HVIL: [number, string] = [13, 'u8'];
export const error_flags_from_BMS: [number, string] = [14, 'u8'];
export const contactor_state: [number, string] = [15, 'u8'];
export const HMI_part1: [number, string] = [16, 'u8'];
export const HMI_part2: [number, string] = [17, 'u8'];
export const isolation_fault_status: [number, string] = [18, 'u8'];
export const system_safe_state: [number, string] = [19, 'u8'];
export const error_relay_open: [number, string] = [20, 'b'];
export const IsolationMeasurementOngoing: [number, string] = [21, 'b'];
export const puncture_fault: [number, string] = [22, 'b'];
export const liquid_fault: [number, string] = [23, 'b'];
export const contactor_DCFC_welded: [number, string] = [24, 'b'];
export const NACS_charger_detected: [number, string] = [25, 'b'];
export const slewrate_potential_violation: [number, string] = [26, 'b'];
export const minimum_power_potential_violation: [number, string] = [27, 'b'];
export const operation_limit_violation_warning: [number, string] = [28, 'b'];
