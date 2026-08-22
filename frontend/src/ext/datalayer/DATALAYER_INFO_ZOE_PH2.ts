export const DATALAYER_INFO_ZOE_PH2_FIELDS: ([string, string] | [string, string, number])[] = [
  ['battery_slave_failures', 'u32'],
  ['battery_soc', 'u16'],
  ['battery_usable_soc', 'u16'],
  ['battery_soh', 'u16'],
  ['battery_pack_voltage', 'u16'],
  ['battery_max_cell_voltage', 'u16'],
  ['battery_min_cell_voltage', 'u16'],
  ['battery_12v', 'u16'],
  ['battery_avg_temp', 'u16'],
  ['battery_min_temp', 'u16'],
  ['battery_max_temp', 'u16'],
  ['battery_max_power', 'u16'],
  ['battery_interlock', 'u16'],
  ['battery_kwh', 'u16'],
  ['battery_current', 'u16'],
  ['battery_current_offset', 'u16'],
  ['battery_max_generated', 'u16'],
  ['battery_max_available', 'u16'],
  ['battery_current_voltage', 'u16'],
  ['battery_charging_status', 'u16'],
  ['battery_remaining_charge', 'u16'],
  ['battery_balance_capacity_total', 'u16'],
  ['battery_balance_time_total', 'u16'],
  ['battery_balance_capacity_sleep', 'u16'],
  ['battery_balance_time_sleep', 'u16'],
  ['battery_balance_capacity_wake', 'u16'],
  ['battery_balance_time_wake', 'u16'],
  ['battery_bms_state', 'u16'],
  ['battery_energy_complete', 'u16'],
  ['battery_energy_partial', 'u16'],
  ['battery_mileage', 'u16'],
  ['battery_fan_speed', 'u16'],
  ['battery_fan_period', 'u16'],
  ['battery_fan_control', 'u16'],
  ['battery_fan_duty', 'u16'],
  ['battery_temporisation', 'u16'],
  ['battery_time', 'u16'],
  ['battery_pack_time', 'u16'],
  ['battery_soc_min', 'u16'],
  ['battery_soc_max', 'u16'],
  ['UserRequestNVROLReset', 'b'],
];

export const battery_slave_failures: [number, string] = [0, 'u32'];
export const battery_soc: [number, string] = [4, 'u16'];
export const battery_usable_soc: [number, string] = [6, 'u16'];
export const battery_soh: [number, string] = [8, 'u16'];
export const battery_pack_voltage: [number, string] = [10, 'u16'];
export const battery_max_cell_voltage: [number, string] = [12, 'u16'];
export const battery_min_cell_voltage: [number, string] = [14, 'u16'];
export const battery_12v: [number, string] = [16, 'u16'];
export const battery_avg_temp: [number, string] = [18, 'u16'];
export const battery_min_temp: [number, string] = [20, 'u16'];
export const battery_max_temp: [number, string] = [22, 'u16'];
export const battery_max_power: [number, string] = [24, 'u16'];
export const battery_interlock: [number, string] = [26, 'u16'];
export const battery_kwh: [number, string] = [28, 'u16'];
export const battery_current: [number, string] = [30, 'u16'];
export const battery_current_offset: [number, string] = [32, 'u16'];
export const battery_max_generated: [number, string] = [34, 'u16'];
export const battery_max_available: [number, string] = [36, 'u16'];
export const battery_current_voltage: [number, string] = [38, 'u16'];
export const battery_charging_status: [number, string] = [40, 'u16'];
export const battery_remaining_charge: [number, string] = [42, 'u16'];
export const battery_balance_capacity_total: [number, string] = [44, 'u16'];
export const battery_balance_time_total: [number, string] = [46, 'u16'];
export const battery_balance_capacity_sleep: [number, string] = [48, 'u16'];
export const battery_balance_time_sleep: [number, string] = [50, 'u16'];
export const battery_balance_capacity_wake: [number, string] = [52, 'u16'];
export const battery_balance_time_wake: [number, string] = [54, 'u16'];
export const battery_bms_state: [number, string] = [56, 'u16'];
export const battery_energy_complete: [number, string] = [58, 'u16'];
export const battery_energy_partial: [number, string] = [60, 'u16'];
export const battery_mileage: [number, string] = [62, 'u16'];
export const battery_fan_speed: [number, string] = [64, 'u16'];
export const battery_fan_period: [number, string] = [66, 'u16'];
export const battery_fan_control: [number, string] = [68, 'u16'];
export const battery_fan_duty: [number, string] = [70, 'u16'];
export const battery_temporisation: [number, string] = [72, 'u16'];
export const battery_time: [number, string] = [74, 'u16'];
export const battery_pack_time: [number, string] = [76, 'u16'];
export const battery_soc_min: [number, string] = [78, 'u16'];
export const battery_soc_max: [number, string] = [80, 'u16'];
export const UserRequestNVROLReset: [number, string] = [82, 'b'];
