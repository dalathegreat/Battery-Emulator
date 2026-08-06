export const DATALAYER_INFO_ZOE_FIELDS: ([string, string] | [string, string, number])[] = [
  ['mileage_km', 'u16'],
  ['alltime_kWh', 'u16'],
  ['CUV', 'u8'],
  ['HVBIR', 'u8'],
  ['HVBUV', 'u8'],
  ['EOCR', 'u8'],
  ['HVBOC', 'u8'],
  ['HVBOT', 'u8'],
  ['HVBOV', 'u8'],
  ['COV', 'u8'],
];

export const mileage_km: [number, string] = [0, 'u16'];
export const alltime_kWh: [number, string] = [2, 'u16'];
export const CUV: [number, string] = [4, 'u8'];
export const HVBIR: [number, string] = [5, 'u8'];
export const HVBUV: [number, string] = [6, 'u8'];
export const EOCR: [number, string] = [7, 'u8'];
export const HVBOC: [number, string] = [8, 'u8'];
export const HVBOT: [number, string] = [9, 'u8'];
export const HVBOV: [number, string] = [10, 'u8'];
export const COV: [number, string] = [11, 'u8'];
