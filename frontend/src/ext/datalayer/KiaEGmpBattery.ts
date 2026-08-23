// KiaEGmpBattery: 896 bytes; base classes: CanBattery@0
export const KIA_EGMP_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['renderer', 'KiaEGMPHtmlRenderer'],
  ['', ' ', 8],
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
  ['batteryRelay', 'u8'],
  ['waterleakageSensor', 'u8'],
  ['startedUp', 'b'],
  ['ok_start_polling_battery', 'b'],
  ['counter_200', 'u8'],
  ['KIA_7E4_COUNTER', 'u8'],
  ['temperature_water_inlet', 'i8'],
  ['powerRelayTemperature', 'i8'],
  ['heatertemp', 'i8'],
  ['set_voltage_limits', 'b'],
  ['numPoints', 'u8'],
  ['', ' '],
  ['SOC', '9b03'],
  ['', ' ', 202],
  ['voltage', '9b03'],
  ['', ' ', 202],
  ['startMillis', 'u32'],
  ['messageIndex', 'u8'],
  ['messageDelays', 'u8', 63],
  ['messages', '9b18'],
  ['', ' ', 256],
  ['EGMP_7E4', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const renderer: [number, string] = [28, 'KiaEGMPHtmlRenderer'];
export const previousMillis200ms: [number, string] = [36, 'u32'];
export const previousMillis10s: [number, string] = [40, 'u32'];
export const inverterVoltageFrameHigh: [number, string] = [44, 'u16'];
export const inverterVoltage: [number, string] = [46, 'u16'];
export const soc_calculated: [number, string] = [48, 'u16'];
export const SOC_BMS: [number, string] = [50, 'u16'];
export const SOC_Display: [number, string] = [52, 'u16'];
export const SOC_estimated_lowest: [number, string] = [54, 'u16'];
export const SOC_estimated_highest: [number, string] = [56, 'u16'];
export const batterySOH: [number, string] = [58, 'u16'];
export const CellVoltMax_mV: [number, string] = [60, 'u16'];
export const CellVoltMin_mV: [number, string] = [62, 'u16'];
export const batteryVoltage: [number, string] = [64, 'u16'];
export const leadAcidBatteryVoltage: [number, string] = [66, 'i16'];
export const batteryAmps: [number, string] = [68, 'i16'];
export const temperatureMax: [number, string] = [70, 'i16'];
export const temperatureMin: [number, string] = [72, 'i16'];
export const allowedDischargePower: [number, string] = [74, 'i16'];
export const allowedChargePower: [number, string] = [76, 'i16'];
export const poll_data_pid: [number, string] = [78, 'i16'];
export const CellVmaxNo: [number, string] = [80, 'u8'];
export const CellVminNo: [number, string] = [81, 'u8'];
export const batteryManagementMode: [number, string] = [82, 'u8'];
export const BMS_ign: [number, string] = [83, 'u8'];
export const batteryRelay: [number, string] = [84, 'u8'];
export const waterleakageSensor: [number, string] = [85, 'u8'];
export const startedUp: [number, string] = [86, 'b'];
export const ok_start_polling_battery: [number, string] = [87, 'b'];
export const counter_200: [number, string] = [88, 'u8'];
export const KIA_7E4_COUNTER: [number, string] = [89, 'u8'];
export const temperature_water_inlet: [number, string] = [90, 'i8'];
export const powerRelayTemperature: [number, string] = [91, 'i8'];
export const heatertemp: [number, string] = [92, 'i8'];
export const set_voltage_limits: [number, string] = [93, 'b'];
export const numPoints: [number, string] = [94, 'u8'];
export const SOC: [number, string] = [96, '9b03'];
export const voltage: [number, string] = [298, '9b03'];
export const startMillis: [number, string] = [500, 'u32'];
export const messageIndex: [number, string] = [504, 'u8'];
export const messageDelays: [number, string, number] = [505, 'u8', 63];
export const messages: [number, string] = [568, '9b18'];
export const EGMP_7E4: [number, string] = [824, 'CAN_frame'];
