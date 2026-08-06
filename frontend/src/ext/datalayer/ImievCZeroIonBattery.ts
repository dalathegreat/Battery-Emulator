// ImievCZeroIonBattery: 812 bytes; base classes: CanBattery@0
export const IMIEV_CZERO_ION_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['errorCode', 'u8'],
  ['BMU_Detected', 'u8'],
  ['CMU_Detected', 'u8'],
  ['', ' '],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['pid_index', 'i32'],
  ['cmu_id', 'i32'],
  ['voltage_index', 'i32'],
  ['temp_index', 'i32'],
  ['BMU_SOC', 'u8'],
  ['', ' ', 3],
  ['temp_value', 'i32'],
  ['temp1', 'f'],
  ['temp2', 'f'],
  ['temp3', 'f'],
  ['voltage1', 'f'],
  ['voltage2', 'f'],
  ['BMU_Current', 'f'],
  ['BMU_PackVoltage', 'f'],
  ['BMU_Power', 'f'],
  ['cell_voltages', 'f', 88],
  ['cell_temperatures', 'f', 88],
  ['max_volt_cel', 'f'],
  ['min_volt_cel', 'f'],
  ['max_temp_cel', 'f'],
  ['min_temp_cel', 'f'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const errorCode: [number, string] = [24, 'u8'];
export const BMU_Detected: [number, string] = [25, 'u8'];
export const CMU_Detected: [number, string] = [26, 'u8'];
export const previousMillis10: [number, string] = [28, 'u32'];
export const previousMillis100: [number, string] = [32, 'u32'];
export const pid_index: [number, string] = [36, 'i32'];
export const cmu_id: [number, string] = [40, 'i32'];
export const voltage_index: [number, string] = [44, 'i32'];
export const temp_index: [number, string] = [48, 'i32'];
export const BMU_SOC: [number, string] = [52, 'u8'];
export const temp_value: [number, string] = [56, 'i32'];
export const temp1: [number, string] = [60, 'f'];
export const temp2: [number, string] = [64, 'f'];
export const temp3: [number, string] = [68, 'f'];
export const voltage1: [number, string] = [72, 'f'];
export const voltage2: [number, string] = [76, 'f'];
export const BMU_Current: [number, string] = [80, 'f'];
export const BMU_PackVoltage: [number, string] = [84, 'f'];
export const BMU_Power: [number, string] = [88, 'f'];
export const cell_voltages: [number, string, number] = [92, 'f', 88];
export const cell_temperatures: [number, string, number] = [444, 'f', 88];
export const max_volt_cel: [number, string] = [796, 'f'];
export const min_volt_cel: [number, string] = [800, 'f'];
export const max_temp_cel: [number, string] = [804, 'f'];
export const min_temp_cel: [number, string] = [808, 'f'];
