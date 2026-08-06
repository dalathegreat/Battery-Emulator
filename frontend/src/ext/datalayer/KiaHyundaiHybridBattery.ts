// KiaHyundaiHybridBattery: 768 bytes; base classes: CanBattery@0
export const KIA_HYUNDAI_HYBRID_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['counter_200', 'u8'],
  ['', ' '],
  ['SOC', 'u16'],
  ['SOC_display', 'u16'],
  ['interlock_missing', 'b'],
  ['', ' '],
  ['battery_current', 'i16'],
  ['battery_current_high_byte', 'u8'],
  ['', ' '],
  ['battery_voltage', 'u16'],
  ['', ' ', 2],
  ['available_charge_power', 'u32'],
  ['available_discharge_power', 'u32'],
  ['battery_module_max_temperature', 'i8'],
  ['battery_module_min_temperature', 'i8'],
  ['poll_data_pid', 'u8'],
  ['', ' '],
  ['cellvoltages_mv', 'u16', 96],
  ['min_cell_voltage_mv', 'u16'],
  ['max_cell_voltage_mv', 'u16'],
  ['KIA_7E4', 'CAN_frame'],
  ['KIA_7E4_ack', 'CAN_frame'],
  ['KIA_CLEAR_DTC', 'CAN_frame'],
  ['KIA_200', 'CAN_frame'],
  ['KIA_2A1', 'CAN_frame'],
  ['KIA_2F0', 'CAN_frame'],
  ['KIA_523', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const previousMillis10: [number, string] = [28, 'u32'];
export const previousMillis100: [number, string] = [32, 'u32'];
export const previousMillis1000: [number, string] = [36, 'u32'];
export const counter_200: [number, string] = [40, 'u8'];
export const SOC: [number, string] = [42, 'u16'];
export const SOC_display: [number, string] = [44, 'u16'];
export const interlock_missing: [number, string] = [46, 'b'];
export const battery_current: [number, string] = [48, 'i16'];
export const battery_current_high_byte: [number, string] = [50, 'u8'];
export const battery_voltage: [number, string] = [52, 'u16'];
export const available_charge_power: [number, string] = [56, 'u32'];
export const available_discharge_power: [number, string] = [60, 'u32'];
export const battery_module_max_temperature: [number, string] = [64, 'i8'];
export const battery_module_min_temperature: [number, string] = [65, 'i8'];
export const poll_data_pid: [number, string] = [66, 'u8'];
export const cellvoltages_mv: [number, string, number] = [68, 'u16', 96];
export const min_cell_voltage_mv: [number, string] = [260, 'u16'];
export const max_cell_voltage_mv: [number, string] = [262, 'u16'];
export const KIA_7E4: [number, string] = [264, 'CAN_frame'];
export const KIA_7E4_ack: [number, string] = [336, 'CAN_frame'];
export const KIA_CLEAR_DTC: [number, string] = [408, 'CAN_frame'];
export const KIA_200: [number, string] = [480, 'CAN_frame'];
export const KIA_2A1: [number, string] = [552, 'CAN_frame'];
export const KIA_2F0: [number, string] = [624, 'CAN_frame'];
export const KIA_523: [number, string] = [696, 'CAN_frame'];
