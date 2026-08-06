// GrowattHvArkBattery: 288 bytes; base classes: CanBattery@0
export const GROWATT_HV_ARK_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['PCS_3010', 'CAN_frame'],
  ['PCS_3020', 'CAN_frame'],
  ['PCS_3030', 'CAN_frame'],
  ['previousMillis1000', 'u32'],
  ['send_times', 'u16'],
  ['', ' ', 2],
  ['epoch_time_s', 'u32'],
  ['max_charge_voltage_dV', 'u16'],
  ['discharge_cutoff_voltage_dV', 'u16'],
  ['max_charge_current_dA', 'i16'],
  ['max_discharge_current_dA', 'i16'],
  ['pack_voltage_dV', 'u16'],
  ['pack_current_dA', 'i16'],
  ['temp_max_dC', 'i16'],
  ['temp_min_dC', 'i16'],
  ['soc_pct', 'u8'],
  ['soh_pct', 'u8'],
  ['cell_max_mV', 'u16'],
  ['cell_min_mV', 'u16'],
  ['remaining_capacity_10mAh', 'u16'],
  ['full_capacity_10mAh', 'u16'],
  ['battery_sleeping', 'b'],
  ['battery_fault_present', 'b'],
  ['battery_no_charge', 'b'],
  ['battery_no_discharge', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const PCS_3010: [number, string] = [24, 'CAN_frame'];
export const PCS_3020: [number, string] = [96, 'CAN_frame'];
export const PCS_3030: [number, string] = [168, 'CAN_frame'];
export const previousMillis1000: [number, string] = [240, 'u32'];
export const send_times: [number, string] = [244, 'u16'];
export const epoch_time_s: [number, string] = [248, 'u32'];
export const max_charge_voltage_dV: [number, string] = [252, 'u16'];
export const discharge_cutoff_voltage_dV: [number, string] = [254, 'u16'];
export const max_charge_current_dA: [number, string] = [256, 'i16'];
export const max_discharge_current_dA: [number, string] = [258, 'i16'];
export const pack_voltage_dV: [number, string] = [260, 'u16'];
export const pack_current_dA: [number, string] = [262, 'i16'];
export const temp_max_dC: [number, string] = [264, 'i16'];
export const temp_min_dC: [number, string] = [266, 'i16'];
export const soc_pct: [number, string] = [268, 'u8'];
export const soh_pct: [number, string] = [269, 'u8'];
export const cell_max_mV: [number, string] = [270, 'u16'];
export const cell_min_mV: [number, string] = [272, 'u16'];
export const remaining_capacity_10mAh: [number, string] = [274, 'u16'];
export const full_capacity_10mAh: [number, string] = [276, 'u16'];
export const battery_sleeping: [number, string] = [278, 'b'];
export const battery_fault_present: [number, string] = [279, 'b'];
export const battery_no_charge: [number, string] = [280, 'b'];
export const battery_no_discharge: [number, string] = [281, 'b'];
