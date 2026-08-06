// HyundaiIoniq28Battery: 928 bytes; base classes: CanBattery@0
export const HYUNDAI_IONIQ28_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'HyundaiIoniq28BatteryHtmlRenderer'],
  ['', ' ', 8],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['previousMillis250', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis10', 'u32'],
  ['SOC_BMS', 'u16'],
  ['SOC_Display', 'u16'],
  ['batterySOH', 'u16'],
  ['CellVoltMax_mV', 'u16'],
  ['CellVoltMin_mV', 'u16'],
  ['allowedDischargePower', 'u16'],
  ['allowedChargePower', 'u16'],
  ['batteryVoltage', 'u16'],
  ['inverterVoltage', 'u16'],
  ['isolation_resistance', 'u16'],
  ['cellvoltages_mv', 'u16', 96],
  ['leadAcidBatteryVoltage', 'u16'],
  ['batteryAmps', 'i16'],
  ['temperatureMax', 'i16'],
  ['temperatureMin', 'i16'],
  ['batteryManagementMode', 'u8'],
  ['counter_200', 'u8'],
  ['heatertemperature_1', 'i8'],
  ['heatertemperature_2', 'i8'],
  ['powerRelayTemperature', 'i8'],
  ['startedUp', 'b'],
  ['incoming_poll_group', 'u8'],
  ['poll_group', 'u8'],
  ['IONIQ_200', 'CAN_frame'],
  ['IONIQ_523', 'CAN_frame'],
  ['IONIQ_524', 'CAN_frame'],
  ['IONIQ_553', 'CAN_frame'],
  ['IONIQ_57F', 'CAN_frame'],
  ['IONIQ_2A1', 'CAN_frame'],
  ['IONIQ_7E4_POLL', 'CAN_frame'],
  ['IONIQ_7E4_ACK', 'CAN_frame'],
  ['IONIQ_CLEAR_DTC', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'HyundaiIoniq28BatteryHtmlRenderer'];
export const datalayer_battery: [number, string] = [32, 'DATALAYER_BATTERY_TYPE*'];
export const UserRequestDTCreset: [number, string] = [36, 'b'];
export const previousMillis250: [number, string] = [40, 'u32'];
export const previousMillis100: [number, string] = [44, 'u32'];
export const previousMillis10: [number, string] = [48, 'u32'];
export const SOC_BMS: [number, string] = [52, 'u16'];
export const SOC_Display: [number, string] = [54, 'u16'];
export const batterySOH: [number, string] = [56, 'u16'];
export const CellVoltMax_mV: [number, string] = [58, 'u16'];
export const CellVoltMin_mV: [number, string] = [60, 'u16'];
export const allowedDischargePower: [number, string] = [62, 'u16'];
export const allowedChargePower: [number, string] = [64, 'u16'];
export const batteryVoltage: [number, string] = [66, 'u16'];
export const inverterVoltage: [number, string] = [68, 'u16'];
export const isolation_resistance: [number, string] = [70, 'u16'];
export const cellvoltages_mv: [number, string, number] = [72, 'u16', 96];
export const leadAcidBatteryVoltage: [number, string] = [264, 'u16'];
export const batteryAmps: [number, string] = [266, 'i16'];
export const temperatureMax: [number, string] = [268, 'i16'];
export const temperatureMin: [number, string] = [270, 'i16'];
export const batteryManagementMode: [number, string] = [272, 'u8'];
export const counter_200: [number, string] = [273, 'u8'];
export const heatertemperature_1: [number, string] = [274, 'i8'];
export const heatertemperature_2: [number, string] = [275, 'i8'];
export const powerRelayTemperature: [number, string] = [276, 'i8'];
export const startedUp: [number, string] = [277, 'b'];
export const incoming_poll_group: [number, string] = [278, 'u8'];
export const poll_group: [number, string] = [279, 'u8'];
export const IONIQ_200: [number, string] = [280, 'CAN_frame'];
export const IONIQ_523: [number, string] = [352, 'CAN_frame'];
export const IONIQ_524: [number, string] = [424, 'CAN_frame'];
export const IONIQ_553: [number, string] = [496, 'CAN_frame'];
export const IONIQ_57F: [number, string] = [568, 'CAN_frame'];
export const IONIQ_2A1: [number, string] = [640, 'CAN_frame'];
export const IONIQ_7E4_POLL: [number, string] = [712, 'CAN_frame'];
export const IONIQ_7E4_ACK: [number, string] = [784, 'CAN_frame'];
export const IONIQ_CLEAR_DTC: [number, string] = [856, 'CAN_frame'];
