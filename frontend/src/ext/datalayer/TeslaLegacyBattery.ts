// TeslaLegacyBattery: 544 bytes; base classes: CanBattery@0
export const TESLA_LEGACY_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['TESLA_25C', 'CAN_frame'],
  ['TESLA_2C8', 'CAN_frame'],
  ['TESLA_21C', 'CAN_frame'],
  ['TESLA_20E', 'CAN_frame'],
  ['TESLA_602', 'CAN_frame'],
  ['TESLA_408', 'CAN_frame'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['BMS_CAC_min', 'u32'],
  ['battery_cell_max_v', 'u16'],
  ['battery_cell_min_v', 'u16'],
  ['battery_soc_ui', 'u16'],
  ['battery_BMS_state', 'u8'],
  ['cellvoltagesRead', 'b'],
  ['battery_volts', 'u16'],
  ['battery_amps', 'i16'],
  ['battery_max_discharge_current', 'u16'],
  ['battery_max_charge_current', 'u16'],
  ['battery_bms_max_voltage', 'u16'],
  ['battery_bms_min_voltage', 'u16'],
  ['battery_total_discharge', 'u32'],
  ['battery_total_charge', 'u32'],
  ['battery_max_temp', 'i16'],
  ['battery_min_temp', 'i16'],
  ['battery_BrickVoltageMax', 'u16'],
  ['battery_BrickVoltageMin', 'u16'],
  ['battery_BrickTempMaxNum', 'u8'],
  ['battery_BrickTempMinNum', 'u8'],
  ['battery_BrickModelTMax', 'u8'],
  ['battery_BrickModelTMin', 'u8'],
  ['battery_BrickVoltageMaxNum', 'u8'],
  ['battery_BrickVoltageMinNum', 'u8'],
  ['battery_hwID', 'u8'],
  ['battery_BMS_rapidDCLinkDchgRequest', 'b'],
  ['battery_BMS_chargingActiveOrTrans', 'b'],
  ['battery_BMS_dcdcEnableOn', 'b'],
  ['battery_BMS_hvilStatus', 'b'],
  ['battery_BMS_okToShipByAir', 'b'],
  ['battery_BMS_okToShipByLand', 'b'],
  ['battery_BMS_activeHeatingWorthwhile', 'b'],
  ['battery_BMS_notEnoughPowerForSupport', 'b'],
  ['battery_BMS_hvacPowerRequest', 'b'],
  ['battery_BMS_notEnoughPowerForDrive', 'b'],
  ['battery_BMS_hvilOn', 'b'],
  ['battery_BMS_highestFaultCategory', 'u8'],
  ['battery_BMS_contactorStateFC', 'u8'],
  ['battery_BMS_cpChargeStatus', 'u8'],
  ['', ' '],
  ['battery_BMS_isolationResistance', 'u16'],
  ['battery_BMS_contactorState', 'u8'],
  ['stateMachineBMSReset', 'u8'],
  ['bms_uds_response', 'u8'],
  ['user_requests_bms_reset', 'b'],
  ['bms_uds_response_received', 'b'],
  ['', ' ', 3],
  ['bms_uds_timeout', 'u32'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const TESLA_25C: [number, string] = [24, 'CAN_frame'];
export const TESLA_2C8: [number, string] = [96, 'CAN_frame'];
export const TESLA_21C: [number, string] = [168, 'CAN_frame'];
export const TESLA_20E: [number, string] = [240, 'CAN_frame'];
export const TESLA_602: [number, string] = [312, 'CAN_frame'];
export const TESLA_408: [number, string] = [384, 'CAN_frame'];
export const previousMillis100: [number, string] = [456, 'u32'];
export const previousMillis1000: [number, string] = [460, 'u32'];
export const BMS_CAC_min: [number, string] = [464, 'u32'];
export const battery_cell_max_v: [number, string] = [468, 'u16'];
export const battery_cell_min_v: [number, string] = [470, 'u16'];
export const battery_soc_ui: [number, string] = [472, 'u16'];
export const battery_BMS_state: [number, string] = [474, 'u8'];
export const cellvoltagesRead: [number, string] = [475, 'b'];
export const battery_volts: [number, string] = [476, 'u16'];
export const battery_amps: [number, string] = [478, 'i16'];
export const battery_max_discharge_current: [number, string] = [480, 'u16'];
export const battery_max_charge_current: [number, string] = [482, 'u16'];
export const battery_bms_max_voltage: [number, string] = [484, 'u16'];
export const battery_bms_min_voltage: [number, string] = [486, 'u16'];
export const battery_total_discharge: [number, string] = [488, 'u32'];
export const battery_total_charge: [number, string] = [492, 'u32'];
export const battery_max_temp: [number, string] = [496, 'i16'];
export const battery_min_temp: [number, string] = [498, 'i16'];
export const battery_BrickVoltageMax: [number, string] = [500, 'u16'];
export const battery_BrickVoltageMin: [number, string] = [502, 'u16'];
export const battery_BrickTempMaxNum: [number, string] = [504, 'u8'];
export const battery_BrickTempMinNum: [number, string] = [505, 'u8'];
export const battery_BrickModelTMax: [number, string] = [506, 'u8'];
export const battery_BrickModelTMin: [number, string] = [507, 'u8'];
export const battery_BrickVoltageMaxNum: [number, string] = [508, 'u8'];
export const battery_BrickVoltageMinNum: [number, string] = [509, 'u8'];
export const battery_hwID: [number, string] = [510, 'u8'];
export const battery_BMS_rapidDCLinkDchgRequest: [number, string] = [511, 'b'];
export const battery_BMS_chargingActiveOrTrans: [number, string] = [512, 'b'];
export const battery_BMS_dcdcEnableOn: [number, string] = [513, 'b'];
export const battery_BMS_hvilStatus: [number, string] = [514, 'b'];
export const battery_BMS_okToShipByAir: [number, string] = [515, 'b'];
export const battery_BMS_okToShipByLand: [number, string] = [516, 'b'];
export const battery_BMS_activeHeatingWorthwhile: [number, string] = [517, 'b'];
export const battery_BMS_notEnoughPowerForSupport: [number, string] = [518, 'b'];
export const battery_BMS_hvacPowerRequest: [number, string] = [519, 'b'];
export const battery_BMS_notEnoughPowerForDrive: [number, string] = [520, 'b'];
export const battery_BMS_hvilOn: [number, string] = [521, 'b'];
export const battery_BMS_highestFaultCategory: [number, string] = [522, 'u8'];
export const battery_BMS_contactorStateFC: [number, string] = [523, 'u8'];
export const battery_BMS_cpChargeStatus: [number, string] = [524, 'u8'];
export const battery_BMS_isolationResistance: [number, string] = [526, 'u16'];
export const battery_BMS_contactorState: [number, string] = [528, 'u8'];
export const stateMachineBMSReset: [number, string] = [529, 'u8'];
export const bms_uds_response: [number, string] = [530, 'u8'];
export const user_requests_bms_reset: [number, string] = [531, 'b'];
export const bms_uds_response_received: [number, string] = [532, 'b'];
export const bms_uds_timeout: [number, string] = [536, 'u32'];
