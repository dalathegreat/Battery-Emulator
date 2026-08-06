export const DATALAYER_INFO_BOLTAMPERA_FIELDS: ([string, string] | [string, string, number])[] = [
  ['battery_5V_ref', 'u16'],
  ['battery_capacity_my17_18', 'u16'],
  ['battery_capacity_my19plus', 'u16'],
  ['battery_SOC_display', 'u16'],
  ['battery_SOC_raw_highprec', 'u16'],
  ['battery_max_temperature', 'u16'],
  ['battery_min_temperature', 'u16'],
  ['battery_max_cell_voltage', 'u16'],
  ['battery_min_cell_voltage', 'u16'],
  ['battery_lowest_cell', 'u16'],
  ['battery_highest_cell', 'u16'],
  ['battery_internal_resistance', 'u16'],
  ['battery_voltage_polled', 'u16'],
  ['battery_vehicle_isolation', 'u16'],
  ['battery_isolation_kohm', 'u16'],
  ['battery_HV_locked', 'u16'],
  ['battery_crash_event', 'u16'],
  ['battery_HVIL', 'u16'],
  ['battery_HVIL_status', 'u16'],
  ['battery_cell_average_voltage', 'u16'],
  ['battery_cell_average_voltage_2', 'u16'],
  ['battery_terminal_voltage', 'u16'],
  ['battery_ignition_power_mode', 'u16'],
  ['battery_module_temp_1', 'i16'],
  ['battery_module_temp_2', 'i16'],
  ['battery_module_temp_3', 'i16'],
  ['battery_module_temp_4', 'i16'],
  ['battery_module_temp_5', 'i16'],
  ['battery_module_temp_6', 'i16'],
  ['battery_current_7E7', 'i16'],
  ['battery_current_7E4', 'i16'],
];

export const battery_5V_ref: [number, string] = [0, 'u16'];
export const battery_capacity_my17_18: [number, string] = [2, 'u16'];
export const battery_capacity_my19plus: [number, string] = [4, 'u16'];
export const battery_SOC_display: [number, string] = [6, 'u16'];
export const battery_SOC_raw_highprec: [number, string] = [8, 'u16'];
export const battery_max_temperature: [number, string] = [10, 'u16'];
export const battery_min_temperature: [number, string] = [12, 'u16'];
export const battery_max_cell_voltage: [number, string] = [14, 'u16'];
export const battery_min_cell_voltage: [number, string] = [16, 'u16'];
export const battery_lowest_cell: [number, string] = [18, 'u16'];
export const battery_highest_cell: [number, string] = [20, 'u16'];
export const battery_internal_resistance: [number, string] = [22, 'u16'];
export const battery_voltage_polled: [number, string] = [24, 'u16'];
export const battery_vehicle_isolation: [number, string] = [26, 'u16'];
export const battery_isolation_kohm: [number, string] = [28, 'u16'];
export const battery_HV_locked: [number, string] = [30, 'u16'];
export const battery_crash_event: [number, string] = [32, 'u16'];
export const battery_HVIL: [number, string] = [34, 'u16'];
export const battery_HVIL_status: [number, string] = [36, 'u16'];
export const battery_cell_average_voltage: [number, string] = [38, 'u16'];
export const battery_cell_average_voltage_2: [number, string] = [40, 'u16'];
export const battery_terminal_voltage: [number, string] = [42, 'u16'];
export const battery_ignition_power_mode: [number, string] = [44, 'u16'];
export const battery_module_temp_1: [number, string] = [46, 'i16'];
export const battery_module_temp_2: [number, string] = [48, 'i16'];
export const battery_module_temp_3: [number, string] = [50, 'i16'];
export const battery_module_temp_4: [number, string] = [52, 'i16'];
export const battery_module_temp_5: [number, string] = [54, 'i16'];
export const battery_module_temp_6: [number, string] = [56, 'i16'];
export const battery_current_7E7: [number, string] = [58, 'i16'];
export const battery_current_7E4: [number, string] = [60, 'i16'];
