// ChademoBattery: 528 bytes; base classes: CanBattery@0
export const CHADEMO_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['MinimumChargeCurrent', 'u8'],
  ['MaxChargingTime10sBit', 'u8'],
  ['unused_3', 'b'],
  ['unused_2', 'b'],
  ['unused_1', 'b'],
  ['FaultBatteryVoltageDeviation', 'b'],
  ['FaultHighBatteryTemperature', 'b'],
  ['FaultBatteryCurrentDeviation', 'b'],
  ['FaultBatteryUnderVoltage', 'b'],
  ['FaultBatteryOverVoltage', 'b'],
  ['StatusVehicleDischargeCompatible', 'b'],
  ['unused_2_2', 'b'],
  ['unused_1_2', 'b'],
  ['StatusNormalStopRequest', 'b'],
  ['StatusVehicle', 'b'],
  ['StatusChargingError', 'b'],
  ['StatusVehicleShifterPosition', 'b'],
  ['StatusVehicleChargingEnabled', 'b'],
  ['ControlProtocolNumberEV', 'u8'],
  ['contactor_weld_detection', 'b'],
  ['EVSE_status', 'b'],
  ['EVSE_error', 'b'],
  ['connector_locked', 'b'],
  ['battery_incompatible', 'b'],
  ['ChgDischError', 'b'],
  ['ChgDischStopControl', 'b'],
  ['CHADEMO_protocol_number', 'u8'],
  ['MaximumDischargeCurrent', 'u8'],
  ['V2HchargeDischargeSequenceNum', 'u8'],
  ['present_discharge_current', 'u8'],
  ['sequence_control_number', 'u8'],
  ['PermissionResetMaxChgTime', 'b'],
  ['unused_3_2', 'b'],
  ['unused_2_3', 'b'],
  ['unused_1_3', 'b'],
  ['HighVoltageControlStatus', 'b'],
  ['HighCurrentControlStatus', 'b'],
  ['DynamicControlStatus', 'b'],
  ['u', '14896'],
  ['PermissionResetMaxChgTime_2', 'b'],
  ['unused_3_3', 'b'],
  ['unused_2_4', 'b'],
  ['unused_1_4', 'b'],
  ['HighVoltageControlStatus_2', 'b'],
  ['HighCurrentControlStatus_2', 'b'],
  ['DynamicControlStatus_2', 'b'],
  ['u_2', '1494d'],
  ['AutomakerCode', 'u8'],
  ['MaxChargingTime1minBit', 'u8'],
  ['OptionalContent', 'u8'],
  ['MinimumBatteryVoltage', 'u16'],
  ['EstimatedChargingTime', 'u8'],
  ['TargetBatteryVoltage', 'u16'],
  ['available_output_voltage', 'u16'],
  ['setpoint_HV_VDC', 'u16'],
  ['MinimumDischargeVoltage', 'u16'],
  ['ApproxDischargeCompletionTime', 'u16'],
  ['available_input_voltage', 'u16'],
  ['remaining_discharge_time', 'u16'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['MaximumBatteryVoltage', 'u16'],
  ['RatedBatteryCapacity', 'u16'],
  ['ChargingCurrentRequest', 'u8'],
  ['available_output_current', 'u8'],
  ['setpoint_HV_IDC', 'u8'],
  ['MinimumBatteryDischargeLevel', 'u16'],
  ['AvailableVehicleEnergy', 'u16'],
  ['available_input_current', 'u16'],
  ['f', '144cd'],
  ['discharge_compatible', 'b'],
  ['ConstantOfChargingRateIndication', 'u8'],
  ['s', '1457a'],
  ['threshold_voltage', 'u16'],
  ['s_2', '146c1'],
  ['MaxRemainingCapacityForCharging', 'u16'],
  ['lower_threshold_voltage', 'u16'],
  ['StateOfCharge', 'u8'],
  ['remaining_time_10s', 'u8'],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['remaining_time_1m', 'u8'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['pin2', 'gpio_num_t'],
  ['pin10', 'gpio_num_t'],
  ['pin4', 'gpio_num_t'],
  ['pin7', 'gpio_num_t'],
  ['pin_lock', 'gpio_num_t'],
  ['precharge', 'gpio_num_t'],
  ['positive_contactor', 'gpio_num_t'],
  ['renderer', 'ChademoBatteryHtmlRenderer'],
  ['setupMillis', 'u32'],
  ['handlerBeforeMillis', 'u32'],
  ['handlerAfterMillis', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis5000', 'u32'],
  ['plug_inserted', 'b'],
  ['vehicle_can_initialized', 'b'],
  ['vehicle_can_received', 'b'],
  ['vehicle_permission', 'b'],
  ['evse_permission', 'b'],
  ['precharge_low', 'b'],
  ['positive_high', 'b'],
  ['contactors_ready', 'b'],
  ['framecount', 'u8'],
  ['max_discharge_current', 'u8'],
  ['high_current_control_enabled', 'b'],
  ['EVSE_mode', 'Mode'],
  ['CHADEMO_Status', 'u8'],
  ['x201_received', 'b'],
  ['x209_sent', 'b'],
  ['x100_chg_lim', 'x100_Vehicle_Charging_Limits'],
  ['x101_chg_est', 'x101_Vehicle_Charging_Estimate'],
  ['x102_chg_session', 'x102_Vehicle_Charging_Session'],
  ['x110_vehicle_dyn', 'x110_Vehicle_Dynamic_Control'],
  ['x200_discharge_limits', 'x200_Vehicle_Discharge_Limits'],
  ['x201_discharge_estimate', 'x201_Vehicle_Discharge_Estimate'],
  ['x700_vendor_id', 'x700_Vehicle_Vendor_ID'],
  ['x209_evse_dischg_est', 'x209_EVSE_Discharge_Estimate'],
  ['x108_evse_cap', 'x108_EVSE_Capabilities'],
  ['x109_evse_state', 'x109_EVSE_Status'],
  ['x118_evse_dyn', 'x118_EVSE_Dynamic_Control'],
  ['x208_evse_dischg_cap', 'x208_EVSE_Discharge_Capability'],
  ['CHADEMO_108', 'CAN_frame'],
  ['CHADEMO_109', 'CAN_frame'],
  ['CHADEMO_118', 'CAN_frame'],
  ['CHADEMO_208', 'CAN_frame'],
  ['CHADEMO_209', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const MinimumChargeCurrent: [number, string] = [0, 'u8'];
export const MaxChargingTime10sBit: [number, string] = [0, 'u8'];
export const unused_3: [number, string] = [0, 'b'];
export const unused_2: [number, string] = [0, 'b'];
export const unused_1: [number, string] = [0, 'b'];
export const FaultBatteryVoltageDeviation: [number, string] = [0, 'b'];
export const FaultHighBatteryTemperature: [number, string] = [0, 'b'];
export const FaultBatteryCurrentDeviation: [number, string] = [0, 'b'];
export const FaultBatteryUnderVoltage: [number, string] = [0, 'b'];
export const FaultBatteryOverVoltage: [number, string] = [0, 'b'];
export const StatusVehicleDischargeCompatible: [number, string] = [0, 'b'];
export const unused_2_2: [number, string] = [0, 'b'];
export const unused_1_2: [number, string] = [0, 'b'];
export const StatusNormalStopRequest: [number, string] = [0, 'b'];
export const StatusVehicle: [number, string] = [0, 'b'];
export const StatusChargingError: [number, string] = [0, 'b'];
export const StatusVehicleShifterPosition: [number, string] = [0, 'b'];
export const StatusVehicleChargingEnabled: [number, string] = [0, 'b'];
export const ControlProtocolNumberEV: [number, string] = [0, 'u8'];
export const contactor_weld_detection: [number, string] = [0, 'b'];
export const EVSE_status: [number, string] = [0, 'b'];
export const EVSE_error: [number, string] = [0, 'b'];
export const connector_locked: [number, string] = [0, 'b'];
export const battery_incompatible: [number, string] = [0, 'b'];
export const ChgDischError: [number, string] = [0, 'b'];
export const ChgDischStopControl: [number, string] = [0, 'b'];
export const CHADEMO_protocol_number: [number, string] = [0, 'u8'];
export const MaximumDischargeCurrent: [number, string] = [0, 'u8'];
export const V2HchargeDischargeSequenceNum: [number, string] = [0, 'u8'];
export const present_discharge_current: [number, string] = [0, 'u8'];
export const sequence_control_number: [number, string] = [0, 'u8'];
export const PermissionResetMaxChgTime: [number, string] = [0, 'b'];
export const unused_3_2: [number, string] = [0, 'b'];
export const unused_2_3: [number, string] = [0, 'b'];
export const unused_1_3: [number, string] = [0, 'b'];
export const HighVoltageControlStatus: [number, string] = [0, 'b'];
export const HighCurrentControlStatus: [number, string] = [0, 'b'];
export const DynamicControlStatus: [number, string] = [0, 'b'];
export const u: [number, string] = [0, '14896'];
export const PermissionResetMaxChgTime_2: [number, string] = [0, 'b'];
export const unused_3_3: [number, string] = [0, 'b'];
export const unused_2_4: [number, string] = [0, 'b'];
export const unused_1_4: [number, string] = [0, 'b'];
export const HighVoltageControlStatus_2: [number, string] = [0, 'b'];
export const HighCurrentControlStatus_2: [number, string] = [0, 'b'];
export const DynamicControlStatus_2: [number, string] = [0, 'b'];
export const u_2: [number, string] = [0, '1494d'];
export const AutomakerCode: [number, string] = [0, 'u8'];
export const MaxChargingTime1minBit: [number, string] = [1, 'u8'];
export const OptionalContent: [number, string] = [1, 'u8'];
export const MinimumBatteryVoltage: [number, string] = [2, 'u16'];
export const EstimatedChargingTime: [number, string] = [2, 'u8'];
export const TargetBatteryVoltage: [number, string] = [2, 'u16'];
export const available_output_voltage: [number, string] = [2, 'u16'];
export const setpoint_HV_VDC: [number, string] = [2, 'u16'];
export const MinimumDischargeVoltage: [number, string] = [2, 'u16'];
export const ApproxDischargeCompletionTime: [number, string] = [2, 'u16'];
export const available_input_voltage: [number, string] = [2, 'u16'];
export const remaining_discharge_time: [number, string] = [2, 'u16'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const MaximumBatteryVoltage: [number, string] = [4, 'u16'];
export const RatedBatteryCapacity: [number, string] = [4, 'u16'];
export const ChargingCurrentRequest: [number, string] = [4, 'u8'];
export const available_output_current: [number, string] = [4, 'u8'];
export const setpoint_HV_IDC: [number, string] = [4, 'u8'];
export const MinimumBatteryDischargeLevel: [number, string] = [4, 'u16'];
export const AvailableVehicleEnergy: [number, string] = [4, 'u16'];
export const available_input_current: [number, string] = [4, 'u16'];
export const f: [number, string] = [5, '144cd'];
export const discharge_compatible: [number, string] = [5, 'b'];
export const ConstantOfChargingRateIndication: [number, string] = [6, 'u8'];
export const s: [number, string] = [6, '1457a'];
export const threshold_voltage: [number, string] = [6, 'u16'];
export const s_2: [number, string] = [6, '146c1'];
export const MaxRemainingCapacityForCharging: [number, string] = [6, 'u16'];
export const lower_threshold_voltage: [number, string] = [6, 'u16'];
export const StateOfCharge: [number, string] = [7, 'u8'];
export const remaining_time_10s: [number, string] = [7, 'u8'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const remaining_time_1m: [number, string] = [8, 'u8'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const pin2: [number, string] = [24, 'gpio_num_t'];
export const pin10: [number, string] = [28, 'gpio_num_t'];
export const pin4: [number, string] = [32, 'gpio_num_t'];
export const pin7: [number, string] = [36, 'gpio_num_t'];
export const pin_lock: [number, string] = [40, 'gpio_num_t'];
export const precharge: [number, string] = [44, 'gpio_num_t'];
export const positive_contactor: [number, string] = [48, 'gpio_num_t'];
export const renderer: [number, string] = [52, 'ChademoBatteryHtmlRenderer'];
export const setupMillis: [number, string] = [56, 'u32'];
export const handlerBeforeMillis: [number, string] = [60, 'u32'];
export const handlerAfterMillis: [number, string] = [64, 'u32'];
export const previousMillis100: [number, string] = [68, 'u32'];
export const previousMillis5000: [number, string] = [72, 'u32'];
export const plug_inserted: [number, string] = [76, 'b'];
export const vehicle_can_initialized: [number, string] = [77, 'b'];
export const vehicle_can_received: [number, string] = [78, 'b'];
export const vehicle_permission: [number, string] = [79, 'b'];
export const evse_permission: [number, string] = [80, 'b'];
export const precharge_low: [number, string] = [81, 'b'];
export const positive_high: [number, string] = [82, 'b'];
export const contactors_ready: [number, string] = [83, 'b'];
export const framecount: [number, string] = [84, 'u8'];
export const max_discharge_current: [number, string] = [85, 'u8'];
export const high_current_control_enabled: [number, string] = [86, 'b'];
export const EVSE_mode: [number, string] = [88, 'Mode'];
export const CHADEMO_Status: [number, string] = [92, 'u8'];
export const x201_received: [number, string] = [93, 'b'];
export const x209_sent: [number, string] = [94, 'b'];
export const x100_chg_lim: [number, string] = [96, 'x100_Vehicle_Charging_Limits'];
export const x101_chg_est: [number, string] = [104, 'x101_Vehicle_Charging_Estimate'];
export const x102_chg_session: [number, string] = [110, 'x102_Vehicle_Charging_Session'];
export const x110_vehicle_dyn: [number, string] = [118, 'x110_Vehicle_Dynamic_Control'];
export const x200_discharge_limits: [number, string] = [120, 'x200_Vehicle_Discharge_Limits'];
export const x201_discharge_estimate: [number, string] = [128, 'x201_Vehicle_Discharge_Estimate'];
export const x700_vendor_id: [number, string] = [134, 'x700_Vehicle_Vendor_ID'];
export const x209_evse_dischg_est: [number, string] = [136, 'x209_EVSE_Discharge_Estimate'];
export const x108_evse_cap: [number, string] = [140, 'x108_EVSE_Capabilities'];
export const x109_evse_state: [number, string] = [148, 'x109_EVSE_Status'];
export const x118_evse_dyn: [number, string] = [158, 'x118_EVSE_Dynamic_Control'];
export const x208_evse_dischg_cap: [number, string] = [160, 'x208_EVSE_Discharge_Capability'];
export const CHADEMO_108: [number, string] = [168, 'CAN_frame'];
export const CHADEMO_109: [number, string] = [240, 'CAN_frame'];
export const CHADEMO_118: [number, string] = [312, 'CAN_frame'];
export const CHADEMO_208: [number, string] = [384, 'CAN_frame'];
export const CHADEMO_209: [number, string] = [456, 'CAN_frame'];
