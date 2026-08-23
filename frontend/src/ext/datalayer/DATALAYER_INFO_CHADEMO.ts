export const DATALAYER_INFO_CHADEMO_FIELDS: ([string, string] | [string, string, number])[] = [
  ['CHADEMO_Status', 'u8'],
  ['ControlProtocolNumberEV', 'u8'],
  ['UserRequestRestart', 'b'],
  ['UserRequestStop', 'b'],
  ['FaultBatteryVoltageDeviation', 'b'],
  ['FaultHighBatteryTemperature', 'b'],
  ['FaultBatteryCurrentDeviation', 'b'],
  ['FaultBatteryUnderVoltage', 'b'],
  ['FaultBatteryOverVoltage', 'b'],
];

export const CHADEMO_Status: [number, string] = [0, 'u8'];
export const ControlProtocolNumberEV: [number, string] = [1, 'u8'];
export const UserRequestRestart: [number, string] = [2, 'b'];
export const UserRequestStop: [number, string] = [3, 'b'];
export const FaultBatteryVoltageDeviation: [number, string] = [4, 'b'];
export const FaultHighBatteryTemperature: [number, string] = [5, 'b'];
export const FaultBatteryCurrentDeviation: [number, string] = [6, 'b'];
export const FaultBatteryUnderVoltage: [number, string] = [7, 'b'];
export const FaultBatteryOverVoltage: [number, string] = [8, 'b'];
