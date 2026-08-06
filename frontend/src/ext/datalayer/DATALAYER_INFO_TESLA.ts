export const DATALAYER_INFO_TESLA_FIELDS: ([string, string] | [string, string, number])[] = [
  ['BMS_alertMatrixActive', 'b', 100],
  ['PCS_alertMatrixActive', 'b', 94],
  ['CP_alertMatrixActive', 'b', 96],
  ['', ' ', 6],
  ['BMS_info_bootGitHash', 'u64'],
  ['PCS_info_bootGitHash', 'u64'],
  ['HVP_info_bootGitHash', 'u64'],
  ['HVP_info_bootCrc', 'u32'],
  ['HVP_info_appCrc', 'u32'],
  ['PCS_info_appCrc', 'u32'],
  ['PCS_info_cpu2AppCrc', 'u32'],
  ['PCS_info_bootCrc', 'u32'],
  ['PCS_dcdc12vSupportLifetimekWh', 'u32'],
  ['BMS_info_appCrc', 'u32'],
  ['BMS_info_bootCrc', 'u32'],
  ['battery_packMass', 'u32'],
  ['battery_platformMaxBusVoltage', 'u32'],
  ['BMS_min_voltage', 'u32'],
  ['BMS_max_voltage', 'u32'],
  ['battery_max_charge_current', 'u32'],
  ['battery_max_discharge_current', 'u32'],
  ['battery_soc_min', 'u32'],
  ['battery_soc_max', 'u32'],
  ['battery_soc_ave', 'u32'],
  ['battery_soc_ui', 'u32'],
  ['BMS_info_buildConfigId', 'u16'],
  ['BMS_info_hardwareId', 'u16'],
  ['BMS_info_componentId', 'u16'],
  ['BMS_info_usageId', 'u16'],
  ['BMS_info_subUsageId', 'u16'],
  ['battery_dcdcLvBusVolt', 'u16'],
  ['battery_dcdcHvBusVolt', 'u16'],
  ['battery_dcdcLvOutputCurrent', 'u16'],
  ['battery_nominal_full_pack_energy', 'u16'],
  ['battery_nominal_full_pack_energy_m0', 'u16'],
  ['battery_nominal_energy_remaining', 'u16'],
  ['battery_nominal_energy_remaining_m0', 'u16'],
  ['battery_ideal_energy_remaining', 'u16'],
  ['battery_ideal_energy_remaining_m0', 'u16'],
  ['battery_energy_to_charge_complete', 'u16'],
  ['battery_energy_to_charge_complete_m1', 'u16'],
  ['battery_energy_buffer', 'u16'],
  ['battery_energy_buffer_m1', 'u16'],
  ['battery_expected_energy_remaining', 'u16'],
  ['battery_expected_energy_remaining_m1', 'u16'],
  ['battery_BrickVoltageMax', 'u16'],
  ['battery_BrickVoltageMin', 'u16'],
  ['HVP_hvp1v5Ref', 'u16'],
  ['HVP_shuntCurrentDebug', 'u16'],
  ['PCS_dcdcTemp', 'i16'],
  ['PCS_ambientTemp', 'i16'],
  ['PCS_chgPhATemp', 'i16'],
  ['PCS_chgPhBTemp', 'i16'],
  ['PCS_chgPhCTemp', 'i16'],
  ['PCS_dcdcMaxLvOutputCurrent', 'u16'],
  ['PCS_dcdcCurrentLimit', 'u16'],
  ['PCS_dcdcLvOutputCurrentTempLimit', 'u16'],
  ['PCS_dcdcUnifiedCommand', 'u16'],
  ['PCS_dcdcCLAControllerOutput', 'u16'],
  ['PCS_dcdcTankVoltage', 'u16'],
  ['PCS_dcdcTankVoltageTarget', 'u16'],
  ['PCS_dcdcClaCurrentFreq', 'u16'],
  ['PCS_dcdcTCommMeasured', 'u16'],
  ['PCS_dcdcShortTimeUs', 'u16'],
  ['PCS_dcdcHalfPeriodUs', 'u16'],
  ['PCS_dcdcIntervalMaxFrequency', 'u16'],
  ['PCS_dcdcIntervalMaxHvBusVolt', 'u16'],
  ['PCS_dcdcIntervalMaxLvBusVolt', 'u16'],
  ['PCS_dcdcIntervalMaxLvOutputCurr', 'u16'],
  ['PCS_dcdcIntervalMinFrequency', 'u16'],
  ['PCS_dcdcIntervalMinHvBusVolt', 'u16'],
  ['PCS_dcdcIntervalMinLvBusVolt', 'u16'],
  ['PCS_dcdcIntervalMinLvOutputCurr', 'u16'],
  ['battery_packConfigMultiplexer', 'u16'],
  ['battery_moduleType', 'u16'],
  ['battery_reservedConfig', 'u16'],
  ['BMS_isolationResistance', 'u16'],
  ['BMS_chgPowerAvailable', 'u16'],
  ['BMS_maxRegenPower', 'u16'],
  ['BMS_maxDischargePower', 'u16'],
  ['BMS_maxStationaryHeatPower', 'u16'],
  ['BMS_hvacPowerBudget', 'u16'],
  ['BMS_powerDissipation', 'u16'],
  ['BMS_inletActiveCoolTargetT', 'u16'],
  ['BMS_inletPassiveTargetT', 'u16'],
  ['BMS_inletActiveHeatTargetT', 'u16'],
  ['BMS_packTMin', 'u16'],
  ['BMS_packTMax', 'u16'],
  ['PCS_info_buildConfigId', 'u16'],
  ['PCS_info_hardwareId', 'u16'],
  ['PCS_info_componentId', 'u16'],
  ['PCS_dcdcMaxOutputCurrentAllowed', 'u16'],
  ['PCS_info_usageId', 'u16'],
  ['PCS_info_subUsageId', 'u16'],
  ['HVP_dcLinkVoltage', 'u16'],
  ['HVP_packVoltage', 'u16'],
  ['HVP_fcLinkVoltage', 'u16'],
  ['HVP_packContVoltage', 'u16'],
  ['HVP_packNegativeV', 'u16'],
  ['HVP_packPositiveV', 'u16'],
  ['HVP_pyroAnalog', 'u16'],
  ['HVP_dcLinkNegativeV', 'u16'],
  ['HVP_dcLinkPositiveV', 'u16'],
  ['HVP_fcLinkNegativeV', 'u16'],
  ['HVP_fcContCoilCurrent', 'u16'],
  ['HVP_fcContVoltage', 'u16'],
  ['HVP_hvilInVoltage', 'u16'],
  ['HVP_hvilOutVoltage', 'u16'],
  ['HVP_fcLinkPositiveV', 'u16'],
  ['HVP_packContCoilCurrent', 'u16'],
  ['HVP_battery12V', 'u16'],
  ['HVP_shuntRefVoltageDbg', 'u16'],
  ['HVP_shuntAuxCurrentDbg', 'u16'],
  ['HVP_shuntBarTempDbg', 'u16'],
  ['HVP_shuntAsicTempDbg', 'u16'],
  ['HVP_info_buildConfigId', 'u16'],
  ['HVP_info_hardwareId', 'u16'],
  ['HVP_info_componentId', 'u16'],
  ['HVP_info_usageId', 'u16'],
  ['HVP_info_subUsageId', 'u16'],
  ['hvil_status', 'u8'],
  ['packContNegativeState', 'u8'],
  ['packContPositiveState', 'u8'],
  ['packContactorSetState', 'u8'],
  ['battery_packCtrsRequestStatus', 'u8'],
  ['BMS_info_pcbaId', 'u8'],
  ['BMS_info_assemblyId', 'u8'],
  ['BMS_info_platformType', 'u8'],
  ['BMS_info_bootUdsProtoVersion', 'u8'],
  ['battery_beginning_of_life', 'u8'],
  ['battery_battTempPct', 'u8'],
  ['battery_BrickVoltageMaxNum', 'u8'],
  ['battery_BrickVoltageMinNum', 'u8'],
  ['battery_BrickTempMaxNum', 'u8'],
  ['battery_BrickTempMinNum', 'u8'],
  ['battery_BrickModelTMax', 'u8'],
  ['battery_BrickModelTMin', 'u8'],
  ['BMS_flowRequest', 'u8'],
  ['BMS_uiChargeStatus', 'u8'],
  ['BMS_contactorState', 'u8'],
  ['BMS_state', 'u8'],
  ['BMS_hvState', 'u8'],
  ['BMS_notEnoughPowerForHeatPump', 'u8'],
  ['BMS_powerLimitState', 'u8'],
  ['BMS_inverterTQF', 'u8'],
  ['PCS_dcdcPrechargeStatus', 'u8'],
  ['PCS_dcdc12VSupportStatus', 'u8'],
  ['PCS_dcdcHvBusDischargeStatus', 'u8'],
  ['PCS_dcdcMainState', 'u8'],
  ['PCS_dcdcSubState', 'u8'],
  ['PCS_dcdcPrechargeRtyCnt', 'u8'],
  ['PCS_dcdc12VSupportRtyCnt', 'u8'],
  ['PCS_dcdcDischargeRtyCnt', 'u8'],
  ['PCS_dcdcPwmEnableLine', 'u8'],
  ['PCS_dcdcSupportingFixedLvTarget', 'u8'],
  ['PCS_dcdcPrechargeRestartCnt', 'u8'],
  ['PCS_dcdcInitialPrechargeSubState', 'u8'],
  ['PCS_info_pcbaId', 'u8'],
  ['PCS_info_assemblyId', 'u8'],
  ['PCS_info_platformType', 'u8'],
  ['PCS_info_bootUdsProtoVersion', 'u8'],
  ['HVP_info_platformType', 'u8'],
  ['HVP_info_pcbaId', 'u8'],
  ['HVP_info_assemblyId', 'u8'],
  ['HVP_info_bootUdsProtoVersion', 'u8'],
  ['HVP_shuntHwMia', 'u8'],
  ['HVP_shuntAuxCurrentStatus', 'u8'],
  ['HVP_shuntBarTempStatus', 'u8'],
  ['HVP_shuntAsicTempStatus', 'u8'],
  ['packCtrsClosingBlocked', 'b'],
  ['pyroTestInProgress', 'b'],
  ['battery_packCtrsOpenNowRequested', 'b'],
  ['battery_packCtrsOpenRequested', 'b'],
  ['battery_packCtrsResetRequestRequired', 'b'],
  ['battery_dcLinkAllowedToEnergize', 'b'],
  ['BMS352_mux', 'b'],
  ['battery_full_charge_complete', 'b'],
  ['battery_fully_charged', 'b'],
  ['BMS_hvilFault', 'b'],
  ['BMS_diLimpRequest', 'b'],
  ['BMS_pcsPwmEnabled', 'b'],
  ['BMS_pcsNoFlowRequest', 'b'],
  ['BMS_noFlowRequest', 'b'],
  ['PCS_dcdcFaulted', 'b'],
  ['PCS_dcdcOutputIsLimited', 'b'],
  ['HVP_gpioPassivePyroDepl', 'b'],
  ['HVP_gpioPyroIsoEn', 'b'],
  ['HVP_gpioCpFaultIn', 'b'],
  ['HVP_gpioPackContPowerEn', 'b'],
  ['HVP_gpioHvCablesOk', 'b'],
  ['HVP_gpioHvpSelfEnable', 'b'],
  ['HVP_gpioLed', 'b'],
  ['HVP_gpioCrashSignal', 'b'],
  ['HVP_gpioShuntDataReady', 'b'],
  ['HVP_gpioFcContPosAux', 'b'],
  ['HVP_gpioFcContNegAux', 'b'],
  ['HVP_gpioBmsEout', 'b'],
  ['HVP_gpioCpFaultOut', 'b'],
  ['HVP_gpioPyroPor', 'b'],
  ['HVP_gpioShuntEn', 'b'],
  ['HVP_gpioHvpVerEn', 'b'],
  ['HVP_gpioPackCoontPosFlywheel', 'b'],
  ['HVP_gpioCpLatchEnable', 'b'],
  ['HVP_gpioPcsEnable', 'b'],
  ['HVP_gpioPcsDcdcPwmEnable', 'b'],
  ['HVP_gpioPcsChargePwmEnable', 'b'],
  ['HVP_gpioFcContPowerEnable', 'b'],
  ['HVP_gpioHvilEnable', 'b'],
  ['HVP_gpioSecDrdy', 'b'],
  ['HVP_packCurrentMia', 'b'],
  ['HVP_auxCurrentMia', 'b'],
  ['HVP_currentSenseMia', 'b'],
  ['HVP_shuntRefVoltageMismatch', 'b'],
  ['HVP_shuntThermistorMia', 'b'],
  ['BMS_partNumber', 'u8', 12],
  ['battery_serialNumber', 'u8', 15],
  ['battery_partNumber', 'u8', 12],
  ['PCS_partNumber', 'u8', 13],
  ['HVP_partNumber', 'u8', 13],
  ['', ' ', 3],
];

