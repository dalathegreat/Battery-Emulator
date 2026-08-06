export const DATALAYER_INFO_VOLVO_POLESTAR_FIELDS: ([string, string] | [string, string, number])[] = [
  ['BECMsupplyVoltage', 'u16'],
  ['BECMUDynMaxLim', 'u16'],
  ['BECMUDynMinLim', 'u16'],
  ['HvBattPwrLimDcha1', 'u16'],
  ['HvBattPwrLimDchaSoft', 'u16'],
  ['HvBattPwrLimDchaSlowAgi', 'u16'],
  ['HvBattPwrLimChrgSlowAgi', 'u16'],
  ['HVSysRlySts', 'u8'],
  ['HVSysDCRlySts1', 'u8'],
  ['HVSysDCRlySts2', 'u8'],
  ['HVSysIsoRMonrSts', 'u8'],
  ['HVILstatusBits', 'u8'],
];

export const BECMsupplyVoltage: [number, string] = [0, 'u16'];
export const BECMUDynMaxLim: [number, string] = [2, 'u16'];
export const BECMUDynMinLim: [number, string] = [4, 'u16'];
export const HvBattPwrLimDcha1: [number, string] = [6, 'u16'];
export const HvBattPwrLimDchaSoft: [number, string] = [8, 'u16'];
export const HvBattPwrLimDchaSlowAgi: [number, string] = [10, 'u16'];
export const HvBattPwrLimChrgSlowAgi: [number, string] = [12, 'u16'];
export const HVSysRlySts: [number, string] = [14, 'u8'];
export const HVSysDCRlySts1: [number, string] = [15, 'u8'];
export const HVSysDCRlySts2: [number, string] = [16, 'u8'];
export const HVSysIsoRMonrSts: [number, string] = [17, 'u8'];
export const HVILstatusBits: [number, string] = [18, 'u8'];
