// ThinkBattery: 208 bytes; base classes: CanBattery@0
export const THINK_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['previousMillis200', 'u32'],
  ['', ' ', 4],
  ['PCU_310', 'CAN_frame'],
  ['PCU_311', 'CAN_frame'],
  ['min_cellvoltage', 'u16'],
  ['max_cellvoltage', 'u16'],
  ['sys_voltage', 'u16'],
  ['sys_dod', 'u16'],
  ['sys_voltageMinDischarge', 'u16'],
  ['sys_currentMaxDischarge', 'u16'],
  ['sys_currentMaxCharge', 'u16'],
  ['sys_voltageMaxCharge', 'u16'],
  ['BatterySOC', 'u16'],
  ['sys_tempMean', 'i16'],
  ['sys_current', 'i16'],
  ['Battery_Type', 'u8'],
  ['sys_ZebraTempError', 'u8'],
  ['sys_numberFailedCells', 'u8'],
  ['min_pack_temperature', 'i8'],
  ['max_pack_temperature', 'i8'],
  ['sys_errGeneral', 'b'],
  ['sys_isolationError', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const previousMillis200: [number, string] = [24, 'u32'];
export const PCU_310: [number, string] = [32, 'CAN_frame'];
export const PCU_311: [number, string] = [104, 'CAN_frame'];
export const min_cellvoltage: [number, string] = [176, 'u16'];
export const max_cellvoltage: [number, string] = [178, 'u16'];
export const sys_voltage: [number, string] = [180, 'u16'];
export const sys_dod: [number, string] = [182, 'u16'];
export const sys_voltageMinDischarge: [number, string] = [184, 'u16'];
export const sys_currentMaxDischarge: [number, string] = [186, 'u16'];
export const sys_currentMaxCharge: [number, string] = [188, 'u16'];
export const sys_voltageMaxCharge: [number, string] = [190, 'u16'];
export const BatterySOC: [number, string] = [192, 'u16'];
export const sys_tempMean: [number, string] = [194, 'i16'];
export const sys_current: [number, string] = [196, 'i16'];
export const Battery_Type: [number, string] = [198, 'u8'];
export const sys_ZebraTempError: [number, string] = [199, 'u8'];
export const sys_numberFailedCells: [number, string] = [200, 'u8'];
export const min_pack_temperature: [number, string] = [201, 'i8'];
export const max_pack_temperature: [number, string] = [202, 'i8'];
export const sys_errGeneral: [number, string] = [203, 'b'];
export const sys_isolationError: [number, string] = [204, 'b'];