export const BMS_alertMatrixActive: [number, string, number] = [0, 'b', 100];
export const PCS_alertMatrixActive: [number, string, number] = [100, 'b', 94];
export const CP_alertMatrixActive: [number, string, number] = [194, 'b', 96];
export const BMS_info_bootGitHash: [number, string] = [296, 'u64'];
export const PCS_info_bootGitHash: [number, string] = [304, 'u64'];
export const HVP_info_bootGitHash: [number, string] = [312, 'u64'];
export const HVP_info_bootCrc: [number, string] = [320, 'u32'];
export const HVP_info_appCrc: [number, string] = [324, 'u32'];
export const PCS_info_appCrc: [number, string] = [328, 'u32'];
export const PCS_info_cpu2AppCrc: [number, string] = [332, 'u32'];
export const PCS_info_bootCrc: [number, string] = [336, 'u32'];
export const PCS_dcdc12vSupportLifetimekWh: [number, string] = [340, 'u32'];
export const BMS_info_appCrc: [number, string] = [344, 'u32'];
export const BMS_info_bootCrc: [number, string] = [348, 'u32'];
export const battery_packMass: [number, string] = [352, 'u32'];
export const battery_platformMaxBusVoltage: [number, string] = [356, 'u32'];
export const BMS_min_voltage: [number, string] = [360, 'u32'];
export const BMS_max_voltage: [number, string] = [364, 'u32'];
export const battery_max_charge_current: [number, string] = [368, 'u32'];
export const battery_max_discharge_current: [number, string] = [372, 'u32'];
export const battery_soc_min: [number, string] = [376, 'u32'];
export const battery_soc_max: [number, string] = [380, 'u32'];
export const battery_soc_ave: [number, string] = [384, 'u32'];
export const battery_soc_ui: [number, string] = [388, 'u32'];
export const BMS_info_buildConfigId: [number, string] = [392, 'u16'];
export const BMS_info_hardwareId: [number, string] = [394, 'u16'];
export const BMS_info_componentId: [number, string] = [396, 'u16'];
export const BMS_info_usageId: [number, string] = [398, 'u16'];
export const BMS_info_subUsageId: [number, string] = [400, 'u16'];
export const battery_dcdcLvBusVolt: [number, string] = [402, 'u16'];
export const battery_dcdcHvBusVolt: [number, string] = [404, 'u16'];
export const battery_dcdcLvOutputCurrent: [number, string] = [406, 'u16'];
export const battery_nominal_full_pack_energy: [number, string] = [408, 'u16'];
export const battery_nominal_full_pack_energy_m0: [number, string] = [410, 'u16'];
export const battery_nominal_energy_remaining: [number, string] = [412, 'u16'];
export const battery_nominal_energy_remaining_m0: [number, string] = [414, 'u16'];
export const battery_ideal_energy_remaining: [number, string] = [416, 'u16'];
export const battery_ideal_energy_remaining_m0: [number, string] = [418, 'u16'];
export const battery_energy_to_charge_complete: [number, string] = [420, 'u16'];
export const battery_energy_to_charge_complete_m1: [number, string] = [422, 'u16'];
export const battery_energy_buffer: [number, string] = [424, 'u16'];
export const battery_energy_buffer_m1: [number, string] = [426, 'u16'];
export const battery_expected_energy_remaining: [number, string] = [428, 'u16'];
export const battery_expected_energy_remaining_m1: [number, string] = [430, 'u16'];
export const battery_BrickVoltageMax: [number, string] = [432, 'u16'];
export const battery_BrickVoltageMin: [number, string] = [434, 'u16'];
export const HVP_hvp1v5Ref: [number, string] = [436, 'u16'];
export const HVP_shuntCurrentDebug: [number, string] = [438, 'u16'];
export const PCS_dcdcTemp: [number, string] = [440, 'i16'];
export const PCS_ambientTemp: [number, string] = [442, 'i16'];
export const PCS_chgPhATemp: [number, string] = [444, 'i16'];
export const PCS_chgPhBTemp: [number, string] = [446, 'i16'];
export const PCS_chgPhCTemp: [number, string] = [448, 'i16'];
export const PCS_dcdcMaxLvOutputCurrent: [number, string] = [450, 'u16'];
export const PCS_dcdcCurrentLimit: [number, string] = [452, 'u16'];
export const PCS_dcdcLvOutputCurrentTempLimit: [number, string] = [454, 'u16'];
export const PCS_dcdcUnifiedCommand: [number, string] = [456, 'u16'];
export const PCS_dcdcCLAControllerOutput: [number, string] = [458, 'u16'];
export const PCS_dcdcTankVoltage: [number, string] = [460, 'u16'];
export const PCS_dcdcTankVoltageTarget: [number, string] = [462, 'u16'];
export const PCS_dcdcClaCurrentFreq: [number, string] = [464, 'u16'];
export const PCS_dcdcTCommMeasured: [number, string] = [466, 'u16'];
export const PCS_dcdcShortTimeUs: [number, string] = [468, 'u16'];
export const PCS_dcdcHalfPeriodUs: [number, string] = [470, 'u16'];
export const PCS_dcdcIntervalMaxFrequency: [number, string] = [472, 'u16'];
export const PCS_dcdcIntervalMaxHvBusVolt: [number, string] = [474, 'u16'];
export const PCS_dcdcIntervalMaxLvBusVolt: [number, string] = [476, 'u16'];
export const PCS_dcdcIntervalMaxLvOutputCurr: [number, string] = [478, 'u16'];
export const PCS_dcdcIntervalMinFrequency: [number, string] = [480, 'u16'];
export const PCS_dcdcIntervalMinHvBusVolt: [number, string] = [482, 'u16'];
export const PCS_dcdcIntervalMinLvBusVolt: [number, string] = [484, 'u16'];
export const PCS_dcdcIntervalMinLvOutputCurr: [number, string] = [486, 'u16'];
export const battery_packConfigMultiplexer: [number, string] = [488, 'u16'];
export const battery_moduleType: [number, string] = [490, 'u16'];
export const battery_reservedConfig: [number, string] = [492, 'u16'];
export const BMS_isolationResistance: [number, string] = [494, 'u16'];
export const BMS_chgPowerAvailable: [number, string] = [496, 'u16'];
export const BMS_maxRegenPower: [number, string] = [498, 'u16'];
export const BMS_maxDischargePower: [number, string] = [500, 'u16'];
export const BMS_maxStationaryHeatPower: [number, string] = [502, 'u16'];
export const BMS_hvacPowerBudget: [number, string] = [504, 'u16'];
export const BMS_powerDissipation: [number, string] = [506, 'u16'];
export const BMS_inletActiveCoolTargetT: [number, string] = [508, 'u16'];
export const BMS_inletPassiveTargetT: [number, string] = [510, 'u16'];
export const BMS_inletActiveHeatTargetT: [number, string] = [512, 'u16'];
export const BMS_packTMin: [number, string] = [514, 'u16'];
export const BMS_packTMax: [number, string] = [516, 'u16'];
export const PCS_info_buildConfigId: [number, string] = [518, 'u16'];
export const PCS_info_hardwareId: [number, string] = [520, 'u16'];
export const PCS_info_componentId: [number, string] = [522, 'u16'];
export const PCS_dcdcMaxOutputCurrentAllowed: [number, string] = [524, 'u16'];
export const PCS_info_usageId: [number, string] = [526, 'u16'];
export const PCS_info_subUsageId: [number, string] = [528, 'u16'];
export const HVP_dcLinkVoltage: [number, string] = [530, 'u16'];
export const HVP_packVoltage: [number, string] = [532, 'u16'];
export const HVP_fcLinkVoltage: [number, string] = [534, 'u16'];
export const HVP_packContVoltage: [number, string] = [536, 'u16'];
export const HVP_packNegativeV: [number, string] = [538, 'u16'];
export const HVP_packPositiveV: [number, string] = [540, 'u16'];
export const HVP_pyroAnalog: [number, string] = [542, 'u16'];
export const HVP_dcLinkNegativeV: [number, string] = [544, 'u16'];
export const HVP_dcLinkPositiveV: [number, string] = [546, 'u16'];
export const HVP_fcLinkNegativeV: [number, string] = [548, 'u16'];
export const HVP_fcContCoilCurrent: [number, string] = [550, 'u16'];
export const HVP_fcContVoltage: [number, string] = [552, 'u16'];
export const HVP_hvilInVoltage: [number, string] = [554, 'u16'];
export const HVP_hvilOutVoltage: [number, string] = [556, 'u16'];
export const HVP_fcLinkPositiveV: [number, string] = [558, 'u16'];
export const HVP_packContCoilCurrent: [number, string] = [560, 'u16'];
export const HVP_battery12V: [number, string] = [562, 'u16'];
export const HVP_shuntRefVoltageDbg: [number, string] = [564, 'u16'];
export const HVP_shuntAuxCurrentDbg: [number, string] = [566, 'u16'];
export const HVP_shuntBarTempDbg: [number, string] = [568, 'u16'];
export const HVP_shuntAsicTempDbg: [number, string] = [570, 'u16'];
export const HVP_info_buildConfigId: [number, string] = [572, 'u16'];
export const HVP_info_hardwareId: [number, string] = [574, 'u16'];
export const HVP_info_componentId: [number, string] = [576, 'u16'];
export const HVP_info_usageId: [number, string] = [578, 'u16'];
export const HVP_info_subUsageId: [number, string] = [580, 'u16'];
export const hvil_status: [number, string] = [582, 'u8'];
export const packContNegativeState: [number, string] = [583, 'u8'];
export const packContPositiveState: [number, string] = [584, 'u8'];
export const packContactorSetState: [number, string] = [585, 'u8'];
export const battery_packCtrsRequestStatus: [number, string] = [586, 'u8'];
export const BMS_info_pcbaId: [number, string] = [587, 'u8'];
export const BMS_info_assemblyId: [number, string] = [588, 'u8'];
export const BMS_info_platformType: [number, string] = [589, 'u8'];
export const BMS_info_bootUdsProtoVersion: [number, string] = [590, 'u8'];
export const battery_beginning_of_life: [number, string] = [591, 'u8'];
export const battery_battTempPct: [number, string] = [592, 'u8'];
export const battery_BrickVoltageMaxNum: [number, string] = [593, 'u8'];
export const battery_BrickVoltageMinNum: [number, string] = [594, 'u8'];
export const battery_BrickTempMaxNum: [number, string] = [595, 'u8'];
export const battery_BrickTempMinNum: [number, string] = [596, 'u8'];
export const battery_BrickModelTMax: [number, string] = [597, 'u8'];
export const battery_BrickModelTMin: [number, string] = [598, 'u8'];
export const BMS_flowRequest: [number, string] = [599, 'u8'];
export const BMS_uiChargeStatus: [number, string] = [600, 'u8'];
export const BMS_contactorState: [number, string] = [601, 'u8'];
export const BMS_state: [number, string] = [602, 'u8'];
export const BMS_hvState: [number, string] = [603, 'u8'];
export const BMS_notEnoughPowerForHeatPump: [number, string] = [604, 'u8'];
export const BMS_powerLimitState: [number, string] = [605, 'u8'];
export const BMS_inverterTQF: [number, string] = [606, 'u8'];
export const PCS_dcdcPrechargeStatus: [number, string] = [607, 'u8'];
export const PCS_dcdc12VSupportStatus: [number, string] = [608, 'u8'];
export const PCS_dcdcHvBusDischargeStatus: [number, string] = [609, 'u8'];
export const PCS_dcdcMainState: [number, string] = [610, 'u8'];
export const PCS_dcdcSubState: [number, string] = [611, 'u8'];
export const PCS_dcdcPrechargeRtyCnt: [number, string] = [612, 'u8'];
export const PCS_dcdc12VSupportRtyCnt: [number, string] = [613, 'u8'];
export const PCS_dcdcDischargeRtyCnt: [number, string] = [614, 'u8'];
export const PCS_dcdcPwmEnableLine: [number, string] = [615, 'u8'];
export const PCS_dcdcSupportingFixedLvTarget: [number, string] = [616, 'u8'];
export const PCS_dcdcPrechargeRestartCnt: [number, string] = [617, 'u8'];
export const PCS_dcdcInitialPrechargeSubState: [number, string] = [618, 'u8'];
export const PCS_info_pcbaId: [number, string] = [619, 'u8'];
export const PCS_info_assemblyId: [number, string] = [620, 'u8'];
export const PCS_info_platformType: [number, string] = [621, 'u8'];
export const PCS_info_bootUdsProtoVersion: [number, string] = [622, 'u8'];
export const HVP_info_platformType: [number, string] = [623, 'u8'];
export const HVP_info_pcbaId: [number, string] = [624, 'u8'];
export const HVP_info_assemblyId: [number, string] = [625, 'u8'];
export const HVP_info_bootUdsProtoVersion: [number, string] = [626, 'u8'];
export const HVP_shuntHwMia: [number, string] = [627, 'u8'];
export const HVP_shuntAuxCurrentStatus: [number, string] = [628, 'u8'];
export const HVP_shuntBarTempStatus: [number, string] = [629, 'u8'];
export const HVP_shuntAsicTempStatus: [number, string] = [630, 'u8'];
export const packCtrsClosingBlocked: [number, string] = [631, 'b'];
export const pyroTestInProgress: [number, string] = [632, 'b'];
export const battery_packCtrsOpenNowRequested: [number, string] = [633, 'b'];
export const battery_packCtrsOpenRequested: [number, string] = [634, 'b'];
export const battery_packCtrsResetRequestRequired: [number, string] = [635, 'b'];
export const battery_dcLinkAllowedToEnergize: [number, string] = [636, 'b'];
export const BMS352_mux: [number, string] = [637, 'b'];
export const battery_full_charge_complete: [number, string] = [638, 'b'];
export const battery_fully_charged: [number, string] = [639, 'b'];
export const BMS_hvilFault: [number, string] = [640, 'b'];
export const BMS_diLimpRequest: [number, string] = [641, 'b'];
export const BMS_pcsPwmEnabled: [number, string] = [642, 'b'];
export const BMS_pcsNoFlowRequest: [number, string] = [643, 'b'];
export const BMS_noFlowRequest: [number, string] = [644, 'b'];
export const PCS_dcdcFaulted: [number, string] = [645, 'b'];
export const PCS_dcdcOutputIsLimited: [number, string] = [646, 'b'];
export const HVP_gpioPassivePyroDepl: [number, string] = [647, 'b'];
export const HVP_gpioPyroIsoEn: [number, string] = [648, 'b'];
export const HVP_gpioCpFaultIn: [number, string] = [649, 'b'];
export const HVP_gpioPackContPowerEn: [number, string] = [650, 'b'];
export const HVP_gpioHvCablesOk: [number, string] = [651, 'b'];
export const HVP_gpioHvpSelfEnable: [number, string] = [652, 'b'];
export const HVP_gpioLed: [number, string] = [653, 'b'];
export const HVP_gpioCrashSignal: [number, string] = [654, 'b'];
export const HVP_gpioShuntDataReady: [number, string] = [655, 'b'];
export const HVP_gpioFcContPosAux: [number, string] = [656, 'b'];
export const HVP_gpioFcContNegAux: [number, string] = [657, 'b'];
export const HVP_gpioBmsEout: [number, string] = [658, 'b'];
export const HVP_gpioCpFaultOut: [number, string] = [659, 'b'];
export const HVP_gpioPyroPor: [number, string] = [660, 'b'];
export const HVP_gpioShuntEn: [number, string] = [661, 'b'];
export const HVP_gpioHvpVerEn: [number, string] = [662, 'b'];
export const HVP_gpioPackCoontPosFlywheel: [number, string] = [663, 'b'];
export const HVP_gpioCpLatchEnable: [number, string] = [664, 'b'];
export const HVP_gpioPcsEnable: [number, string] = [665, 'b'];
export const HVP_gpioPcsDcdcPwmEnable: [number, string] = [666, 'b'];
export const HVP_gpioPcsChargePwmEnable: [number, string] = [667, 'b'];
export const HVP_gpioFcContPowerEnable: [number, string] = [668, 'b'];
export const HVP_gpioHvilEnable: [number, string] = [669, 'b'];
export const HVP_gpioSecDrdy: [number, string] = [670, 'b'];
export const HVP_packCurrentMia: [number, string] = [671, 'b'];
export const HVP_auxCurrentMia: [number, string] = [672, 'b'];
export const HVP_currentSenseMia: [number, string] = [673, 'b'];
export const HVP_shuntRefVoltageMismatch: [number, string] = [674, 'b'];
export const HVP_shuntThermistorMia: [number, string] = [675, 'b'];
export const BMS_partNumber: [number, string, number] = [676, 'u8', 12];
export const battery_serialNumber: [number, string, number] = [688, 'u8', 15];
export const battery_partNumber: [number, string, number] = [703, 'u8', 12];
export const PCS_partNumber: [number, string, number] = [715, 'u8', 13];
export const HVP_partNumber: [number, string, number] = [728, 'u8', 13];
