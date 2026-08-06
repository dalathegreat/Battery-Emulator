// TeslaBattery: 2776 bytes; base classes: CanBattery@0
export const TESLA_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'TeslaHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['mux', 'u8'],
  ['', ' '],
  ['temp', 'u16'],
  ['mux0_read', 'b'],
  ['mux1_read', 'b'],
  ['brick_volts', 'u16'],
  ['mux_zero_counter', 'u8'],
  ['mux_max', 'u8'],
  ['', ' ', 2],
  ['transmitPhase', 'i32'],
  ['operate_contactors', 'b'],
  ['', ' ', 3],
  ['allows_contactor_closing', 'bool*'],
  ['previousMillis10', 'u32'],
  ['previousMillis50', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis1000', 'u32'],
  ['alternateMux', 'u8'],
  ['frameCounter_TESLA_221', 'u8'],
  ['vehicleState', 'u8'],
  ['powerDownSeconds', 'u8'],
  ['muxNumber_TESLA_2E1', 'u8'],
  ['TESLA_334_INITIAL_SENT', 'b'],
  ['frameCounter_TESLA_3A1', 'u8'],
  ['timeToMux3A1', 'b'],
  ['TESLA_504_INITIAL_SENT', 'b'],
  ['muxNumber_TESLA_7FF', 'u8'],
  ['previous_max_percentage', 'u16'],
  ['TESLA_118', 'CAN_frame'],
  ['TESLA_118_digital_hvil', 'CAN_frame'],
  ['content_118_digital_hvil', 'u16', 16],
  ['TESLA_2A8', 'CAN_frame'],
  ['TESLA_213', 'CAN_frame'],
  ['TESLA_221_DRIVE_Mux0', 'CAN_frame'],
  ['TESLA_221_DRIVE_Mux1', 'CAN_frame'],
  ['TESLA_221_ACCESSORY_Mux0', 'CAN_frame'],
  ['TESLA_221_ACCESSORY_Mux1', 'CAN_frame'],
  ['TESLA_221_GOING_DOWN_Mux0', 'CAN_frame'],
  ['TESLA_221_GOING_DOWN_Mux1', 'CAN_frame'],
  ['TESLA_221_OFF_Mux0', 'CAN_frame'],
  ['TESLA_221_OFF_Mux1', 'CAN_frame'],
  ['TESLA_229', 'CAN_frame'],
  ['TESLA_2E8', 'CAN_frame'],
  ['TESLA_293', 'CAN_frame'],
  ['TESLA_3A1', 'CAN_frame'],
  ['frame6_3A1', 'u8', 16],
  ['frame7_3A1', 'u8', 16],
  ['TESLA_313', 'CAN_frame'],
  ['TESLA_321', 'CAN_frame'],
  ['TESLA_333', 'CAN_frame'],
  ['TESLA_334', 'CAN_frame'],
  ['TESLA_39D', 'CAN_frame'],
  ['TESLA_7FF_Mux1', 'CAN_frame'],
  ['TESLA_7FF_Mux3', 'CAN_frame'],
  ['TESLA_602', 'CAN_frame'],
  ['TESLA_1CF_digital_hvil', 'CAN_frame'],
  ['content_1CF_digital_hvil', 'u16', 8],
  ['index_1CF', 'u8'],
  ['index_118', 'u8'],
  ['contactor_counter', 'u8'],
  ['stateMachineClearIsolationFault', 'u8'],
  ['stateMachineBMSReset', 'u8'],
  ['stateMachineSOCReset', 'u8'],
  ['stateMachineBMSQuery', 'u8'],
  ['', ' '],
  ['battery_cell_max_v', 'u16'],
  ['battery_cell_min_v', 'u16'],
  ['cellvoltagesRead', 'b'],
  ['', ' ', 3],
  ['battery_total_discharge', 'u32'],
  ['battery_total_charge', 'u32'],
  ['BMS352_mux', 'b'],
  ['', ' '],
  ['battery_energy_buffer', 'u16'],
  ['battery_energy_buffer_m1', 'u16'],
  ['battery_energy_to_charge_complete', 'u16'],
  ['battery_energy_to_charge_complete_m1', 'u16'],
  ['battery_expected_energy_remaining', 'u16'],
  ['battery_expected_energy_remaining_m1', 'u16'],
  ['battery_full_charge_complete', 'b'],
  ['battery_fully_charged', 'b'],
  ['battery_ideal_energy_remaining', 'u16'],
  ['battery_ideal_energy_remaining_m0', 'u16'],
  ['battery_nominal_energy_remaining', 'u16'],
  ['battery_nominal_energy_remaining_m0', 'u16'],
  ['battery_nominal_full_pack_energy', 'u16'],
  ['battery_nominal_full_pack_energy_m0', 'u16'],
  ['battery_volts', 'u16'],
  ['battery_amps', 'i16'],
  ['battery_raw_amps', 'i16'],
  ['battery_charge_time_remaining', 'u16'],
  ['BMS_maxRegenPower', 'u16'],
  ['BMS_maxDischargePower', 'u16'],
  ['BMS_maxStationaryHeatPower', 'u16'],
  ['BMS_hvacPowerBudget', 'u16'],
  ['BMS_notEnoughPowerForHeatPump', 'u8'],
  ['BMS_powerLimitState', 'u8'],
  ['BMS_inverterTQF', 'u8'],
  ['', ' '],
  ['battery_max_discharge_current', 'u16'],
  ['battery_max_charge_current', 'u16'],
  ['BMS_max_voltage', 'u16'],
  ['BMS_min_voltage', 'u16'],
  ['battery_dcdcHvBusVolt', 'u16'],
  ['battery_dcdcLvBusVolt', 'u16'],
  ['battery_dcdcLvOutputCurrent', 'u16'],
  ['battery_beginning_of_life', 'u16'],
  ['battery_soc_min', 'u16'],
  ['battery_soc_max', 'u16'],
  ['battery_soc_ui', 'u16'],
  ['battery_soc_ave', 'u16'],
  ['battery_battTempPct', 'u8'],
  ['', ' ', 3],
  ['battery_packMass', 'u32'],
  ['battery_platformMaxBusVoltage', 'u32'],
  ['battery_packConfigMultiplexer', 'u32'],
  ['battery_moduleType', 'u32'],
  ['battery_reservedConfig', 'u32'],
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
  ['battery_contactor', 'u8'],
  ['battery_hvil_status', 'u8'],
  ['battery_packContNegativeState', 'u8'],
  ['battery_packContPositiveState', 'u8'],
  ['battery_packContactorSetState', 'u8'],
  ['battery_packCtrsClosingBlocked', 'b'],
  ['battery_pyroTestInProgress', 'b'],
  ['battery_packCtrsOpenNowRequested', 'b'],
  ['battery_packCtrsOpenRequested', 'b'],
  ['battery_packCtrsRequestStatus', 'u8'],
  ['battery_packCtrsResetRequestRequired', 'b'],
  ['battery_dcLinkAllowedToEnergize', 'b'],
  ['battery_fcContNegativeAuxOpen', 'b'],
  ['battery_fcContNegativeState', 'u8'],
  ['battery_fcContPositiveAuxOpen', 'b'],
  ['battery_fcContPositiveState', 'u8'],
  ['battery_fcContactorSetState', 'u8'],
  ['battery_fcCtrsClosingAllowed', 'b'],
  ['battery_fcCtrsOpenNowRequested', 'b'],
  ['battery_fcCtrsOpenRequested', 'b'],
  ['battery_fcCtrsRequestStatus', 'u8'],
  ['battery_fcCtrsResetRequestRequired', 'b'],
  ['battery_fcLinkAllowedToEnergize', 'b'],
  ['battery_serialNumber', 'u8', 14],
  ['parsed_battery_serialNumber', 'b'],
  ['', ' ', 4],
  ['battery_partNumber', 'u8', 12],
  ['parsed_battery_partNumber', 'b'],
  ['', ' '],
  ['BMS_info_buildConfigId', 'u16'],
  ['BMS_info_hardwareId', 'u16'],
  ['BMS_info_componentId', 'u16'],
  ['BMS_info_pcbaId', 'u8'],
  ['BMS_info_assemblyId', 'u8'],
  ['BMS_info_usageId', 'u16'],
  ['BMS_info_subUsageId', 'u16'],
  ['BMS_info_platformType', 'u8'],
  ['', ' '],
  ['BMS_info_appCrc', 'u32'],
  ['BMS_info_bootGitHash', 'u64'],
  ['BMS_info_bootUdsProtoVersion', 'u8'],
  ['', ' ', 3],
  ['BMS_info_bootCrc', 'u32'],
  ['BMS_hvacPowerRequest', 'b'],
  ['BMS_notEnoughPowerForDrive', 'b'],
  ['BMS_notEnoughPowerForSupport', 'b'],
  ['BMS_preconditionAllowed', 'b'],
  ['BMS_updateAllowed', 'b'],
  ['BMS_activeHeatingWorthwhile', 'b'],
  ['BMS_cpMiaOnHvs', 'b'],
  ['BMS_contactorState', 'u8'],
  ['BMS_state', 'u8'],
  ['BMS_hvState', 'u8'],
  ['BMS_isolationResistance', 'u16'],
  ['BMS_chargeRequest', 'b'],
  ['BMS_keepWarmRequest', 'b'],
  ['BMS_uiChargeStatus', 'u8'],
  ['BMS_diLimpRequest', 'b'],
  ['BMS_okToShipByAir', 'b'],
  ['BMS_okToShipByLand', 'b'],
  ['', ' ', 2],
  ['BMS_chgPowerAvailable', 'u32'],
  ['BMS_chargeRetryCount', 'u8'],
  ['BMS_pcsPwmEnabled', 'b'],
  ['BMS_ecuLogUploadRequest', 'b'],
  ['BMS_minPackTemperature', 'u8'],
  ['PCS_dcdcPrechargeStatus', 'u8'],
  ['PCS_dcdc12VSupportStatus', 'u8'],
  ['PCS_dcdcHvBusDischargeStatus', 'u8'],
  ['', ' '],
  ['PCS_dcdcMainState', 'u16'],
  ['PCS_dcdcSubState', 'u8'],
  ['PCS_dcdcFaulted', 'b'],
  ['PCS_dcdcOutputIsLimited', 'b'],
  ['', ' ', 3],
  ['PCS_dcdcMaxOutputCurrentAllowed', 'u32'],
  ['PCS_dcdcPrechargeRtyCnt', 'u8'],
  ['PCS_dcdc12VSupportRtyCnt', 'u8'],
  ['PCS_dcdcDischargeRtyCnt', 'u8'],
  ['PCS_dcdcPwmEnableLine', 'u8'],
  ['PCS_dcdcSupportingFixedLvTarget', 'u8'],
  ['PCS_ecuLogUploadRequest', 'u8'],
  ['PCS_dcdcPrechargeRestartCnt', 'u8'],
  ['PCS_dcdcInitialPrechargeSubState', 'u8'],
  ['BMS_powerDissipation', 'u16'],
  ['BMS_flowRequest', 'u16'],
  ['BMS_inletActiveCoolTargetT', 'u16'],
  ['BMS_inletPassiveTargetT', 'u16'],
  ['BMS_inletActiveHeatTargetT', 'u16'],
  ['BMS_packTMin', 'u16'],
  ['BMS_packTMax', 'u16'],
  ['BMS_pcsNoFlowRequest', 'b'],
  ['BMS_noFlowRequest', 'b'],
  ['PCS_partNumber', 'u8', 13],
  ['parsed_PCS_partNumber', 'b'],
  ['PCS_info_buildConfigId', 'u16'],
  ['PCS_info_hardwareId', 'u16'],
  ['PCS_info_componentId', 'u16'],
  ['PCS_info_pcbaId', 'u8'],
  ['PCS_info_assemblyId', 'u8'],
  ['PCS_info_usageId', 'u16'],
  ['PCS_info_subUsageId', 'u16'],
  ['PCS_info_platformType', 'u8'],
  ['', ' '],
  ['PCS_info_appCrc', 'u32'],
  ['PCS_info_cpu2AppCrc', 'u32'],
  ['PCS_info_bootGitHash', 'u64'],
  ['PCS_info_bootUdsProtoVersion', 'u8'],
  ['', ' ', 3],
  ['PCS_info_bootCrc', 'u32'],
  ['PCS_chgPhATemp', 'i16'],
  ['PCS_chgPhBTemp', 'i16'],
  ['PCS_chgPhCTemp', 'i16'],
  ['PCS_dcdcTemp', 'i16'],
  ['PCS_ambientTemp', 'i16'],
  ['PCS_logMessageSelect', 'u16'],
  ['PCS_dcdcMaxLvOutputCurrent', 'u16'],
  ['PCS_dcdcCurrentLimit', 'u16'],
  ['PCS_dcdcLvOutputCurrentTempLimit', 'u16'],
  ['PCS_dcdcUnifiedCommand', 'u16'],
  ['PCS_dcdcCLAControllerOutput', 'u16'],
  ['PCS_dcdcTankVoltage', 'i16'],
  ['PCS_dcdcTankVoltageTarget', 'u16'],
  ['PCS_dcdcClaCurrentFreq', 'u16'],
  ['PCS_dcdcTCommMeasured', 'i16'],
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
  ['', ' ', 2],
  ['PCS_dcdc12vSupportLifetimekWh', 'u32'],
  ['HVP_debugMessageMultiplexer', 'u8'],
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
  ['', ' '],
  ['HVP_hvp1v5Ref', 'u16'],
  ['HVP_shuntCurrentDebug', 'i16'],
  ['HVP_packCurrentMia', 'b'],
  ['HVP_auxCurrentMia', 'b'],
  ['HVP_currentSenseMia', 'b'],
  ['HVP_shuntRefVoltageMismatch', 'b'],
  ['HVP_shuntThermistorMia', 'b'],
  ['HVP_shuntHwMia', 'b'],
  ['HVP_info_buildConfigId', 'u16'],
  ['HVP_info_hardwareId', 'u16'],
  ['HVP_info_componentId', 'u16'],
  ['HVP_info_pcbaId', 'u8'],
  ['HVP_info_assemblyId', 'u8'],
  ['HVP_info_usageId', 'u16'],
  ['HVP_info_subUsageId', 'u16'],
  ['HVP_info_platformType', 'u8'],
  ['', ' ', 3],
  ['HVP_info_appCrc', 'u32'],
  ['HVP_info_bootGitHash', 'u64'],
  ['HVP_info_bootUdsProtoVersion', 'u8'],
  ['', ' ', 3],
  ['HVP_info_bootCrc', 'u32'],
  ['HVP_dcLinkVoltage', 'i16'],
  ['HVP_packVoltage', 'i16'],
  ['HVP_fcLinkVoltage', 'i16'],
  ['HVP_packContVoltage', 'u16'],
  ['HVP_packNegativeV', 'i16'],
  ['HVP_packPositiveV', 'i16'],
  ['HVP_pyroAnalog', 'u16'],
  ['HVP_dcLinkNegativeV', 'i16'],
  ['HVP_dcLinkPositiveV', 'i16'],
  ['HVP_fcLinkNegativeV', 'i16'],
  ['HVP_fcContCoilCurrent', 'u16'],
  ['HVP_fcContVoltage', 'u16'],
  ['HVP_hvilInVoltage', 'u16'],
  ['HVP_hvilOutVoltage', 'u16'],
  ['HVP_fcLinkPositiveV', 'i16'],
  ['HVP_packContCoilCurrent', 'u16'],
  ['HVP_battery12V', 'u16'],
  ['HVP_shuntRefVoltageDbg', 'i16'],
  ['HVP_shuntAuxCurrentDbg', 'i16'],
  ['HVP_shuntBarTempDbg', 'i16'],
  ['HVP_shuntAsicTempDbg', 'i16'],
  ['HVP_shuntAuxCurrentStatus', 'u8'],
  ['HVP_shuntBarTempStatus', 'u8'],
  ['HVP_shuntAsicTempStatus', 'u8'],
  ['BMS_matrixIndex', 'u8'],
  ['BMS_a001_Pack_Config_Mismatch', 'b'],
  ['BMS_a055_SW_HvChain_Model_Fault', 'b'],
  ['BMS_a126_SW_Thermistor_Failure', 'b'],
  ['BMS_a135_HW_BMB_Diagnostics_Failure', 'b'],
  ['BMS_a143_SW_CAC_Change', 'b'],
  ['BMS_a155_SW_Weak_short_impedence', 'b'],
  ['BMS_a173_SW_Charge_Component_Fault', 'b'],
  ['BMS_a178_SW_Uncontrolled_Regen_PwrB', 'b'],
  ['BMS_a061_robinBrickOverVoltage', 'b'],
  ['BMS_a062_SW_BrickV_Imbalance', 'b'],
  ['BMS_a063_SW_ChargePort_Fault', 'b'],
  ['BMS_a064_SW_SOC_Imbalance', 'b'],
  ['BMS_a127_SW_shunt_SNA', 'b'],
  ['BMS_a128_SW_shunt_MIA', 'b'],
  ['BMS_a069_SW_Low_Power', 'b'],
  ['BMS_a130_IO_CAN_Error', 'b'],
  ['BMS_a071_SW_SM_TransCon_Not_Met', 'b'],
  ['BMS_a132_HW_BMB_OTP_Uncorrctbl', 'b'],
  ['BMS_a134_SW_Delayed_Ctr_Off', 'b'],
  ['BMS_a075_SW_Chg_Disable_Failure', 'b'],
  ['BMS_a076_SW_Dch_While_Charging', 'b'],
  ['BMS_a017_SW_Brick_OV', 'b'],
  ['BMS_a018_SW_Brick_UV', 'b'],
  ['BMS_a019_SW_Module_OT', 'b'],
  ['BMS_a021_SW_Dr_Limits_Regulation', 'b'],
  ['BMS_a022_SW_Over_Current', 'b'],
  ['BMS_a023_SW_Stack_OV', 'b'],
  ['BMS_a024_SW_Islanded_Brick', 'b'],
  ['BMS_a025_SW_PwrBalance_Anomaly', 'b'],
  ['BMS_a026_SW_HFCurrent_Anomaly', 'b'],
  ['BMS_a087_SW_Feim_Test_Blocked', 'b'],
  ['BMS_a088_SW_VcFront_MIA_InDrive', 'b'],
  ['BMS_a089_SW_VcFront_MIA', 'b'],
  ['BMS_a090_SW_Gateway_MIA', 'b'],
  ['BMS_a091_SW_ChargePort_MIA', 'b'],
  ['BMS_a092_SW_ChargePort_Mia_On_Hv', 'b'],
  ['BMS_a034_SW_Passive_Isolation', 'b'],
  ['BMS_a035_SW_Isolation', 'b'],
  ['BMS_a036_SW_HvpHvilFault', 'b'],
  ['BMS_a037_SW_Flood_Port_Open', 'b'],
  ['BMS_a158_SW_HVP_HVI_Comms', 'b'],
  ['BMS_a039_SW_DC_Link_Over_Voltage', 'b'],
  ['BMS_a041_SW_Power_On_Reset', 'b'],
  ['BMS_a042_SW_MPU_Error', 'b'],
  ['BMS_a043_SW_Watch_Dog_Reset', 'b'],
  ['BMS_a044_SW_Assertion', 'b'],
  ['BMS_a045_SW_Exception', 'b'],
  ['BMS_a046_SW_Task_Stack_Usage', 'b'],
  ['BMS_a047_SW_Task_Stack_Overflow', 'b'],
  ['BMS_a048_SW_Log_Upload_Request', 'b'],
  ['BMS_a169_SW_FC_Pack_Weld', 'b'],
  ['BMS_a050_SW_Brick_Voltage_MIA', 'b'],
  ['BMS_a051_SW_HVC_Vref_Bad', 'b'],
  ['BMS_a052_SW_PCS_MIA', 'b'],
  ['BMS_a053_SW_ThermalModel_Sanity', 'b'],
  ['BMS_a054_SW_Ver_Supply_Fault', 'b'],
  ['BMS_a176_SW_GracefulPowerOff', 'b'],
  ['BMS_a059_SW_Pack_Voltage_Sensing', 'b'],
  ['BMS_a060_SW_Leakage_Test_Failure', 'b'],
  ['BMS_a077_SW_Charger_Regulation', 'b'],
  ['BMS_a081_SW_Ctr_Close_Blocked', 'b'],
  ['BMS_a082_SW_Ctr_Force_Open', 'b'],
  ['BMS_a083_SW_Ctr_Close_Failure', 'b'],
  ['BMS_a084_SW_Sleep_Wake_Aborted', 'b'],
  ['BMS_a094_SW_Drive_Inverter_MIA', 'b'],
  ['BMS_a099_SW_BMB_Communication', 'b'],
  ['BMS_a105_SW_One_Module_Tsense', 'b'],
  ['BMS_a106_SW_All_Module_Tsense', 'b'],
  ['BMS_a107_SW_Stack_Voltage_MIA', 'b'],
  ['BMS_a121_SW_NVRAM_Config_Error', 'b'],
  ['BMS_a122_SW_BMS_Therm_Irrational', 'b'],
  ['BMS_a123_SW_Internal_Isolation', 'b'],
  ['BMS_a129_SW_VSH_Failure', 'b'],
  ['BMS_a131_Bleed_FET_Failure', 'b'],
  ['BMS_a136_SW_Module_OT_Warning', 'b'],
  ['BMS_a137_SW_Brick_UV_Warning', 'b'],
  ['BMS_a138_SW_Brick_OV_Warning', 'b'],
  ['BMS_a139_SW_DC_Link_V_Irrational', 'b'],
  ['BMS_a141_SW_BMB_Status_Warning', 'b'],
  ['BMS_a144_Hvp_Config_Mismatch', 'b'],
  ['BMS_a145_SW_SOC_Change', 'b'],
  ['BMS_a146_SW_Brick_Overdischarged', 'b'],
  ['BMS_a149_SW_Missing_Config_Block', 'b'],
  ['BMS_a151_SW_external_isolation', 'b'],
  ['BMS_a156_SW_BMB_Vref_bad', 'b'],
  ['BMS_a157_SW_HVP_HVS_Comms', 'b'],
  ['BMS_a159_SW_HVP_ECU_Error', 'b'],
  ['BMS_a161_SW_DI_Open_Request', 'b'],
  ['BMS_a162_SW_No_Power_For_Support', 'b'],
  ['BMS_a163_SW_Contactor_Mismatch', 'b'],
  ['BMS_a164_SW_Uncontrolled_Regen', 'b'],
  ['BMS_a165_SW_Pack_Partial_Weld', 'b'],
  ['BMS_a166_SW_Pack_Full_Weld', 'b'],
  ['BMS_a167_SW_FC_Partial_Weld', 'b'],
  ['BMS_a168_SW_FC_Full_Weld', 'b'],
  ['BMS_a170_SW_Limp_Mode', 'b'],
  ['BMS_a171_SW_Stack_Voltage_Sense', 'b'],
  ['BMS_a174_SW_Charge_Failure', 'b'],
  ['BMS_a179_SW_Hvp_12V_Fault', 'b'],
  ['BMS_a180_SW_ECU_reset_blocked', 'b'],
  ['PCS_a001_chgHwInputOc', 'b'],
  ['PCS_a002_chgHwOutputOc', 'b'],
  ['PCS_a003_chgHwInputOv', 'b'],
  ['PCS_a004_chgHwIntBusOv', 'b'],
  ['PCS_a005_chgOutputOv', 'b'],
  ['PCS_a006_chgPrechargeFailedScr', 'b'],
  ['PCS_a007_chgPhaseTempHot', 'b'],
  ['PCS_a008_chgPhaseOverTemp', 'b'],
  ['PCS_a009_chgPfcCurrentRegulation', 'b'],
  ['PCS_a010_chgIntBusVRegulation', 'b'],
  ['PCS_a011_chgLlcCurrentRegulation', 'b'],
  ['PCS_a012_chgPfcIBandTracerFault', 'b'],
  ['PCS_a013_chgPrechargeFailedBoost', 'b'],
  ['PCS_a014_chgTempRationality', 'b'],
  ['PCS_a015_chg12vUv', 'b'],
  ['PCS_a016_chgAllPhasesFaulted', 'b'],
  ['PCS_a017_chgWallPowerRemoval', 'b'],
  ['PCS_a018_chgUnknownGridConfig', 'b'],
  ['PCS_a019_acChargePowerLimited', 'b'],
  ['PCS_a020_chgEnableLineMismatch', 'b'],
  ['PCS_a021_hvpMia', 'b'],
  ['PCS_a022_bmsMia', 'b'],
  ['PCS_a023_cpMia', 'b'],
  ['PCS_a024_vcfrontMia', 'b'],
  ['PCS_a025_cpu2Malfunction', 'b'],
  ['PCS_a026_watchdogAlarmed', 'b'],
  ['PCS_a027_chgInsufficientCooling', 'b'],
  ['PCS_a028_chgOutputUv', 'b'],
  ['PCS_a029_chgPowerRationality', 'b'],
  ['PCS_a030_canRationality', 'b'],
  ['PCS_a031_uiMia', 'b'],
  ['PCS_a032_gtwMia', 'b'],
  ['PCS_a033_hvBusUv', 'b'],
  ['PCS_a034_hvBusOv', 'b'],
  ['PCS_a035_lvBusUv', 'b'],
  ['PCS_a036_lvBusOv', 'b'],
  ['PCS_a037_resonantTankOc', 'b'],
  ['PCS_a038_claFaulted', 'b'],
  ['PCS_a039_sdModuleClkFault', 'b'],
  ['PCS_a040_dcdcMaxPowerReached', 'b'],
  ['PCS_a041_dcdcOverTemp', 'b'],
  ['PCS_a042_dcdcEnableLineMismatch', 'b'],
  ['PCS_a043_hvBusPrechargeFailure', 'b'],
  ['PCS_a044_12vSupportRegulation', 'b'],
  ['PCS_a045_hvBusLowImpedance', 'b'],
  ['PCS_a046_hvBusHighImpedence', 'b'],
  ['PCS_a047_lvBusLowImpedance', 'b'],
  ['PCS_a048_lvBusHighImpedance', 'b'],
  ['PCS_a049_dcdcTempRationality', 'b'],
  ['PCS_a050_dcdc12VsupportFaulted', 'b'],
  ['PCS_a051_chgIntBusUv', 'b'],
  ['PCS_a052_acVoltageNotPresent', 'b'],
  ['PCS_a053_chgInputVDropHigh', 'b'],
  ['PCS_a054_chgInputVDropTooHigh', 'b'],
  ['PCS_a055_chgLineImedanceHigh', 'b'],
  ['PCS_a056_chgLineImedanceTooHigh', 'b'],
  ['PCS_a057_chgInputOverFreq', 'b'],
  ['PCS_a058_chgInputUnderFreq', 'b'],
  ['PCS_a059_chgInputOvRms', 'b'],
  ['PCS_a060_chgInputOvPeak', 'b'],
  ['PCS_a061_chgVLineRationality', 'b'],
  ['PCS_a062_chgILineRationality', 'b'],
  ['PCS_a063_chgVOutRationality', 'b'],
  ['PCS_a064_chgIOutRationality', 'b'],
  ['PCS_a065_chgPllNotLocked', 'b'],
  ['PCS_a066_dcdcHvRationality', 'b'],
  ['PCS_a067_dcdcLvRationality', 'b'],
  ['PCS_a068_dcdcTankvRationality', 'b'],
  ['PCS_a069_chgPfcLineDidt', 'b'],
  ['PCS_a070_chgPfcLineDvdt', 'b'],
  ['PCS_a071_chgPfcILoopRationality', 'b'],
  ['PCS_a072_cpu2ClaStopped', 'b'],
  ['PCS_a073_unexpectedAcInputVoltage', 'b'],
  ['PCS_a074_hvBusDischargeFailure', 'b'],
  ['PCS_a075_hvBusDischargeTimeout', 'b'],
  ['PCS_a076_dcdcEnDeassertedErr', 'b'],
  ['PCS_a077_microGridEnergyLow', 'b'],
  ['PCS_a078_chgStopDcdcTooHot', 'b'],
  ['PCS_a079_eepromOperationError', 'b'],
  ['PCS_a080_damagedPhaseDetected', 'b'],
  ['PCS_a081_dcdcPchgTimeout', 'b'],
  ['PCS_a082_dcdcPchgUnsafeDiVoltage', 'b'],
  ['PCS_a083_triggerOdin', 'b'],
  ['PCS_a084_dcdcPchgStartVoltages', 'b'],
  ['PCS_a085_dcdcFetsNotSwitching', 'b'],
  ['PCS_a086_dcdcInsufficientCooling', 'b'],
  ['PCS_a087_nvramRecordStatusError', 'b'],
  ['PCS_a088_pchgParameters', 'b'],
  ['PCS_a089_hvBusDischargeIrrational', 'b'],
  ['PCS_a090_expectedAcVoltageSourceMissing', 'b'],
  ['PCS_a091_chgIntBusRationality', 'b'],
  ['PCS_a092_chgPowerLimitedByBusRipple', 'b'],
  ['PCS_a093_powerRailRationality', 'b'],
  ['PCS_a094_pcsDcdcNeedService', 'b'],
  ['CP_a001_canRx', 'b'],
  ['CP_a002_canTx', 'b'],
  ['CP_a003_canError', 'b'],
  ['CP_a004_proximityRationality', 'b'],
  ['CP_a005_gbdcLiveDisconnect', 'b'],
  ['CP_a006_lostCommsBMS', 'b'],
  ['CP_a007_watchdog', 'b'],
  ['CP_a008_memoryError', 'b'],
  ['CP_a009_coverOpen', 'b'],
  ['CP_a010_pilotRationality', 'b'],
  ['CP_a011_eeprom', 'b'],
  ['CP_a012_ledDriver', 'b'],
  ['CP_a013_lostCommsGTW', 'b'],
  ['CP_a014_lostCommsCHG', 'b'],
  ['CP_a015_apsVov', 'b'],
  ['CP_a016_apsVuv', 'b'],
  ['CP_a017_fiveVov', 'b'],
  ['CP_a018_fiveVuv', 'b'],
  ['CP_a019_threeVov', 'b'],
  ['CP_a020_threeVuv', 'b'],
  ['CP_a021_zeroVov', 'b'],
  ['CP_a022_zeroVuv', 'b'],
  ['CP_a023_gbdcSessionFailed', 'b'],
  ['CP_a024_ledsUC', 'b'],
  ['CP_a025_ledsOC', 'b'],
  ['CP_a026_networkManagement', 'b'],
  ['CP_a027_doorSensorOutOfSpec', 'b'],
  ['CP_a028_insertEnableMismatch', 'b'],
  ['CP_a029_doorClosedProxPilot', 'b'],
  ['CP_a030_busOff', 'b'],
  ['CP_a031_doorClosedCommandedOpen', 'b'],
  ['CP_a032_doorOpenExpectedClosed', 'b'],
  ['CP_a033_spiOpen', 'b'],
  ['CP_a034_calibrationIncomplete', 'b'],
  ['CP_a035_latchMovement_1', 'b'],
  ['CP_a036_latchNotDisengaged', 'b'],
  ['CP_a037_latchNotEngaged', 'b'],
  ['CP_a038_latchNotBlocking', 'b'],
  ['CP_a039_latchMovement_2', 'b'],
  ['CP_a040_doNotUse', 'b'],
  ['CP_a041_doorSensorUnplugged', 'b'],
  ['CP_a042_doorAssemblyBroken', 'b'],
  ['CP_a043_doorPotIrrational', 'b'],
  ['CP_a044_lostCommsHVP', 'b'],
  ['CP_a045_lostCommsVCSEC', 'b'],
  ['CP_a046_lostCommsEVSE', 'b'],
  ['CP_a047_lostCommsVCFRONT', 'b'],
  ['CP_a048_lostCommsUI', 'b'],
  ['CP_a049_multipleCablesDetected', 'b'],
  ['CP_a050_latchNotConnected', 'b'],
  ['CP_a051_doorInductiveSensorMIA', 'b'],
  ['CP_a052_evseNotSupported', 'b'],
  ['CP_a053_proxLatchedNoPilot', 'b'],
  ['CP_a054_cableNotSecured', 'b'],
  ['CP_a055_chargeStoppedNoPilot', 'b'],
  ['CP_a056_proxDisconnected', 'b'],
  ['CP_a057_evseFaulted', 'b'],
  ['CP_a058_acChargingBlocked', 'b'],
  ['CP_a059_swcanError', 'b'],
  ['CP_a060_lostCommsPCS', 'b'],
  ['CP_a061_uhfReceiverMIA', 'b'],
  ['CP_a062_scOutOfService', 'b'],
  ['CP_a063_scUpdateInProgress', 'b'],
  ['CP_a064_superchargingBlocked', 'b'],
  ['CP_a065_selfTestFailed', 'b'],
  ['CP_a066_proxLatchedIdlePilot', 'b'],
  ['CP_a067_gbdcConnFault', 'b'],
  ['CP_a068_doorSensorMismatch', 'b'],
  ['CP_a069_doorInductiveSensorError', 'b'],
  ['CP_a070_doorInductiveSensorReset', 'b'],
  ['CP_a071_exiDecodeFailure', 'b'],
  ['CP_a072_v2gEvccTimeout', 'b'],
  ['CP_a073_iecComboShutdown', 'b'],
  ['CP_a074_failedToEstablishV2gComm', 'b'],
  ['CP_a075_v2gCommsFailure', 'b'],
  ['CP_a076_LDC1612errorWatchdog', 'b'],
  ['CP_a077_invalidMacAddress', 'b'],
  ['CP_a078_latchNotDisengagedCold', 'b'],
  ['CP_a079_cableNotSecuredCold', 'b'],
  ['CP_a080_taskStackOverflow', 'b'],
  ['CP_a081_swException', 'b'],
  ['CP_a082_powerOnReset', 'b'],
  ['CP_a083_watchdogTraceData', 'b'],
  ['CP_a084_proximityPeDisconnected', 'b'],
  ['CP_a085_dcPinTempFaulted', 'b'],
  ['CP_a086_dcPinTempIrrational', 'b'],
  ['CP_a087_dcTempModelFault', 'b'],
  ['CP_a088_dcTempModelDeviation', 'b'],
  ['CP_a089_plcConfigMismatch', 'b'],
  ['CP_a090_ccsEvseLowIso', 'b'],
  ['CP_a091_wrongSuperchargerHandle', 'b'],
  ['CP_a092_modemAppLoadFailed', 'b'],
  ['CP_a093_modemLoadedWithReset', 'b'],
  ['CP_a094_inductiveResetSuccessful', 'b'],
  ['CP_a095_thermalDcLimitActive', 'b'],
  ['CP_a096_pilotWake', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'TeslaHtmlRenderer'];
export const datalayer_battery: [number, string] = [28, 'DATALAYER_BATTERY_TYPE*'];
export const mux: [number, string] = [32, 'u8'];
export const temp: [number, string] = [34, 'u16'];
export const mux0_read: [number, string] = [36, 'b'];
export const mux1_read: [number, string] = [37, 'b'];
export const brick_volts: [number, string] = [38, 'u16'];
export const mux_zero_counter: [number, string] = [40, 'u8'];
export const mux_max: [number, string] = [41, 'u8'];
export const transmitPhase: [number, string] = [44, 'i32'];
export const operate_contactors: [number, string] = [48, 'b'];
export const allows_contactor_closing: [number, string] = [52, 'bool*'];
export const previousMillis10: [number, string] = [56, 'u32'];
export const previousMillis50: [number, string] = [60, 'u32'];
export const previousMillis100: [number, string] = [64, 'u32'];
export const previousMillis500: [number, string] = [68, 'u32'];
export const previousMillis1000: [number, string] = [72, 'u32'];
export const alternateMux: [number, string] = [76, 'u8'];
export const frameCounter_TESLA_221: [number, string] = [77, 'u8'];
export const vehicleState: [number, string] = [78, 'u8'];
export const powerDownSeconds: [number, string] = [79, 'u8'];
export const muxNumber_TESLA_2E1: [number, string] = [80, 'u8'];
export const TESLA_334_INITIAL_SENT: [number, string] = [81, 'b'];
export const frameCounter_TESLA_3A1: [number, string] = [82, 'u8'];
export const timeToMux3A1: [number, string] = [83, 'b'];
export const TESLA_504_INITIAL_SENT: [number, string] = [84, 'b'];
export const muxNumber_TESLA_7FF: [number, string] = [85, 'u8'];
export const previous_max_percentage: [number, string] = [86, 'u16'];
export const TESLA_118: [number, string] = [88, 'CAN_frame'];
export const TESLA_118_digital_hvil: [number, string] = [160, 'CAN_frame'];
export const content_118_digital_hvil: [number, string, number] = [232, 'u16', 16];
export const TESLA_2A8: [number, string] = [264, 'CAN_frame'];
export const TESLA_213: [number, string] = [336, 'CAN_frame'];
export const TESLA_221_DRIVE_Mux0: [number, string] = [408, 'CAN_frame'];
export const TESLA_221_DRIVE_Mux1: [number, string] = [480, 'CAN_frame'];
export const TESLA_221_ACCESSORY_Mux0: [number, string] = [552, 'CAN_frame'];
export const TESLA_221_ACCESSORY_Mux1: [number, string] = [624, 'CAN_frame'];
export const TESLA_221_GOING_DOWN_Mux0: [number, string] = [696, 'CAN_frame'];
export const TESLA_221_GOING_DOWN_Mux1: [number, string] = [768, 'CAN_frame'];
export const TESLA_221_OFF_Mux0: [number, string] = [840, 'CAN_frame'];
export const TESLA_221_OFF_Mux1: [number, string] = [912, 'CAN_frame'];
export const TESLA_229: [number, string] = [984, 'CAN_frame'];
export const TESLA_2E8: [number, string] = [1056, 'CAN_frame'];
export const TESLA_293: [number, string] = [1128, 'CAN_frame'];
export const TESLA_3A1: [number, string] = [1200, 'CAN_frame'];
export const frame6_3A1: [number, string, number] = [1272, 'u8', 16];
export const frame7_3A1: [number, string, number] = [1288, 'u8', 16];
export const TESLA_313: [number, string] = [1304, 'CAN_frame'];
export const TESLA_321: [number, string] = [1376, 'CAN_frame'];
export const TESLA_333: [number, string] = [1448, 'CAN_frame'];
export const TESLA_334: [number, string] = [1520, 'CAN_frame'];
export const TESLA_39D: [number, string] = [1592, 'CAN_frame'];
export const TESLA_7FF_Mux1: [number, string] = [1664, 'CAN_frame'];
export const TESLA_7FF_Mux3: [number, string] = [1736, 'CAN_frame'];
export const TESLA_602: [number, string] = [1808, 'CAN_frame'];
export const TESLA_1CF_digital_hvil: [number, string] = [1880, 'CAN_frame'];
export const content_1CF_digital_hvil: [number, string, number] = [1952, 'u16', 8];
export const index_1CF: [number, string] = [1968, 'u8'];
export const index_118: [number, string] = [1969, 'u8'];
export const contactor_counter: [number, string] = [1970, 'u8'];
export const stateMachineClearIsolationFault: [number, string] = [1971, 'u8'];
export const stateMachineBMSReset: [number, string] = [1972, 'u8'];
export const stateMachineSOCReset: [number, string] = [1973, 'u8'];
export const stateMachineBMSQuery: [number, string] = [1974, 'u8'];
export const battery_cell_max_v: [number, string] = [1976, 'u16'];
export const battery_cell_min_v: [number, string] = [1978, 'u16'];
export const cellvoltagesRead: [number, string] = [1980, 'b'];
export const battery_total_discharge: [number, string] = [1984, 'u32'];
export const battery_total_charge: [number, string] = [1988, 'u32'];
export const BMS352_mux: [number, string] = [1992, 'b'];
export const battery_energy_buffer: [number, string] = [1994, 'u16'];
export const battery_energy_buffer_m1: [number, string] = [1996, 'u16'];
export const battery_energy_to_charge_complete: [number, string] = [1998, 'u16'];
export const battery_energy_to_charge_complete_m1: [number, string] = [2000, 'u16'];
export const battery_expected_energy_remaining: [number, string] = [2002, 'u16'];
export const battery_expected_energy_remaining_m1: [number, string] = [2004, 'u16'];
export const battery_full_charge_complete: [number, string] = [2006, 'b'];
export const battery_fully_charged: [number, string] = [2007, 'b'];
export const battery_ideal_energy_remaining: [number, string] = [2008, 'u16'];
export const battery_ideal_energy_remaining_m0: [number, string] = [2010, 'u16'];
export const battery_nominal_energy_remaining: [number, string] = [2012, 'u16'];
export const battery_nominal_energy_remaining_m0: [number, string] = [2014, 'u16'];
export const battery_nominal_full_pack_energy: [number, string] = [2016, 'u16'];
export const battery_nominal_full_pack_energy_m0: [number, string] = [2018, 'u16'];
export const battery_volts: [number, string] = [2020, 'u16'];
export const battery_amps: [number, string] = [2022, 'i16'];
export const battery_raw_amps: [number, string] = [2024, 'i16'];
export const battery_charge_time_remaining: [number, string] = [2026, 'u16'];
export const BMS_maxRegenPower: [number, string] = [2028, 'u16'];
export const BMS_maxDischargePower: [number, string] = [2030, 'u16'];
export const BMS_maxStationaryHeatPower: [number, string] = [2032, 'u16'];
export const BMS_hvacPowerBudget: [number, string] = [2034, 'u16'];
export const BMS_notEnoughPowerForHeatPump: [number, string] = [2036, 'u8'];
export const BMS_powerLimitState: [number, string] = [2037, 'u8'];
export const BMS_inverterTQF: [number, string] = [2038, 'u8'];
export const battery_max_discharge_current: [number, string] = [2040, 'u16'];
export const battery_max_charge_current: [number, string] = [2042, 'u16'];
export const BMS_max_voltage: [number, string] = [2044, 'u16'];
export const BMS_min_voltage: [number, string] = [2046, 'u16'];
export const battery_dcdcHvBusVolt: [number, string] = [2048, 'u16'];
export const battery_dcdcLvBusVolt: [number, string] = [2050, 'u16'];
export const battery_dcdcLvOutputCurrent: [number, string] = [2052, 'u16'];
export const battery_beginning_of_life: [number, string] = [2054, 'u16'];
export const battery_soc_min: [number, string] = [2056, 'u16'];
export const battery_soc_max: [number, string] = [2058, 'u16'];
export const battery_soc_ui: [number, string] = [2060, 'u16'];
export const battery_soc_ave: [number, string] = [2062, 'u16'];
export const battery_battTempPct: [number, string] = [2064, 'u8'];
export const battery_packMass: [number, string] = [2068, 'u32'];
export const battery_platformMaxBusVoltage: [number, string] = [2072, 'u32'];
export const battery_packConfigMultiplexer: [number, string] = [2076, 'u32'];
export const battery_moduleType: [number, string] = [2080, 'u32'];
export const battery_reservedConfig: [number, string] = [2084, 'u32'];
export const battery_max_temp: [number, string] = [2088, 'i16'];
export const battery_min_temp: [number, string] = [2090, 'i16'];
export const battery_BrickVoltageMax: [number, string] = [2092, 'u16'];
export const battery_BrickVoltageMin: [number, string] = [2094, 'u16'];
export const battery_BrickTempMaxNum: [number, string] = [2096, 'u8'];
export const battery_BrickTempMinNum: [number, string] = [2097, 'u8'];
export const battery_BrickModelTMax: [number, string] = [2098, 'u8'];
export const battery_BrickModelTMin: [number, string] = [2099, 'u8'];
export const battery_BrickVoltageMaxNum: [number, string] = [2100, 'u8'];
export const battery_BrickVoltageMinNum: [number, string] = [2101, 'u8'];
export const battery_contactor: [number, string] = [2102, 'u8'];
export const battery_hvil_status: [number, string] = [2103, 'u8'];
export const battery_packContNegativeState: [number, string] = [2104, 'u8'];
export const battery_packContPositiveState: [number, string] = [2105, 'u8'];
export const battery_packContactorSetState: [number, string] = [2106, 'u8'];
export const battery_packCtrsClosingBlocked: [number, string] = [2107, 'b'];
export const battery_pyroTestInProgress: [number, string] = [2108, 'b'];
export const battery_packCtrsOpenNowRequested: [number, string] = [2109, 'b'];
export const battery_packCtrsOpenRequested: [number, string] = [2110, 'b'];
export const battery_packCtrsRequestStatus: [number, string] = [2111, 'u8'];
export const battery_packCtrsResetRequestRequired: [number, string] = [2112, 'b'];
export const battery_dcLinkAllowedToEnergize: [number, string] = [2113, 'b'];
export const battery_fcContNegativeAuxOpen: [number, string] = [2114, 'b'];
export const battery_fcContNegativeState: [number, string] = [2115, 'u8'];
export const battery_fcContPositiveAuxOpen: [number, string] = [2116, 'b'];
export const battery_fcContPositiveState: [number, string] = [2117, 'u8'];
export const battery_fcContactorSetState: [number, string] = [2118, 'u8'];
export const battery_fcCtrsClosingAllowed: [number, string] = [2119, 'b'];
export const battery_fcCtrsOpenNowRequested: [number, string] = [2120, 'b'];
export const battery_fcCtrsOpenRequested: [number, string] = [2121, 'b'];
export const battery_fcCtrsRequestStatus: [number, string] = [2122, 'u8'];
export const battery_fcCtrsResetRequestRequired: [number, string] = [2123, 'b'];
export const battery_fcLinkAllowedToEnergize: [number, string] = [2124, 'b'];
export const battery_serialNumber: [number, string, number] = [2125, 'u8', 14];
export const parsed_battery_serialNumber: [number, string] = [2139, 'b'];
export const battery_partNumber: [number, string, number] = [2144, 'u8', 12];
export const parsed_battery_partNumber: [number, string] = [2156, 'b'];
export const BMS_info_buildConfigId: [number, string] = [2158, 'u16'];
export const BMS_info_hardwareId: [number, string] = [2160, 'u16'];
export const BMS_info_componentId: [number, string] = [2162, 'u16'];
export const BMS_info_pcbaId: [number, string] = [2164, 'u8'];
export const BMS_info_assemblyId: [number, string] = [2165, 'u8'];
export const BMS_info_usageId: [number, string] = [2166, 'u16'];
export const BMS_info_subUsageId: [number, string] = [2168, 'u16'];
export const BMS_info_platformType: [number, string] = [2170, 'u8'];
export const BMS_info_appCrc: [number, string] = [2172, 'u32'];
export const BMS_info_bootGitHash: [number, string] = [2176, 'u64'];
export const BMS_info_bootUdsProtoVersion: [number, string] = [2184, 'u8'];
export const BMS_info_bootCrc: [number, string] = [2188, 'u32'];
export const BMS_hvacPowerRequest: [number, string] = [2192, 'b'];
export const BMS_notEnoughPowerForDrive: [number, string] = [2193, 'b'];
export const BMS_notEnoughPowerForSupport: [number, string] = [2194, 'b'];
export const BMS_preconditionAllowed: [number, string] = [2195, 'b'];
export const BMS_updateAllowed: [number, string] = [2196, 'b'];
export const BMS_activeHeatingWorthwhile: [number, string] = [2197, 'b'];
export const BMS_cpMiaOnHvs: [number, string] = [2198, 'b'];
export const BMS_contactorState: [number, string] = [2199, 'u8'];
export const BMS_state: [number, string] = [2200, 'u8'];
export const BMS_hvState: [number, string] = [2201, 'u8'];
export const BMS_isolationResistance: [number, string] = [2202, 'u16'];
export const BMS_chargeRequest: [number, string] = [2204, 'b'];
export const BMS_keepWarmRequest: [number, string] = [2205, 'b'];
export const BMS_uiChargeStatus: [number, string] = [2206, 'u8'];
export const BMS_diLimpRequest: [number, string] = [2207, 'b'];
export const BMS_okToShipByAir: [number, string] = [2208, 'b'];
export const BMS_okToShipByLand: [number, string] = [2209, 'b'];
export const BMS_chgPowerAvailable: [number, string] = [2212, 'u32'];
export const BMS_chargeRetryCount: [number, string] = [2216, 'u8'];
export const BMS_pcsPwmEnabled: [number, string] = [2217, 'b'];
export const BMS_ecuLogUploadRequest: [number, string] = [2218, 'b'];
export const BMS_minPackTemperature: [number, string] = [2219, 'u8'];
export const PCS_dcdcPrechargeStatus: [number, string] = [2220, 'u8'];
export const PCS_dcdc12VSupportStatus: [number, string] = [2221, 'u8'];
export const PCS_dcdcHvBusDischargeStatus: [number, string] = [2222, 'u8'];
export const PCS_dcdcMainState: [number, string] = [2224, 'u16'];
export const PCS_dcdcSubState: [number, string] = [2226, 'u8'];
export const PCS_dcdcFaulted: [number, string] = [2227, 'b'];
export const PCS_dcdcOutputIsLimited: [number, string] = [2228, 'b'];
export const PCS_dcdcMaxOutputCurrentAllowed: [number, string] = [2232, 'u32'];
export const PCS_dcdcPrechargeRtyCnt: [number, string] = [2236, 'u8'];
export const PCS_dcdc12VSupportRtyCnt: [number, string] = [2237, 'u8'];
export const PCS_dcdcDischargeRtyCnt: [number, string] = [2238, 'u8'];
export const PCS_dcdcPwmEnableLine: [number, string] = [2239, 'u8'];
export const PCS_dcdcSupportingFixedLvTarget: [number, string] = [2240, 'u8'];
export const PCS_ecuLogUploadRequest: [number, string] = [2241, 'u8'];
export const PCS_dcdcPrechargeRestartCnt: [number, string] = [2242, 'u8'];
export const PCS_dcdcInitialPrechargeSubState: [number, string] = [2243, 'u8'];
export const BMS_powerDissipation: [number, string] = [2244, 'u16'];
export const BMS_flowRequest: [number, string] = [2246, 'u16'];
export const BMS_inletActiveCoolTargetT: [number, string] = [2248, 'u16'];
export const BMS_inletPassiveTargetT: [number, string] = [2250, 'u16'];
export const BMS_inletActiveHeatTargetT: [number, string] = [2252, 'u16'];
export const BMS_packTMin: [number, string] = [2254, 'u16'];
export const BMS_packTMax: [number, string] = [2256, 'u16'];
export const BMS_pcsNoFlowRequest: [number, string] = [2258, 'b'];
export const BMS_noFlowRequest: [number, string] = [2259, 'b'];
export const PCS_partNumber: [number, string, number] = [2260, 'u8', 13];
export const parsed_PCS_partNumber: [number, string] = [2273, 'b'];
export const PCS_info_buildConfigId: [number, string] = [2274, 'u16'];
export const PCS_info_hardwareId: [number, string] = [2276, 'u16'];
export const PCS_info_componentId: [number, string] = [2278, 'u16'];
export const PCS_info_pcbaId: [number, string] = [2280, 'u8'];
export const PCS_info_assemblyId: [number, string] = [2281, 'u8'];
export const PCS_info_usageId: [number, string] = [2282, 'u16'];
export const PCS_info_subUsageId: [number, string] = [2284, 'u16'];
export const PCS_info_platformType: [number, string] = [2286, 'u8'];
export const PCS_info_appCrc: [number, string] = [2288, 'u32'];
export const PCS_info_cpu2AppCrc: [number, string] = [2292, 'u32'];
export const PCS_info_bootGitHash: [number, string] = [2296, 'u64'];
export const PCS_info_bootUdsProtoVersion: [number, string] = [2304, 'u8'];
export const PCS_info_bootCrc: [number, string] = [2308, 'u32'];
export const PCS_chgPhATemp: [number, string] = [2312, 'i16'];
export const PCS_chgPhBTemp: [number, string] = [2314, 'i16'];
export const PCS_chgPhCTemp: [number, string] = [2316, 'i16'];
export const PCS_dcdcTemp: [number, string] = [2318, 'i16'];
export const PCS_ambientTemp: [number, string] = [2320, 'i16'];
export const PCS_logMessageSelect: [number, string] = [2322, 'u16'];
export const PCS_dcdcMaxLvOutputCurrent: [number, string] = [2324, 'u16'];
export const PCS_dcdcCurrentLimit: [number, string] = [2326, 'u16'];
export const PCS_dcdcLvOutputCurrentTempLimit: [number, string] = [2328, 'u16'];
export const PCS_dcdcUnifiedCommand: [number, string] = [2330, 'u16'];
export const PCS_dcdcCLAControllerOutput: [number, string] = [2332, 'u16'];
export const PCS_dcdcTankVoltage: [number, string] = [2334, 'i16'];
export const PCS_dcdcTankVoltageTarget: [number, string] = [2336, 'u16'];
export const PCS_dcdcClaCurrentFreq: [number, string] = [2338, 'u16'];
export const PCS_dcdcTCommMeasured: [number, string] = [2340, 'i16'];
export const PCS_dcdcShortTimeUs: [number, string] = [2342, 'u16'];
export const PCS_dcdcHalfPeriodUs: [number, string] = [2344, 'u16'];
export const PCS_dcdcIntervalMaxFrequency: [number, string] = [2346, 'u16'];
export const PCS_dcdcIntervalMaxHvBusVolt: [number, string] = [2348, 'u16'];
export const PCS_dcdcIntervalMaxLvBusVolt: [number, string] = [2350, 'u16'];
export const PCS_dcdcIntervalMaxLvOutputCurr: [number, string] = [2352, 'u16'];
export const PCS_dcdcIntervalMinFrequency: [number, string] = [2354, 'u16'];
export const PCS_dcdcIntervalMinHvBusVolt: [number, string] = [2356, 'u16'];
export const PCS_dcdcIntervalMinLvBusVolt: [number, string] = [2358, 'u16'];
export const PCS_dcdcIntervalMinLvOutputCurr: [number, string] = [2360, 'u16'];
export const PCS_dcdc12vSupportLifetimekWh: [number, string] = [2364, 'u32'];
export const HVP_debugMessageMultiplexer: [number, string] = [2368, 'u8'];
export const HVP_gpioPassivePyroDepl: [number, string] = [2369, 'b'];
export const HVP_gpioPyroIsoEn: [number, string] = [2370, 'b'];
export const HVP_gpioCpFaultIn: [number, string] = [2371, 'b'];
export const HVP_gpioPackContPowerEn: [number, string] = [2372, 'b'];
export const HVP_gpioHvCablesOk: [number, string] = [2373, 'b'];
export const HVP_gpioHvpSelfEnable: [number, string] = [2374, 'b'];
export const HVP_gpioLed: [number, string] = [2375, 'b'];
export const HVP_gpioCrashSignal: [number, string] = [2376, 'b'];
export const HVP_gpioShuntDataReady: [number, string] = [2377, 'b'];
export const HVP_gpioFcContPosAux: [number, string] = [2378, 'b'];
export const HVP_gpioFcContNegAux: [number, string] = [2379, 'b'];
export const HVP_gpioBmsEout: [number, string] = [2380, 'b'];
export const HVP_gpioCpFaultOut: [number, string] = [2381, 'b'];
export const HVP_gpioPyroPor: [number, string] = [2382, 'b'];
export const HVP_gpioShuntEn: [number, string] = [2383, 'b'];
export const HVP_gpioHvpVerEn: [number, string] = [2384, 'b'];
export const HVP_gpioPackCoontPosFlywheel: [number, string] = [2385, 'b'];
export const HVP_gpioCpLatchEnable: [number, string] = [2386, 'b'];
export const HVP_gpioPcsEnable: [number, string] = [2387, 'b'];
export const HVP_gpioPcsDcdcPwmEnable: [number, string] = [2388, 'b'];
export const HVP_gpioPcsChargePwmEnable: [number, string] = [2389, 'b'];
export const HVP_gpioFcContPowerEnable: [number, string] = [2390, 'b'];
export const HVP_gpioHvilEnable: [number, string] = [2391, 'b'];
export const HVP_gpioSecDrdy: [number, string] = [2392, 'b'];
export const HVP_hvp1v5Ref: [number, string] = [2394, 'u16'];
export const HVP_shuntCurrentDebug: [number, string] = [2396, 'i16'];
export const HVP_packCurrentMia: [number, string] = [2398, 'b'];
export const HVP_auxCurrentMia: [number, string] = [2399, 'b'];
export const HVP_currentSenseMia: [number, string] = [2400, 'b'];
export const HVP_shuntRefVoltageMismatch: [number, string] = [2401, 'b'];
export const HVP_shuntThermistorMia: [number, string] = [2402, 'b'];
export const HVP_shuntHwMia: [number, string] = [2403, 'b'];
export const HVP_info_buildConfigId: [number, string] = [2404, 'u16'];
export const HVP_info_hardwareId: [number, string] = [2406, 'u16'];
export const HVP_info_componentId: [number, string] = [2408, 'u16'];
export const HVP_info_pcbaId: [number, string] = [2410, 'u8'];
export const HVP_info_assemblyId: [number, string] = [2411, 'u8'];
export const HVP_info_usageId: [number, string] = [2412, 'u16'];
export const HVP_info_subUsageId: [number, string] = [2414, 'u16'];
export const HVP_info_platformType: [number, string] = [2416, 'u8'];
export const HVP_info_appCrc: [number, string] = [2420, 'u32'];
export const HVP_info_bootGitHash: [number, string] = [2424, 'u64'];
export const HVP_info_bootUdsProtoVersion: [number, string] = [2432, 'u8'];
export const HVP_info_bootCrc: [number, string] = [2436, 'u32'];
export const HVP_dcLinkVoltage: [number, string] = [2440, 'i16'];
export const HVP_packVoltage: [number, string] = [2442, 'i16'];
export const HVP_fcLinkVoltage: [number, string] = [2444, 'i16'];
export const HVP_packContVoltage: [number, string] = [2446, 'u16'];
export const HVP_packNegativeV: [number, string] = [2448, 'i16'];
export const HVP_packPositiveV: [number, string] = [2450, 'i16'];
export const HVP_pyroAnalog: [number, string] = [2452, 'u16'];
export const HVP_dcLinkNegativeV: [number, string] = [2454, 'i16'];
export const HVP_dcLinkPositiveV: [number, string] = [2456, 'i16'];
export const HVP_fcLinkNegativeV: [number, string] = [2458, 'i16'];
export const HVP_fcContCoilCurrent: [number, string] = [2460, 'u16'];
export const HVP_fcContVoltage: [number, string] = [2462, 'u16'];
export const HVP_hvilInVoltage: [number, string] = [2464, 'u16'];
export const HVP_hvilOutVoltage: [number, string] = [2466, 'u16'];
export const HVP_fcLinkPositiveV: [number, string] = [2468, 'i16'];
export const HVP_packContCoilCurrent: [number, string] = [2470, 'u16'];
export const HVP_battery12V: [number, string] = [2472, 'u16'];
export const HVP_shuntRefVoltageDbg: [number, string] = [2474, 'i16'];
export const HVP_shuntAuxCurrentDbg: [number, string] = [2476, 'i16'];
export const HVP_shuntBarTempDbg: [number, string] = [2478, 'i16'];
export const HVP_shuntAsicTempDbg: [number, string] = [2480, 'i16'];
export const HVP_shuntAuxCurrentStatus: [number, string] = [2482, 'u8'];
export const HVP_shuntBarTempStatus: [number, string] = [2483, 'u8'];
export const HVP_shuntAsicTempStatus: [number, string] = [2484, 'u8'];
export const BMS_matrixIndex: [number, string] = [2485, 'u8'];
export const BMS_a001_Pack_Config_Mismatch: [number, string] = [2486, 'b'];
export const BMS_a055_SW_HvChain_Model_Fault: [number, string] = [2487, 'b'];
export const BMS_a126_SW_Thermistor_Failure: [number, string] = [2488, 'b'];
export const BMS_a135_HW_BMB_Diagnostics_Failure: [number, string] = [2489, 'b'];
export const BMS_a143_SW_CAC_Change: [number, string] = [2490, 'b'];
export const BMS_a155_SW_Weak_short_impedence: [number, string] = [2491, 'b'];
export const BMS_a173_SW_Charge_Component_Fault: [number, string] = [2492, 'b'];
export const BMS_a178_SW_Uncontrolled_Regen_PwrB: [number, string] = [2493, 'b'];
export const BMS_a061_robinBrickOverVoltage: [number, string] = [2494, 'b'];
export const BMS_a062_SW_BrickV_Imbalance: [number, string] = [2495, 'b'];
export const BMS_a063_SW_ChargePort_Fault: [number, string] = [2496, 'b'];
export const BMS_a064_SW_SOC_Imbalance: [number, string] = [2497, 'b'];
export const BMS_a127_SW_shunt_SNA: [number, string] = [2498, 'b'];
export const BMS_a128_SW_shunt_MIA: [number, string] = [2499, 'b'];
export const BMS_a069_SW_Low_Power: [number, string] = [2500, 'b'];
export const BMS_a130_IO_CAN_Error: [number, string] = [2501, 'b'];
export const BMS_a071_SW_SM_TransCon_Not_Met: [number, string] = [2502, 'b'];
export const BMS_a132_HW_BMB_OTP_Uncorrctbl: [number, string] = [2503, 'b'];
export const BMS_a134_SW_Delayed_Ctr_Off: [number, string] = [2504, 'b'];
export const BMS_a075_SW_Chg_Disable_Failure: [number, string] = [2505, 'b'];
export const BMS_a076_SW_Dch_While_Charging: [number, string] = [2506, 'b'];
export const BMS_a017_SW_Brick_OV: [number, string] = [2507, 'b'];
export const BMS_a018_SW_Brick_UV: [number, string] = [2508, 'b'];
export const BMS_a019_SW_Module_OT: [number, string] = [2509, 'b'];
export const BMS_a021_SW_Dr_Limits_Regulation: [number, string] = [2510, 'b'];
export const BMS_a022_SW_Over_Current: [number, string] = [2511, 'b'];
export const BMS_a023_SW_Stack_OV: [number, string] = [2512, 'b'];
export const BMS_a024_SW_Islanded_Brick: [number, string] = [2513, 'b'];
export const BMS_a025_SW_PwrBalance_Anomaly: [number, string] = [2514, 'b'];
export const BMS_a026_SW_HFCurrent_Anomaly: [number, string] = [2515, 'b'];
export const BMS_a087_SW_Feim_Test_Blocked: [number, string] = [2516, 'b'];
export const BMS_a088_SW_VcFront_MIA_InDrive: [number, string] = [2517, 'b'];
export const BMS_a089_SW_VcFront_MIA: [number, string] = [2518, 'b'];
export const BMS_a090_SW_Gateway_MIA: [number, string] = [2519, 'b'];
export const BMS_a091_SW_ChargePort_MIA: [number, string] = [2520, 'b'];
export const BMS_a092_SW_ChargePort_Mia_On_Hv: [number, string] = [2521, 'b'];
export const BMS_a034_SW_Passive_Isolation: [number, string] = [2522, 'b'];
export const BMS_a035_SW_Isolation: [number, string] = [2523, 'b'];
export const BMS_a036_SW_HvpHvilFault: [number, string] = [2524, 'b'];
export const BMS_a037_SW_Flood_Port_Open: [number, string] = [2525, 'b'];
export const BMS_a158_SW_HVP_HVI_Comms: [number, string] = [2526, 'b'];
export const BMS_a039_SW_DC_Link_Over_Voltage: [number, string] = [2527, 'b'];
export const BMS_a041_SW_Power_On_Reset: [number, string] = [2528, 'b'];
export const BMS_a042_SW_MPU_Error: [number, string] = [2529, 'b'];
export const BMS_a043_SW_Watch_Dog_Reset: [number, string] = [2530, 'b'];
export const BMS_a044_SW_Assertion: [number, string] = [2531, 'b'];
export const BMS_a045_SW_Exception: [number, string] = [2532, 'b'];
export const BMS_a046_SW_Task_Stack_Usage: [number, string] = [2533, 'b'];
export const BMS_a047_SW_Task_Stack_Overflow: [number, string] = [2534, 'b'];
export const BMS_a048_SW_Log_Upload_Request: [number, string] = [2535, 'b'];
export const BMS_a169_SW_FC_Pack_Weld: [number, string] = [2536, 'b'];
export const BMS_a050_SW_Brick_Voltage_MIA: [number, string] = [2537, 'b'];
export const BMS_a051_SW_HVC_Vref_Bad: [number, string] = [2538, 'b'];
export const BMS_a052_SW_PCS_MIA: [number, string] = [2539, 'b'];
export const BMS_a053_SW_ThermalModel_Sanity: [number, string] = [2540, 'b'];
export const BMS_a054_SW_Ver_Supply_Fault: [number, string] = [2541, 'b'];
export const BMS_a176_SW_GracefulPowerOff: [number, string] = [2542, 'b'];
export const BMS_a059_SW_Pack_Voltage_Sensing: [number, string] = [2543, 'b'];
export const BMS_a060_SW_Leakage_Test_Failure: [number, string] = [2544, 'b'];
export const BMS_a077_SW_Charger_Regulation: [number, string] = [2545, 'b'];
export const BMS_a081_SW_Ctr_Close_Blocked: [number, string] = [2546, 'b'];
export const BMS_a082_SW_Ctr_Force_Open: [number, string] = [2547, 'b'];
export const BMS_a083_SW_Ctr_Close_Failure: [number, string] = [2548, 'b'];
export const BMS_a084_SW_Sleep_Wake_Aborted: [number, string] = [2549, 'b'];
export const BMS_a094_SW_Drive_Inverter_MIA: [number, string] = [2550, 'b'];
export const BMS_a099_SW_BMB_Communication: [number, string] = [2551, 'b'];
export const BMS_a105_SW_One_Module_Tsense: [number, string] = [2552, 'b'];
export const BMS_a106_SW_All_Module_Tsense: [number, string] = [2553, 'b'];
export const BMS_a107_SW_Stack_Voltage_MIA: [number, string] = [2554, 'b'];
export const BMS_a121_SW_NVRAM_Config_Error: [number, string] = [2555, 'b'];
export const BMS_a122_SW_BMS_Therm_Irrational: [number, string] = [2556, 'b'];
export const BMS_a123_SW_Internal_Isolation: [number, string] = [2557, 'b'];
export const BMS_a129_SW_VSH_Failure: [number, string] = [2558, 'b'];
export const BMS_a131_Bleed_FET_Failure: [number, string] = [2559, 'b'];
export const BMS_a136_SW_Module_OT_Warning: [number, string] = [2560, 'b'];
export const BMS_a137_SW_Brick_UV_Warning: [number, string] = [2561, 'b'];
export const BMS_a138_SW_Brick_OV_Warning: [number, string] = [2562, 'b'];
export const BMS_a139_SW_DC_Link_V_Irrational: [number, string] = [2563, 'b'];
export const BMS_a141_SW_BMB_Status_Warning: [number, string] = [2564, 'b'];
export const BMS_a144_Hvp_Config_Mismatch: [number, string] = [2565, 'b'];
export const BMS_a145_SW_SOC_Change: [number, string] = [2566, 'b'];
export const BMS_a146_SW_Brick_Overdischarged: [number, string] = [2567, 'b'];
export const BMS_a149_SW_Missing_Config_Block: [number, string] = [2568, 'b'];
export const BMS_a151_SW_external_isolation: [number, string] = [2569, 'b'];
export const BMS_a156_SW_BMB_Vref_bad: [number, string] = [2570, 'b'];
export const BMS_a157_SW_HVP_HVS_Comms: [number, string] = [2571, 'b'];
export const BMS_a159_SW_HVP_ECU_Error: [number, string] = [2572, 'b'];
export const BMS_a161_SW_DI_Open_Request: [number, string] = [2573, 'b'];
export const BMS_a162_SW_No_Power_For_Support: [number, string] = [2574, 'b'];
export const BMS_a163_SW_Contactor_Mismatch: [number, string] = [2575, 'b'];
export const BMS_a164_SW_Uncontrolled_Regen: [number, string] = [2576, 'b'];
export const BMS_a165_SW_Pack_Partial_Weld: [number, string] = [2577, 'b'];
export const BMS_a166_SW_Pack_Full_Weld: [number, string] = [2578, 'b'];
export const BMS_a167_SW_FC_Partial_Weld: [number, string] = [2579, 'b'];
export const BMS_a168_SW_FC_Full_Weld: [number, string] = [2580, 'b'];
export const BMS_a170_SW_Limp_Mode: [number, string] = [2581, 'b'];
export const BMS_a171_SW_Stack_Voltage_Sense: [number, string] = [2582, 'b'];
export const BMS_a174_SW_Charge_Failure: [number, string] = [2583, 'b'];
export const BMS_a179_SW_Hvp_12V_Fault: [number, string] = [2584, 'b'];
export const BMS_a180_SW_ECU_reset_blocked: [number, string] = [2585, 'b'];
export const PCS_a001_chgHwInputOc: [number, string] = [2586, 'b'];
export const PCS_a002_chgHwOutputOc: [number, string] = [2587, 'b'];
export const PCS_a003_chgHwInputOv: [number, string] = [2588, 'b'];
export const PCS_a004_chgHwIntBusOv: [number, string] = [2589, 'b'];
export const PCS_a005_chgOutputOv: [number, string] = [2590, 'b'];
export const PCS_a006_chgPrechargeFailedScr: [number, string] = [2591, 'b'];
export const PCS_a007_chgPhaseTempHot: [number, string] = [2592, 'b'];
export const PCS_a008_chgPhaseOverTemp: [number, string] = [2593, 'b'];
export const PCS_a009_chgPfcCurrentRegulation: [number, string] = [2594, 'b'];
export const PCS_a010_chgIntBusVRegulation: [number, string] = [2595, 'b'];
export const PCS_a011_chgLlcCurrentRegulation: [number, string] = [2596, 'b'];
export const PCS_a012_chgPfcIBandTracerFault: [number, string] = [2597, 'b'];
export const PCS_a013_chgPrechargeFailedBoost: [number, string] = [2598, 'b'];
export const PCS_a014_chgTempRationality: [number, string] = [2599, 'b'];
export const PCS_a015_chg12vUv: [number, string] = [2600, 'b'];
export const PCS_a016_chgAllPhasesFaulted: [number, string] = [2601, 'b'];
export const PCS_a017_chgWallPowerRemoval: [number, string] = [2602, 'b'];
export const PCS_a018_chgUnknownGridConfig: [number, string] = [2603, 'b'];
export const PCS_a019_acChargePowerLimited: [number, string] = [2604, 'b'];
export const PCS_a020_chgEnableLineMismatch: [number, string] = [2605, 'b'];
export const PCS_a021_hvpMia: [number, string] = [2606, 'b'];
export const PCS_a022_bmsMia: [number, string] = [2607, 'b'];
export const PCS_a023_cpMia: [number, string] = [2608, 'b'];
export const PCS_a024_vcfrontMia: [number, string] = [2609, 'b'];
export const PCS_a025_cpu2Malfunction: [number, string] = [2610, 'b'];
export const PCS_a026_watchdogAlarmed: [number, string] = [2611, 'b'];
export const PCS_a027_chgInsufficientCooling: [number, string] = [2612, 'b'];
export const PCS_a028_chgOutputUv: [number, string] = [2613, 'b'];
export const PCS_a029_chgPowerRationality: [number, string] = [2614, 'b'];
export const PCS_a030_canRationality: [number, string] = [2615, 'b'];
export const PCS_a031_uiMia: [number, string] = [2616, 'b'];
export const PCS_a032_gtwMia: [number, string] = [2617, 'b'];
export const PCS_a033_hvBusUv: [number, string] = [2618, 'b'];
export const PCS_a034_hvBusOv: [number, string] = [2619, 'b'];
export const PCS_a035_lvBusUv: [number, string] = [2620, 'b'];
export const PCS_a036_lvBusOv: [number, string] = [2621, 'b'];
export const PCS_a037_resonantTankOc: [number, string] = [2622, 'b'];
export const PCS_a038_claFaulted: [number, string] = [2623, 'b'];
export const PCS_a039_sdModuleClkFault: [number, string] = [2624, 'b'];
export const PCS_a040_dcdcMaxPowerReached: [number, string] = [2625, 'b'];
export const PCS_a041_dcdcOverTemp: [number, string] = [2626, 'b'];
export const PCS_a042_dcdcEnableLineMismatch: [number, string] = [2627, 'b'];
export const PCS_a043_hvBusPrechargeFailure: [number, string] = [2628, 'b'];
export const PCS_a044_12vSupportRegulation: [number, string] = [2629, 'b'];
export const PCS_a045_hvBusLowImpedance: [number, string] = [2630, 'b'];
export const PCS_a046_hvBusHighImpedence: [number, string] = [2631, 'b'];
export const PCS_a047_lvBusLowImpedance: [number, string] = [2632, 'b'];
export const PCS_a048_lvBusHighImpedance: [number, string] = [2633, 'b'];
export const PCS_a049_dcdcTempRationality: [number, string] = [2634, 'b'];
export const PCS_a050_dcdc12VsupportFaulted: [number, string] = [2635, 'b'];
export const PCS_a051_chgIntBusUv: [number, string] = [2636, 'b'];
export const PCS_a052_acVoltageNotPresent: [number, string] = [2637, 'b'];
export const PCS_a053_chgInputVDropHigh: [number, string] = [2638, 'b'];
export const PCS_a054_chgInputVDropTooHigh: [number, string] = [2639, 'b'];
export const PCS_a055_chgLineImedanceHigh: [number, string] = [2640, 'b'];
export const PCS_a056_chgLineImedanceTooHigh: [number, string] = [2641, 'b'];
export const PCS_a057_chgInputOverFreq: [number, string] = [2642, 'b'];
export const PCS_a058_chgInputUnderFreq: [number, string] = [2643, 'b'];
export const PCS_a059_chgInputOvRms: [number, string] = [2644, 'b'];
export const PCS_a060_chgInputOvPeak: [number, string] = [2645, 'b'];
export const PCS_a061_chgVLineRationality: [number, string] = [2646, 'b'];
export const PCS_a062_chgILineRationality: [number, string] = [2647, 'b'];
export const PCS_a063_chgVOutRationality: [number, string] = [2648, 'b'];
export const PCS_a064_chgIOutRationality: [number, string] = [2649, 'b'];
export const PCS_a065_chgPllNotLocked: [number, string] = [2650, 'b'];
export const PCS_a066_dcdcHvRationality: [number, string] = [2651, 'b'];
export const PCS_a067_dcdcLvRationality: [number, string] = [2652, 'b'];
export const PCS_a068_dcdcTankvRationality: [number, string] = [2653, 'b'];
export const PCS_a069_chgPfcLineDidt: [number, string] = [2654, 'b'];
export const PCS_a070_chgPfcLineDvdt: [number, string] = [2655, 'b'];
export const PCS_a071_chgPfcILoopRationality: [number, string] = [2656, 'b'];
export const PCS_a072_cpu2ClaStopped: [number, string] = [2657, 'b'];
export const PCS_a073_unexpectedAcInputVoltage: [number, string] = [2658, 'b'];
export const PCS_a074_hvBusDischargeFailure: [number, string] = [2659, 'b'];
export const PCS_a075_hvBusDischargeTimeout: [number, string] = [2660, 'b'];
export const PCS_a076_dcdcEnDeassertedErr: [number, string] = [2661, 'b'];
export const PCS_a077_microGridEnergyLow: [number, string] = [2662, 'b'];
export const PCS_a078_chgStopDcdcTooHot: [number, string] = [2663, 'b'];
export const PCS_a079_eepromOperationError: [number, string] = [2664, 'b'];
export const PCS_a080_damagedPhaseDetected: [number, string] = [2665, 'b'];
export const PCS_a081_dcdcPchgTimeout: [number, string] = [2666, 'b'];
export const PCS_a082_dcdcPchgUnsafeDiVoltage: [number, string] = [2667, 'b'];
export const PCS_a083_triggerOdin: [number, string] = [2668, 'b'];
export const PCS_a084_dcdcPchgStartVoltages: [number, string] = [2669, 'b'];
export const PCS_a085_dcdcFetsNotSwitching: [number, string] = [2670, 'b'];
export const PCS_a086_dcdcInsufficientCooling: [number, string] = [2671, 'b'];
export const PCS_a087_nvramRecordStatusError: [number, string] = [2672, 'b'];
export const PCS_a088_pchgParameters: [number, string] = [2673, 'b'];
export const PCS_a089_hvBusDischargeIrrational: [number, string] = [2674, 'b'];
export const PCS_a090_expectedAcVoltageSourceMissing: [number, string] = [2675, 'b'];
export const PCS_a091_chgIntBusRationality: [number, string] = [2676, 'b'];
export const PCS_a092_chgPowerLimitedByBusRipple: [number, string] = [2677, 'b'];
export const PCS_a093_powerRailRationality: [number, string] = [2678, 'b'];
export const PCS_a094_pcsDcdcNeedService: [number, string] = [2679, 'b'];
export const CP_a001_canRx: [number, string] = [2680, 'b'];
export const CP_a002_canTx: [number, string] = [2681, 'b'];
export const CP_a003_canError: [number, string] = [2682, 'b'];
export const CP_a004_proximityRationality: [number, string] = [2683, 'b'];
export const CP_a005_gbdcLiveDisconnect: [number, string] = [2684, 'b'];
export const CP_a006_lostCommsBMS: [number, string] = [2685, 'b'];
export const CP_a007_watchdog: [number, string] = [2686, 'b'];
export const CP_a008_memoryError: [number, string] = [2687, 'b'];
export const CP_a009_coverOpen: [number, string] = [2688, 'b'];
export const CP_a010_pilotRationality: [number, string] = [2689, 'b'];
export const CP_a011_eeprom: [number, string] = [2690, 'b'];
export const CP_a012_ledDriver: [number, string] = [2691, 'b'];
export const CP_a013_lostCommsGTW: [number, string] = [2692, 'b'];
export const CP_a014_lostCommsCHG: [number, string] = [2693, 'b'];
export const CP_a015_apsVov: [number, string] = [2694, 'b'];
export const CP_a016_apsVuv: [number, string] = [2695, 'b'];
export const CP_a017_fiveVov: [number, string] = [2696, 'b'];
export const CP_a018_fiveVuv: [number, string] = [2697, 'b'];
export const CP_a019_threeVov: [number, string] = [2698, 'b'];
export const CP_a020_threeVuv: [number, string] = [2699, 'b'];
export const CP_a021_zeroVov: [number, string] = [2700, 'b'];
export const CP_a022_zeroVuv: [number, string] = [2701, 'b'];
export const CP_a023_gbdcSessionFailed: [number, string] = [2702, 'b'];
export const CP_a024_ledsUC: [number, string] = [2703, 'b'];
export const CP_a025_ledsOC: [number, string] = [2704, 'b'];
export const CP_a026_networkManagement: [number, string] = [2705, 'b'];
export const CP_a027_doorSensorOutOfSpec: [number, string] = [2706, 'b'];
export const CP_a028_insertEnableMismatch: [number, string] = [2707, 'b'];
export const CP_a029_doorClosedProxPilot: [number, string] = [2708, 'b'];
export const CP_a030_busOff: [number, string] = [2709, 'b'];
export const CP_a031_doorClosedCommandedOpen: [number, string] = [2710, 'b'];
export const CP_a032_doorOpenExpectedClosed: [number, string] = [2711, 'b'];
export const CP_a033_spiOpen: [number, string] = [2712, 'b'];
export const CP_a034_calibrationIncomplete: [number, string] = [2713, 'b'];
export const CP_a035_latchMovement_1: [number, string] = [2714, 'b'];
export const CP_a036_latchNotDisengaged: [number, string] = [2715, 'b'];
export const CP_a037_latchNotEngaged: [number, string] = [2716, 'b'];
export const CP_a038_latchNotBlocking: [number, string] = [2717, 'b'];
export const CP_a039_latchMovement_2: [number, string] = [2718, 'b'];
export const CP_a040_doNotUse: [number, string] = [2719, 'b'];
export const CP_a041_doorSensorUnplugged: [number, string] = [2720, 'b'];
export const CP_a042_doorAssemblyBroken: [number, string] = [2721, 'b'];
export const CP_a043_doorPotIrrational: [number, string] = [2722, 'b'];
export const CP_a044_lostCommsHVP: [number, string] = [2723, 'b'];
export const CP_a045_lostCommsVCSEC: [number, string] = [2724, 'b'];
export const CP_a046_lostCommsEVSE: [number, string] = [2725, 'b'];
export const CP_a047_lostCommsVCFRONT: [number, string] = [2726, 'b'];
export const CP_a048_lostCommsUI: [number, string] = [2727, 'b'];
export const CP_a049_multipleCablesDetected: [number, string] = [2728, 'b'];
export const CP_a050_latchNotConnected: [number, string] = [2729, 'b'];
export const CP_a051_doorInductiveSensorMIA: [number, string] = [2730, 'b'];
export const CP_a052_evseNotSupported: [number, string] = [2731, 'b'];
export const CP_a053_proxLatchedNoPilot: [number, string] = [2732, 'b'];
export const CP_a054_cableNotSecured: [number, string] = [2733, 'b'];
export const CP_a055_chargeStoppedNoPilot: [number, string] = [2734, 'b'];
export const CP_a056_proxDisconnected: [number, string] = [2735, 'b'];
export const CP_a057_evseFaulted: [number, string] = [2736, 'b'];
export const CP_a058_acChargingBlocked: [number, string] = [2737, 'b'];
export const CP_a059_swcanError: [number, string] = [2738, 'b'];
export const CP_a060_lostCommsPCS: [number, string] = [2739, 'b'];
export const CP_a061_uhfReceiverMIA: [number, string] = [2740, 'b'];
export const CP_a062_scOutOfService: [number, string] = [2741, 'b'];
export const CP_a063_scUpdateInProgress: [number, string] = [2742, 'b'];
export const CP_a064_superchargingBlocked: [number, string] = [2743, 'b'];
export const CP_a065_selfTestFailed: [number, string] = [2744, 'b'];
export const CP_a066_proxLatchedIdlePilot: [number, string] = [2745, 'b'];
export const CP_a067_gbdcConnFault: [number, string] = [2746, 'b'];
export const CP_a068_doorSensorMismatch: [number, string] = [2747, 'b'];
export const CP_a069_doorInductiveSensorError: [number, string] = [2748, 'b'];
export const CP_a070_doorInductiveSensorReset: [number, string] = [2749, 'b'];
export const CP_a071_exiDecodeFailure: [number, string] = [2750, 'b'];
export const CP_a072_v2gEvccTimeout: [number, string] = [2751, 'b'];
export const CP_a073_iecComboShutdown: [number, string] = [2752, 'b'];
export const CP_a074_failedToEstablishV2gComm: [number, string] = [2753, 'b'];
export const CP_a075_v2gCommsFailure: [number, string] = [2754, 'b'];
export const CP_a076_LDC1612errorWatchdog: [number, string] = [2755, 'b'];
export const CP_a077_invalidMacAddress: [number, string] = [2756, 'b'];
export const CP_a078_latchNotDisengagedCold: [number, string] = [2757, 'b'];
export const CP_a079_cableNotSecuredCold: [number, string] = [2758, 'b'];
export const CP_a080_taskStackOverflow: [number, string] = [2759, 'b'];
export const CP_a081_swException: [number, string] = [2760, 'b'];
export const CP_a082_powerOnReset: [number, string] = [2761, 'b'];
export const CP_a083_watchdogTraceData: [number, string] = [2762, 'b'];
export const CP_a084_proximityPeDisconnected: [number, string] = [2763, 'b'];
export const CP_a085_dcPinTempFaulted: [number, string] = [2764, 'b'];
export const CP_a086_dcPinTempIrrational: [number, string] = [2765, 'b'];
export const CP_a087_dcTempModelFault: [number, string] = [2766, 'b'];
export const CP_a088_dcTempModelDeviation: [number, string] = [2767, 'b'];
export const CP_a089_plcConfigMismatch: [number, string] = [2768, 'b'];
export const CP_a090_ccsEvseLowIso: [number, string] = [2769, 'b'];
export const CP_a091_wrongSuperchargerHandle: [number, string] = [2770, 'b'];
export const CP_a092_modemAppLoadFailed: [number, string] = [2771, 'b'];
export const CP_a093_modemLoadedWithReset: [number, string] = [2772, 'b'];
export const CP_a094_inductiveResetSuccessful: [number, string] = [2773, 'b'];
export const CP_a095_thermalDcLimitActive: [number, string] = [2774, 'b'];
export const CP_a096_pilotWake: [number, string] = [2775, 'b'];
