// RangeRoverPhevBattery: 216 bytes; base classes: CanBattery@0
export const RANGE_ROVER_PHEV_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['previousMillis50ms', 'u32'],
  ['StatusCAT5BPOChg', 'b'],
  ['StatusCAT4Derate', 'b'],
  ['OCMonitorStatus', 'u8'],
  ['StatusCAT3', 'b'],
  ['IsolationStatus', 'b'],
  ['HVILStatus', 'b'],
  ['ContactorStatus', 'b'],
  ['StatusGpCounter', 'u8'],
  ['WeldCheckStatus', 'b'],
  ['StatusCAT7NowBPO', 'b'],
  ['StatusCAT6DlyBPO', 'b'],
  ['StatusGpCS', 'u8'],
  ['CAT6Count', 'u8'],
  ['EndOfCharge', 'b'],
  ['DerateWarning', 'b'],
  ['PrechargeAllowed', 'b'],
  ['DischargeExtGpCounter', 'u8'],
  ['DischargeExtGpCS', 'u8'],
  ['DischargeVoltageLimit', 'u16'],
  ['DischargePowerLimitExt', 'u16'],
  ['DischargeContPwrLmt', 'u16'],
  ['PwrGpCS', 'u8'],
  ['PwrGpCounter', 'u8'],
  ['VoltageExt', 'u16'],
  ['VoltageBus', 'u16'],
  ['', ' ', 2],
  ['CurrentExt', 'i32'],
  ['HVIsolationTestRunning', 'b'],
  ['', ' '],
  ['VoltageOC', 'u16'],
  ['DchCurrentLimit', 'u16'],
  ['ChgCurrentLimit', 'u16'],
  ['ChargeContPwrLmt', 'u16'],
  ['ChargePowerLimitExt', 'u16'],
  ['ChgExtGpCS', 'u8'],
  ['ChgExtGpCounter', 'u8'],
  ['ChargeVoltageLimit', 'u16'],
  ['CurrentWarning', 'u8'],
  ['TempWarning', 'u8'],
  ['TempUpLimit', 'i8'],
  ['CellVoltWarning', 'u8'],
  ['CCCVChargeMode', 'b'],
  ['', ' '],
  ['CellVoltUpLimit', 'u16'],
  ['SOCHighestCell', 'u16'],
  ['SOCLowestCell', 'u16'],
  ['SOCAverage', 'u16'],
  ['WakeUpTopUpReq', 'b'],
  ['WakeUpThermalReq', 'b'],
  ['WakeUpDchReq', 'b'],
  ['', ' '],
  ['StateofHealth', 'u16'],
  ['EstimatedLossChg', 'u16'],
  ['CoolingRequest', 'b'],
  ['', ' '],
  ['EstimatedLossDch', 'u16'],
  ['FanDutyRequest', 'u8'],
  ['ValveCtrlStat', 'b'],
  ['EstLossDchTgtSoC', 'u16'],
  ['HeatPowerGenChg', 'u8'],
  ['HeatPowerGenDch', 'u8'],
  ['WarmupRateChg', 'u8'],
  ['WarmupRateDch', 'u8'],
  ['CellVoltageMax', 'u16'],
  ['CellVoltageMin', 'u16'],
  ['CellTempAverage', 'i8'],
  ['CellTempColdest', 'i8'],
  ['CellTempHottest', 'i8'],
  ['HeaterCtrlStat', 'u8'],
  ['ThermalOvercheck', 'b'],
  ['InletCoolantTemp', 'i8'],
  ['ClntPumpDiagStat_UB', 'b'],
  ['InletCoolantTemp_UB', 'b'],
  ['CoolantLevel', 'b'],
  ['ClntPumpDiagStat', 'b'],
  ['MILRequest', 'u8'],
  ['', ' '],
  ['EnergyAvailable', 'u16'],
  ['EnergyUsableMax', 'u16'],
  ['EnergyUsableMin', 'u16'],
  ['TotalCapacity', 'u16'],
  ['', ' ', 6],
  ['RANGE_ROVER_18B', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const previousMillis50ms: [number, string] = [24, 'u32'];
export const StatusCAT5BPOChg: [number, string] = [28, 'b'];
export const StatusCAT4Derate: [number, string] = [29, 'b'];
export const OCMonitorStatus: [number, string] = [30, 'u8'];
export const StatusCAT3: [number, string] = [31, 'b'];
export const IsolationStatus: [number, string] = [32, 'b'];
export const HVILStatus: [number, string] = [33, 'b'];
export const ContactorStatus: [number, string] = [34, 'b'];
export const StatusGpCounter: [number, string] = [35, 'u8'];
export const WeldCheckStatus: [number, string] = [36, 'b'];
export const StatusCAT7NowBPO: [number, string] = [37, 'b'];
export const StatusCAT6DlyBPO: [number, string] = [38, 'b'];
export const StatusGpCS: [number, string] = [39, 'u8'];
export const CAT6Count: [number, string] = [40, 'u8'];
export const EndOfCharge: [number, string] = [41, 'b'];
export const DerateWarning: [number, string] = [42, 'b'];
export const PrechargeAllowed: [number, string] = [43, 'b'];
export const DischargeExtGpCounter: [number, string] = [44, 'u8'];
export const DischargeExtGpCS: [number, string] = [45, 'u8'];
export const DischargeVoltageLimit: [number, string] = [46, 'u16'];
export const DischargePowerLimitExt: [number, string] = [48, 'u16'];
export const DischargeContPwrLmt: [number, string] = [50, 'u16'];
export const PwrGpCS: [number, string] = [52, 'u8'];
export const PwrGpCounter: [number, string] = [53, 'u8'];
export const VoltageExt: [number, string] = [54, 'u16'];
export const VoltageBus: [number, string] = [56, 'u16'];
export const CurrentExt: [number, string] = [60, 'i32'];
export const HVIsolationTestRunning: [number, string] = [64, 'b'];
export const VoltageOC: [number, string] = [66, 'u16'];
export const DchCurrentLimit: [number, string] = [68, 'u16'];
export const ChgCurrentLimit: [number, string] = [70, 'u16'];
export const ChargeContPwrLmt: [number, string] = [72, 'u16'];
export const ChargePowerLimitExt: [number, string] = [74, 'u16'];
export const ChgExtGpCS: [number, string] = [76, 'u8'];
export const ChgExtGpCounter: [number, string] = [77, 'u8'];
export const ChargeVoltageLimit: [number, string] = [78, 'u16'];
export const CurrentWarning: [number, string] = [80, 'u8'];
export const TempWarning: [number, string] = [81, 'u8'];
export const TempUpLimit: [number, string] = [82, 'i8'];
export const CellVoltWarning: [number, string] = [83, 'u8'];
export const CCCVChargeMode: [number, string] = [84, 'b'];
export const CellVoltUpLimit: [number, string] = [86, 'u16'];
export const SOCHighestCell: [number, string] = [88, 'u16'];
export const SOCLowestCell: [number, string] = [90, 'u16'];
export const SOCAverage: [number, string] = [92, 'u16'];
export const WakeUpTopUpReq: [number, string] = [94, 'b'];
export const WakeUpThermalReq: [number, string] = [95, 'b'];
export const WakeUpDchReq: [number, string] = [96, 'b'];
export const StateofHealth: [number, string] = [98, 'u16'];
export const EstimatedLossChg: [number, string] = [100, 'u16'];
export const CoolingRequest: [number, string] = [102, 'b'];
export const EstimatedLossDch: [number, string] = [104, 'u16'];
export const FanDutyRequest: [number, string] = [106, 'u8'];
export const ValveCtrlStat: [number, string] = [107, 'b'];
export const EstLossDchTgtSoC: [number, string] = [108, 'u16'];
export const HeatPowerGenChg: [number, string] = [110, 'u8'];
export const HeatPowerGenDch: [number, string] = [111, 'u8'];
export const WarmupRateChg: [number, string] = [112, 'u8'];
export const WarmupRateDch: [number, string] = [113, 'u8'];
export const CellVoltageMax: [number, string] = [114, 'u16'];
export const CellVoltageMin: [number, string] = [116, 'u16'];
export const CellTempAverage: [number, string] = [118, 'i8'];
export const CellTempColdest: [number, string] = [119, 'i8'];
export const CellTempHottest: [number, string] = [120, 'i8'];
export const HeaterCtrlStat: [number, string] = [121, 'u8'];
export const ThermalOvercheck: [number, string] = [122, 'b'];
export const InletCoolantTemp: [number, string] = [123, 'i8'];
export const ClntPumpDiagStat_UB: [number, string] = [124, 'b'];
export const InletCoolantTemp_UB: [number, string] = [125, 'b'];
export const CoolantLevel: [number, string] = [126, 'b'];
export const ClntPumpDiagStat: [number, string] = [127, 'b'];
export const MILRequest: [number, string] = [128, 'u8'];
export const EnergyAvailable: [number, string] = [130, 'u16'];
export const EnergyUsableMax: [number, string] = [132, 'u16'];
export const EnergyUsableMin: [number, string] = [134, 'u16'];
export const TotalCapacity: [number, string] = [136, 'u16'];
export const RANGE_ROVER_18B: [number, string] = [144, 'CAN_frame'];
