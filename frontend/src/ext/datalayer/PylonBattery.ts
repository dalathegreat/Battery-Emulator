// PylonBattery: 560 bytes; base classes: CanBattery@0
export const PYLON_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['extended_data', 'PylonExtendedData'],
  ['renderer', 'PylonHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['contactor_closing_allowed', 'bool*'],
  ['previousMillis1000', 'u32'],
  ['previousMillis5000', 'u32'],
  ['PYLON_3010', 'CAN_frame'],
  ['PYLON_8200', 'CAN_frame'],
  ['PYLON_8210', 'CAN_frame'],
  ['PYLON_4200', 'CAN_frame'],
  ['EMUS_CELL_VOLTAGE_REQUEST', 'CAN_frame'],
  ['EMUS_CELL_BALANCING_REQUEST', 'CAN_frame'],
  ['celltemperature_max_dC', 'i16'],
  ['celltemperature_min_dC', 'i16'],
  ['current_dA', 'i16'],
  ['total_capacity_Wh', 'u16'],
  ['remaining_capacity_Wh', 'u16'],
  ['voltage_dV', 'u16'],
  ['cellvoltage_max_mV', 'u16'],
  ['cellvoltage_min_mV', 'u16'],
  ['charge_cutoff_voltage', 'u16'],
  ['discharge_cutoff_voltage', 'u16'],
  ['max_charge_current_dA', 'i16'],
  ['max_discharge_current_dA', 'i16'],
  ['BMS_temperature_dC', 'i16'],
  ['battery_module_quantity', 'u8'],
  ['battery_modules_in_series', 'u8'],
  ['cell_quantity_in_module', 'u8'],
  ['voltage_level', 'u8'],
  ['ah_number', 'u8'],
  ['SOC', 'u8'],
  ['SOH', 'u8'],
  ['charge_forbidden', 'u8'],
  ['discharge_forbidden', 'u8'],
  ['manufacturer_name', 'u8', 16],
  ['mux', 'u8'],
  ['hardware_version', 'u8'],
  ['hardware_version_V', 'u8'],
  ['hardware_version_R', 'u8'],
  ['software_version_major', 'u8'],
  ['software_version_minor', 'u8'],
  ['actual_cell_count', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const extended_data: [number, string] = [24, 'PylonExtendedData'];
export const renderer: [number, string] = [36, 'PylonHtmlRenderer'];
export const datalayer_battery: [number, string] = [44, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [48, 'bool*'];
export const contactor_closing_allowed: [number, string] = [52, 'bool*'];
export const previousMillis1000: [number, string] = [56, 'u32'];
export const previousMillis5000: [number, string] = [60, 'u32'];
export const PYLON_3010: [number, string] = [64, 'CAN_frame'];
export const PYLON_8200: [number, string] = [136, 'CAN_frame'];
export const PYLON_8210: [number, string] = [208, 'CAN_frame'];
export const PYLON_4200: [number, string] = [280, 'CAN_frame'];
export const EMUS_CELL_VOLTAGE_REQUEST: [number, string] = [352, 'CAN_frame'];
export const EMUS_CELL_BALANCING_REQUEST: [number, string] = [424, 'CAN_frame'];
export const celltemperature_max_dC: [number, string] = [496, 'i16'];
export const celltemperature_min_dC: [number, string] = [498, 'i16'];
export const current_dA: [number, string] = [500, 'i16'];
export const total_capacity_Wh: [number, string] = [502, 'u16'];
export const remaining_capacity_Wh: [number, string] = [504, 'u16'];
export const voltage_dV: [number, string] = [506, 'u16'];
export const cellvoltage_max_mV: [number, string] = [508, 'u16'];
export const cellvoltage_min_mV: [number, string] = [510, 'u16'];
export const charge_cutoff_voltage: [number, string] = [512, 'u16'];
export const discharge_cutoff_voltage: [number, string] = [514, 'u16'];
export const max_charge_current_dA: [number, string] = [516, 'i16'];
export const max_discharge_current_dA: [number, string] = [518, 'i16'];
export const BMS_temperature_dC: [number, string] = [520, 'i16'];
export const battery_module_quantity: [number, string] = [522, 'u8'];
export const battery_modules_in_series: [number, string] = [523, 'u8'];
export const cell_quantity_in_module: [number, string] = [524, 'u8'];
export const voltage_level: [number, string] = [525, 'u8'];
export const ah_number: [number, string] = [526, 'u8'];
export const SOC: [number, string] = [527, 'u8'];
export const SOH: [number, string] = [528, 'u8'];
export const charge_forbidden: [number, string] = [529, 'u8'];
export const discharge_forbidden: [number, string] = [530, 'u8'];
export const manufacturer_name: [number, string, number] = [531, 'u8', 16];
export const mux: [number, string] = [547, 'u8'];
export const hardware_version: [number, string] = [548, 'u8'];
export const hardware_version_V: [number, string] = [549, 'u8'];
export const hardware_version_R: [number, string] = [550, 'u8'];
export const software_version_major: [number, string] = [551, 'u8'];
export const software_version_minor: [number, string] = [552, 'u8'];
export const actual_cell_count: [number, string] = [553, 'u8'];
