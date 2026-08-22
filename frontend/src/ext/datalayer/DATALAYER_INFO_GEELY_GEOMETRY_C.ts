export const DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS: ([string, string] | [string, string, number])[] = [
  ['ModuleTemperatures', 'i16', 6],
  ['BatterySoftwareVersion', 'u8', 16],
  ['BatteryHardwareVersion', 'u8', 16],
  ['BatterySerialNumber', 'u8', 28],
  ['soc', 'u16'],
  ['CC2voltage', 'u16'],
  ['cellMaxVoltageNumber', 'u16'],
  ['cellMinVoltageNumber', 'u16'],
  ['cellTotalAmount', 'u16'],
  ['specificialVoltage', 'u16'],
  ['unknown1', 'u16'],
  ['rawSOCmax', 'u16'],
  ['rawSOCmin', 'u16'],
  ['unknown4', 'u16'],
  ['capModMax', 'u16'],
  ['capModMin', 'u16'],
  ['unknown7', 'u16'],
  ['unknown8', 'u16'],
];

export const ModuleTemperatures: [number, string, number] = [0, 'i16', 6];
export const BatterySoftwareVersion: [number, string, number] = [12, 'u8', 16];
export const BatteryHardwareVersion: [number, string, number] = [28, 'u8', 16];
export const BatterySerialNumber: [number, string, number] = [44, 'u8', 28];
export const soc: [number, string] = [72, 'u16'];
export const CC2voltage: [number, string] = [74, 'u16'];
export const cellMaxVoltageNumber: [number, string] = [76, 'u16'];
export const cellMinVoltageNumber: [number, string] = [78, 'u16'];
export const cellTotalAmount: [number, string] = [80, 'u16'];
export const specificialVoltage: [number, string] = [82, 'u16'];
export const unknown1: [number, string] = [84, 'u16'];
export const rawSOCmax: [number, string] = [86, 'u16'];
export const rawSOCmin: [number, string] = [88, 'u16'];
export const unknown4: [number, string] = [90, 'u16'];
export const capModMax: [number, string] = [92, 'u16'];
export const capModMin: [number, string] = [94, 'u16'];
export const unknown7: [number, string] = [96, 'u16'];
export const unknown8: [number, string] = [98, 'u16'];
