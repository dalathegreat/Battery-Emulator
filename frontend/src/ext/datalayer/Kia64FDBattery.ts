// Kia64FDBattery: 1032 bytes; base classes: CanBattery@0
export const KIA64_FDBATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['previousMillis200ms', 'u32'],
  ['previousMillis10s', 'u32'],
  ['inverterVoltageFrameHigh', 'u16'],
  ['inverterVoltage', 'u16'],
  ['soc_calculated', 'u16'],
  ['SOC_BMS', 'u16'],
  ['SOC_Display', 'u16'],
  ['SOC_estimated_lowest', 'u16'],
  ['SOC_estimated_highest', 'u16'],
  ['batterySOH', 'u16'],
  ['CellVoltMax_mV', 'u16'],
  ['CellVoltMin_mV', 'u16'],
  ['batteryVoltage', 'u16'],
  ['leadAcidBatteryVoltage', 'i16'],
  ['batteryAmps', 'i16'],
  ['temperatureMax', 'i16'],
  ['temperatureMin', 'i16'],
  ['allowedDischargePower', 'i16'],
  ['allowedChargePower', 'i16'],
  ['poll_data_pid', 'i16'],
  ['CellVmaxNo', 'u8'],
  ['CellVminNo', 'u8'],
  ['batteryManagementMode', 'u8'],
  ['BMS_ign', 'u8'],
  ['startedUp', 'b'],
  ['ok_start_polling_battery', 'b'],
  ['KIA_7E4_COUNTER', 'u8'],
  ['temperature_water_inlet', 'i8'],
  ['heatertemp', 'i8'],
  ['', ' ', 3],
  ['startMillis', 'u32'],
  ['messageIndex', 'u8'],
  ['numPoints', 'u8'],
  ['SOC', '8171'],
  ['', ' ', 202],
  ['voltage', '8171'],
  ['', ' ', 202],
  ['messageDelays', 'u8', 63],
  ['', ' ', 3],
  ['messages', '8186'],
  ['', ' ', 256],
  ['KIA64FD_7E4', 'CAN_frame'],
  ['KIA64FD_ack', 'CAN_frame'],
  ['KIA64FD_CLEAR_DTC', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const previousMillis200ms: [number, string] = [28, 'u32'];
export const previousMillis10s: [number, string] = [32, 'u32'];
export const inverterVoltageFrameHigh: [number, string] = [36, 'u16'];
export const inverterVoltage: [number, string] = [38, 'u16'];
export const soc_calculated: [number, string] = [40, 'u16'];
export const SOC_BMS: [number, string] = [42, 'u16'];
export const SOC_Display: [number, string] = [44, 'u16'];
export const SOC_estimated_lowest: [number, string] = [46, 'u16'];
export const SOC_estimated_highest: [number, string] = [48, 'u16'];
export const batterySOH: [number, string] = [50, 'u16'];
export const CellVoltMax_mV: [number, string] = [52, 'u16'];
export const CellVoltMin_mV: [number, string] = [54, 'u16'];
export const batteryVoltage: [number, string] = [56, 'u16'];
export const leadAcidBatteryVoltage: [number, string] = [58, 'i16'];
export const batteryAmps: [number, string] = [60, 'i16'];
export const temperatureMax: [number, string] = [62, 'i16'];
export const temperatureMin: [number, string] = [64, 'i16'];
export const allowedDischargePower: [number, string] = [66, 'i16'];
export const allowedChargePower: [number, string] = [68, 'i16'];
export const poll_data_pid: [number, string] = [70, 'i16'];
export const CellVmaxNo: [number, string] = [72, 'u8'];
export const CellVminNo: [number, string] = [73, 'u8'];
export const batteryManagementMode: [number, string] = [74, 'u8'];
export const BMS_ign: [number, string] = [75, 'u8'];
export const startedUp: [number, string] = [76, 'b'];
export const ok_start_polling_battery: [number, string] = [77, 'b'];
export const KIA_7E4_COUNTER: [number, string] = [78, 'u8'];
export const temperature_water_inlet: [number, string] = [79, 'i8'];
export const heatertemp: [number, string] = [80, 'i8'];
export const startMillis: [number, string] = [84, 'u32'];
export const messageIndex: [number, string] = [88, 'u8'];
export const numPoints: [number, string] = [89, 'u8'];
export const SOC: [number, string] = [90, '8171'];
export const voltage: [number, string] = [292, '8171'];
export const messageDelays: [number, string, number] = [494, 'u8', 63];
export const messages: [number, string] = [560, '8186'];
export const KIA64FD_7E4: [number, string] = [816, 'CAN_frame'];
export const KIA64FD_ack: [number, string] = [888, 'CAN_frame'];
export const KIA64FD_CLEAR_DTC: [number, string] = [960, 'CAN_frame'];
