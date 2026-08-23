// SantaFePhevBattery: 776 bytes; base classes: CanBattery@0
export const SANTA_FE_PHEV_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['UserRequestedDTCReset', 'b'],
  ['', ' ', 3],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis500', 'u32'],
  ['poll_data_pid', 'u8'],
  ['counter_200', 'u8'],
  ['checksum_200', 'u8'],
  ['', ' '],
  ['SOC_Display', 'u16'],
  ['batterySOH', 'u16'],
  ['CellVoltMax_mV', 'u16'],
  ['CellVoltMin_mV', 'u16'],
  ['CellVmaxNo', 'u8'],
  ['CellVminNo', 'u8'],
  ['allowedDischargePower', 'u16'],
  ['allowedChargePower', 'u16'],
  ['batteryVoltage', 'u16'],
  ['leadAcidBatteryVoltage', 'i16'],
  ['temperatureMax', 'i8'],
  ['temperatureMin', 'i8'],
  ['batteryAmps', 'i16'],
  ['StatusBattery', 'u8'],
  ['', ' '],
  ['cellvoltages_mv', 'u16', 96],
  ['', ' ', 4],
  ['SANTAFE_200', 'CAN_frame'],
  ['SANTAFE_2A1', 'CAN_frame'],
  ['SANTAFE_2F0', 'CAN_frame'],
  ['SANTAFE_523', 'CAN_frame'],
  ['SANTAFE_7E4_poll', 'CAN_frame'],
  ['SANTAFE_7E4_ack', 'CAN_frame'],
  ['SANTAFE_CLEAR_DTC', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const datalayer_battery: [number, string] = [24, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [28, 'bool*'];
export const UserRequestedDTCReset: [number, string] = [32, 'b'];
export const previousMillis10: [number, string] = [36, 'u32'];
export const previousMillis100: [number, string] = [40, 'u32'];
export const previousMillis500: [number, string] = [44, 'u32'];
export const poll_data_pid: [number, string] = [48, 'u8'];
export const counter_200: [number, string] = [49, 'u8'];
export const checksum_200: [number, string] = [50, 'u8'];
export const SOC_Display: [number, string] = [52, 'u16'];
export const batterySOH: [number, string] = [54, 'u16'];
export const CellVoltMax_mV: [number, string] = [56, 'u16'];
export const CellVoltMin_mV: [number, string] = [58, 'u16'];
export const CellVmaxNo: [number, string] = [60, 'u8'];
export const CellVminNo: [number, string] = [61, 'u8'];
export const allowedDischargePower: [number, string] = [62, 'u16'];
export const allowedChargePower: [number, string] = [64, 'u16'];
export const batteryVoltage: [number, string] = [66, 'u16'];
export const leadAcidBatteryVoltage: [number, string] = [68, 'i16'];
export const temperatureMax: [number, string] = [70, 'i8'];
export const temperatureMin: [number, string] = [71, 'i8'];
export const batteryAmps: [number, string] = [72, 'i16'];
export const StatusBattery: [number, string] = [74, 'u8'];
export const cellvoltages_mv: [number, string, number] = [76, 'u16', 96];
export const SANTAFE_200: [number, string] = [272, 'CAN_frame'];
export const SANTAFE_2A1: [number, string] = [344, 'CAN_frame'];
export const SANTAFE_2F0: [number, string] = [416, 'CAN_frame'];
export const SANTAFE_523: [number, string] = [488, 'CAN_frame'];
export const SANTAFE_7E4_poll: [number, string] = [560, 'CAN_frame'];
export const SANTAFE_7E4_ack: [number, string] = [632, 'CAN_frame'];
export const SANTAFE_CLEAR_DTC: [number, string] = [704, 'CAN_frame'];
