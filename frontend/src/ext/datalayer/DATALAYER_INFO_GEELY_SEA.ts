export const DATALAYER_INFO_GEELY_SEA_FIELDS: ([string, string] | [string, string, number])[] = [
  ['soc_bms', 'u16'],
  ['soh_bms', 'u16'],
  ['BECMsupplyVoltage', 'u16'],
  ['BECMBatteryVoltage', 'u16'],
  ['BatteryCurrent', 'u16'],
  ['CellTempHighest', 'u16'],
  ['CellTempAverage', 'u16'],
  ['CellTempLowest', 'u16'],
  ['Interlock', 'u8'],
  ['', ' '],
  ['CellVoltHighest', 'u16'],
  ['CellVoltLowest', 'u16'],
  ['DTCcount', 'u8'],
  ['CrashStatus', 'u8'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestBECMecuReset', 'b'],
  ['UserRequestCrashReset', 'b'],
];

export const soc_bms: [number, string] = [0, 'u16'];
export const soh_bms: [number, string] = [2, 'u16'];
export const BECMsupplyVoltage: [number, string] = [4, 'u16'];
export const BECMBatteryVoltage: [number, string] = [6, 'u16'];
export const BatteryCurrent: [number, string] = [8, 'u16'];
export const CellTempHighest: [number, string] = [10, 'u16'];
export const CellTempAverage: [number, string] = [12, 'u16'];
export const CellTempLowest: [number, string] = [14, 'u16'];
export const Interlock: [number, string] = [16, 'u8'];
export const CellVoltHighest: [number, string] = [18, 'u16'];
export const CellVoltLowest: [number, string] = [20, 'u16'];
export const DTCcount: [number, string] = [22, 'u8'];
export const CrashStatus: [number, string] = [23, 'u8'];
export const UserRequestDTCreset: [number, string] = [24, 'b'];
export const UserRequestDTCreadout: [number, string] = [25, 'b'];
export const UserRequestBECMecuReset: [number, string] = [26, 'b'];
export const UserRequestCrashReset: [number, string] = [27, 'b'];
