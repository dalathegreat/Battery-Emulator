// RelionBattery: 544 bytes; base classes: CanBattery@0
export const RELION_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['previousMillis500ms', 'u32'],
  ['SOC', '6854'],
  ['', ' ', 202],
  ['numPoints', 'u8'],
  ['', ' '],
  ['cellVoltageLookup', '6854'],
  ['', ' ', 202],
  ['SOC_from_max_cell_voltage', 'u16'],
  ['SOC_from_min_cell_voltage', 'u16'],
  ['battery_total_voltage', 'u16'],
  ['battery_total_current', 'i16'],
  ['system_state', 'u8'],
  ['battery_soc', 'u8'],
  ['battery_soh', 'u8'],
  ['most_serious_fault', 'u8'],
  ['max_cell_voltage', 'u16'],
  ['min_cell_voltage', 'u16'],
  ['max_cell_temperature', 'i16'],
  ['min_cell_temperature', 'i16'],
  ['charge_current_A', 'i16'],
  ['regen_charge_current_A', 'i16'],
  ['discharge_current_A', 'i16'],
  ['', ' ', 4],
  ['RELION_CONTACTOR_MESSAGE', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const datalayer_battery: [number, string] = [24, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [28, 'bool*'];
export const previousMillis500ms: [number, string] = [32, 'u32'];
export const SOC: [number, string] = [36, '6854'];
export const numPoints: [number, string] = [238, 'u8'];
export const cellVoltageLookup: [number, string] = [240, '6854'];
export const SOC_from_max_cell_voltage: [number, string] = [442, 'u16'];
export const SOC_from_min_cell_voltage: [number, string] = [444, 'u16'];
export const battery_total_voltage: [number, string] = [446, 'u16'];
export const battery_total_current: [number, string] = [448, 'i16'];
export const system_state: [number, string] = [450, 'u8'];
export const battery_soc: [number, string] = [451, 'u8'];
export const battery_soh: [number, string] = [452, 'u8'];
export const most_serious_fault: [number, string] = [453, 'u8'];
export const max_cell_voltage: [number, string] = [454, 'u16'];
export const min_cell_voltage: [number, string] = [456, 'u16'];
export const max_cell_temperature: [number, string] = [458, 'i16'];
export const min_cell_temperature: [number, string] = [460, 'i16'];
export const charge_current_A: [number, string] = [462, 'i16'];
export const regen_charge_current_A: [number, string] = [464, 'i16'];
export const discharge_current_A: [number, string] = [466, 'i16'];
export const RELION_CONTACTOR_MESSAGE: [number, string] = [472, 'CAN_frame'];
