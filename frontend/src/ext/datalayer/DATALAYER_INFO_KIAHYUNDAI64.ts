export const DATALAYER_INFO_KIAHYUNDAI64_FIELDS: ([string, string] | [string, string, number])[] = [
  ['cumulative_charge_current_ah', 'u32'],
  ['cumulative_discharge_current_ah', 'u32'],
  ['cumulative_energy_charged_kWh', 'u32'],
  ['cumulative_energy_discharged_kWh', 'u32'],
  ['powered_on_total_time', 'u32'],
  ['inverterVoltage', 'u16'],
  ['isolation_resistance_kOhm', 'u16'],
  ['number_of_standard_charging_sessions', 'u16'],
  ['number_of_fastcharging_sessions', 'u16'],
  ['accumulated_normal_charging_energy_kWh', 'u16'],
  ['accumulated_fastcharging_energy_kWh', 'u16'],
  ['battery_12V', 'u16'],
  ['temperature_water_inlet', 'i8'],
  ['powerRelayTemperature', 'i8'],
  ['total_cell_count', 'u8'],
  ['waterleakageSensor', 'u8'],
  ['batteryManagementMode', 'u8'],
  ['BMS_ign', 'u8'],
  ['batteryRelay', 'u8'],
  ['ecu_serial_number', 'u8', 16],
  ['ecu_version_number', 'u8', 16],
];

export const cumulative_charge_current_ah: [number, string] = [0, 'u32'];
export const cumulative_discharge_current_ah: [number, string] = [4, 'u32'];
export const cumulative_energy_charged_kWh: [number, string] = [8, 'u32'];
export const cumulative_energy_discharged_kWh: [number, string] = [12, 'u32'];
export const powered_on_total_time: [number, string] = [16, 'u32'];
export const inverterVoltage: [number, string] = [20, 'u16'];
export const isolation_resistance_kOhm: [number, string] = [22, 'u16'];
export const number_of_standard_charging_sessions: [number, string] = [24, 'u16'];
export const number_of_fastcharging_sessions: [number, string] = [26, 'u16'];
export const accumulated_normal_charging_energy_kWh: [number, string] = [28, 'u16'];
export const accumulated_fastcharging_energy_kWh: [number, string] = [30, 'u16'];
export const battery_12V: [number, string] = [32, 'u16'];
export const temperature_water_inlet: [number, string] = [34, 'i8'];
export const powerRelayTemperature: [number, string] = [35, 'i8'];
export const total_cell_count: [number, string] = [36, 'u8'];
export const waterleakageSensor: [number, string] = [37, 'u8'];
export const batteryManagementMode: [number, string] = [38, 'u8'];
export const BMS_ign: [number, string] = [39, 'u8'];
export const batteryRelay: [number, string] = [40, 'u8'];
export const ecu_serial_number: [number, string, number] = [41, 'u8', 16];
export const ecu_version_number: [number, string, number] = [57, 'u8', 16];
