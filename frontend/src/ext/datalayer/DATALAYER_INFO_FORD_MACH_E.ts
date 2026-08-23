export const DATALAYER_INFO_FORD_MACH_E_FIELDS: ([string, string] | [string, string, number])[] = [
  ['pid_hvb_temp', 'i16'],
  ['', ' ', 2],
  ['pid_hvb_soc', 'u32'],
  ['pid_hvb_contactor_status', 'u32'],
  ['pid_hvb_contactor_positive_leak_voltage', 'u16'],
  ['pid_hvb_contactor_negative_leak_voltage', 'u16'],
  ['pid_hvb_contactor_positive_voltage', 'u16'],
  ['pid_hvb_contactor_negative_voltage', 'u16'],
  ['pid_hvb_contactor_positive_bus_leak_resistance', 'u16'],
  ['pid_hvb_contactor_negative_bus_leak_resistance', 'u16'],
  ['pid_hvb_contactor_overall_leak_resistance', 'u16'],
  ['pid_hvb_contactor_open_leak_resistance', 'u16'],
  ['pid_hvb_voltage', 'u16'],
  ['pid_hvb_max_charge_current', 'u16'],
  ['pid_hvb_calendar_age_months', 'u16'],
  ['pid_battery_capacity_ah', 'u16'],
  ['pid_maintenance_rebalance_status', 'u8'],
  ['pid_hvb_soh', 'u8'],
];

export const pid_hvb_temp: [number, string] = [0, 'i16'];
export const pid_hvb_soc: [number, string] = [4, 'u32'];
export const pid_hvb_contactor_status: [number, string] = [8, 'u32'];
export const pid_hvb_contactor_positive_leak_voltage: [number, string] = [12, 'u16'];
export const pid_hvb_contactor_negative_leak_voltage: [number, string] = [14, 'u16'];
export const pid_hvb_contactor_positive_voltage: [number, string] = [16, 'u16'];
export const pid_hvb_contactor_negative_voltage: [number, string] = [18, 'u16'];
export const pid_hvb_contactor_positive_bus_leak_resistance: [number, string] = [20, 'u16'];
export const pid_hvb_contactor_negative_bus_leak_resistance: [number, string] = [22, 'u16'];
export const pid_hvb_contactor_overall_leak_resistance: [number, string] = [24, 'u16'];
export const pid_hvb_contactor_open_leak_resistance: [number, string] = [26, 'u16'];
export const pid_hvb_voltage: [number, string] = [28, 'u16'];
export const pid_hvb_max_charge_current: [number, string] = [30, 'u16'];
export const pid_hvb_calendar_age_months: [number, string] = [32, 'u16'];
export const pid_battery_capacity_ah: [number, string] = [34, 'u16'];
export const pid_maintenance_rebalance_status: [number, string] = [36, 'u8'];
export const pid_hvb_soh: [number, string] = [37, 'u8'];
