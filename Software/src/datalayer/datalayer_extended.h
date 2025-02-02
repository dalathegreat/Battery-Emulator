#ifndef _DATALAYER_EXTENDED_H_
#define _DATALAYER_EXTENDED_H_

#include "../include.h"

typedef struct {
  /** uint16_t */
  /** PID polling parameters */
  uint16_t battery_5V_ref = 0;
  int16_t battery_module_temp_1 = 0;
  int16_t battery_module_temp_2 = 0;
  int16_t battery_module_temp_3 = 0;
  int16_t battery_module_temp_4 = 0;
  int16_t battery_module_temp_5 = 0;
  int16_t battery_module_temp_6 = 0;
  uint16_t battery_cell_average_voltage = 0;
  uint16_t battery_cell_average_voltage_2 = 0;
  uint16_t battery_terminal_voltage = 0;
  uint16_t battery_ignition_power_mode = 0;
  int16_t battery_current_7E7 = 0;
  uint16_t battery_capacity_my17_18 = 0;
  uint16_t battery_capacity_my19plus = 0;
  uint16_t battery_SOC_display = 0;
  uint16_t battery_SOC_raw_highprec = 0;
  uint16_t battery_max_temperature = 0;
  uint16_t battery_min_temperature = 0;
  uint16_t battery_max_cell_voltage = 0;
  uint16_t battery_min_cell_voltage = 0;
  uint16_t battery_lowest_cell = 0;
  uint16_t battery_highest_cell = 0;
  uint16_t battery_internal_resistance = 0;
  uint16_t battery_voltage_polled = 0;
  uint16_t battery_vehicle_isolation = 0;
  uint16_t battery_isolation_kohm = 0;
  uint16_t battery_HV_locked = 0;
  uint16_t battery_crash_event = 0;
  uint16_t battery_HVIL = 0;
  uint16_t battery_HVIL_status = 0;
  int16_t battery_current_7E4 = 0;
} DATALAYER_INFO_BOLTAMPERA;

typedef struct {
  /** uint16_t */
  /** Terminal 30 - 12V SME Supply Voltage */
  uint16_t T30_Voltage = 0;
  /** Status HVIL, 1 HVIL OK, 0 HVIL disconnected*/
  uint8_t hvil_status = 0;
  /** Min/Max Cell SOH*/
  uint16_t min_soh_state = 0;
  uint16_t max_soh_state = 0;
  uint32_t bms_uptime = 0;
  uint8_t pyro_status_pss1 = 0;
  uint8_t pyro_status_pss4 = 0;
  uint8_t pyro_status_pss6 = 0;
  int32_t iso_safety_positive = 0;
  int32_t iso_safety_negative = 0;
  int32_t iso_safety_parallel = 0;
  int32_t allowable_charge_amps = 0;
  int32_t allowable_discharge_amps = 0;
  int16_t balancing_status = 0;
  int16_t battery_voltage_after_contactor = 0;
  unsigned long min_cell_voltage_data_age = 0;
  unsigned long max_cell_voltage_data_age = 0;

} DATALAYER_INFO_BMWIX;

typedef struct {
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_ext = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_int = 0;
  /** uint8_t */
  /** Status cooling valve error, 0 not evaluated, 1 OK valve closed, 2 error active valve open, 3 Invalid signal*/
  uint8_t ST_valve_cooling = 0;
  /** uint8_t */
  /** Status interlock error, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_interlock = 0;
  /** uint8_t */
  /** Status precharge, 0 no statement, 1 Not active closing not blocked, 2 error precharge blocked, 3 Invalid signal*/
  uint8_t ST_precharge = 0;
  /** uint8_t */
  /** Status DC switch, 0 contactors open, 1 precharge ongoing, 2 contactors engaged, 3 Invalid signal*/
  uint8_t ST_DCSW = 0;
  /** uint8_t */
  /** Status emergency, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_EMG = 0;
  /** uint8_t */
  /** Status welding detection, 0 Contactors OK, 1 One contactor welded, 2 Two contactors welded, 3 Invalid signal*/
  uint8_t ST_WELD = 0;
  /** uint8_t */
  /** Status isolation, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_isolation = 0;
  /** uint8_t */
  /** Status cold shutoff valve, 0 OK, 1 Short circuit to GND, 2 Short circuit to 12V, 3 Line break, 6 Driver error, 12 Stuck, 13 Stuck, 15 Invalid Signal*/
  uint8_t ST_cold_shutoff_valve = 0;
  /** uint16_t */
  /** Terminal 30 - 12V SME Supply Voltage */
  uint16_t T30_Voltage = 0;
  /** Status HVIL, 1 HVIL OK, 0 HVIL disconnected*/
  uint8_t hvil_status = 0;
  /** Min/Max Cell SOH*/
  uint16_t min_soh_state = 0;
  uint16_t max_soh_state = 0;
  int32_t allowable_charge_amps = 0;
  int32_t allowable_discharge_amps = 0;
  int16_t balancing_status = 0;
  int16_t battery_voltage_after_contactor = 0;
  unsigned long min_cell_voltage_data_age = 0;
  unsigned long max_cell_voltage_data_age = 0;
  int32_t iso_safety_int_kohm = 0;  //STAT_ISOWIDERSTAND_INT_WERT
  int32_t iso_safety_ext_kohm = 0;  //STAT_ISOWIDERSTAND_EXT_STD_WERT
  int32_t iso_safety_trg_kohm = 0;
  int32_t iso_safety_ext_plausible = 0;  //STAT_ISOWIDERSTAND_EXT_TRG_PLAUS
  int32_t iso_safety_int_plausible = 0;
  int32_t iso_safety_trg_plausible = 0;
  int32_t iso_safety_kohm = 0;          //STAT_R_ISO_ROH_01_WERT
  int32_t iso_safety_kohm_quality = 0;  //STAT_R_ISO_ROH_QAL_01_INFO Quality of measurement 0-21 (higher better)

} DATALAYER_INFO_BMWPHEV;

typedef struct {
  /** uint16_t */
  /** SOC% raw battery value. Might not always reach 100% */
  uint16_t SOC_raw = 0;
  /** uint16_t */
  /** SOC% instrumentation cluster value. Will always reach 100% */
  uint16_t SOC_dash = 0;
  /** uint16_t */
  /** SOC% OBD2 value, polled actively */
  uint16_t SOC_OBD2 = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_ext = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_int = 0;
  /** uint8_t */
  /** Status cooling valve error, 0 not evaluated, 1 OK valve closed, 2 error active valve open, 3 Invalid signal*/
  uint8_t ST_valve_cooling = 0;
  /** uint8_t */
  /** Status interlock error, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_interlock = 0;
  /** uint8_t */
  /** Status precharge, 0 no statement, 1 Not active closing not blocked, 2 error precharge blocked, 3 Invalid signal*/
  uint8_t ST_precharge = 0;
  /** uint8_t */
  /** Status DC switch, 0 contactors open, 1 precharge ongoing, 2 contactors engaged, 3 Invalid signal*/
  uint8_t ST_DCSW = 0;
  /** uint8_t */
  /** Status emergency, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_EMG = 0;
  /** uint8_t */
  /** Status welding detection, 0 Contactors OK, 1 One contactor welded, 2 Two contactors welded, 3 Invalid signal*/
  uint8_t ST_WELD = 0;
  /** uint8_t */
  /** Status isolation, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_isolation = 0;
  /** uint8_t */
  /** Status cold shutoff valve, 0 OK, 1 Short circuit to GND, 2 Short circuit to 12V, 3 Line break, 6 Driver error, 12 Stuck, 13 Stuck, 15 Invalid Signal*/
  uint8_t ST_cold_shutoff_valve = 0;
} DATALAYER_INFO_BMWI3;

typedef struct {
  /** bool */
  /** Which SOC method currently used. 0 = Estimated, 1 = Measured */
  bool SOC_method = 0;
  /** uint16_t */
  /** SOC% estimate. Estimated from total pack voltage */
  uint16_t SOC_estimated = 0;
  /** uint16_t */
  /** SOC% raw battery value. Highprecision. Can be locked if pack is crashed */
  uint16_t SOC_highprec = 0;
  /** uint16_t */
  /** SOC% polled OBD2 value. Can be locked if pack is crashed */
  uint16_t SOC_polled = 0;
  /** uint16_t */
  /** Voltage raw battery value */
  uint16_t voltage_periodic = 0;
  /** uint16_t */
  /** Voltage polled OBD2*/
  uint16_t voltage_polled = 0;

} DATALAYER_INFO_BYDATTO3;

typedef struct {
  /** bool */
  /** All values either True or false */
  bool system_state_discharge = false;
  bool system_state_charge = false;
  bool system_state_cellbalancing = false;
  bool system_state_tricklecharge = false;
  bool system_state_idle = false;
  bool system_state_chargecompleted = false;
  bool system_state_maintenancecharge = false;
  bool IO_state_main_positive_relay = false;
  bool IO_state_main_negative_relay = false;
  bool IO_state_charge_enable = false;
  bool IO_state_precharge_relay = false;
  bool IO_state_discharge_enable = false;
  bool IO_state_IO_6 = false;
  bool IO_state_IO_7 = false;
  bool IO_state_IO_8 = false;
  bool error_Cell_overvoltage = false;
  bool error_Cell_undervoltage = false;
  bool error_Cell_end_of_life_voltage = false;
  bool error_Cell_voltage_misread = false;
  bool error_Cell_over_temperature = false;
  bool error_Cell_under_temperature = false;
  bool error_Cell_unmanaged = false;
  bool error_LMU_over_temperature = false;
  bool error_LMU_under_temperature = false;
  bool error_Temp_sensor_open_circuit = false;
  bool error_Temp_sensor_short_circuit = false;
  bool error_SUB_communication = false;
  bool error_LMU_communication = false;
  bool error_Over_current_IN = false;
  bool error_Over_current_OUT = false;
  bool error_Short_circuit = false;
  bool error_Leak_detected = false;
  bool error_Leak_detection_failed = false;
  bool error_Voltage_difference = false;
  bool error_BMCU_supply_over_voltage = false;
  bool error_BMCU_supply_under_voltage = false;
  bool error_Main_positive_contactor = false;
  bool error_Main_negative_contactor = false;
  bool error_Precharge_contactor = false;
  bool error_Midpack_contactor = false;
  bool error_Precharge_timeout = false;
  bool error_Emergency_connector_override = false;
  bool warning_High_cell_voltage = false;
  bool warning_Low_cell_voltage = false;
  bool warning_High_cell_temperature = false;
  bool warning_Low_cell_temperature = false;
  bool warning_High_LMU_temperature = false;
  bool warning_Low_LMU_temperature = false;
  bool warning_SUB_communication_interfered = false;
  bool warning_LMU_communication_interfered = false;
  bool warning_High_current_IN = false;
  bool warning_High_current_OUT = false;
  bool warning_Pack_resistance_difference = false;
  bool warning_High_pack_resistance = false;
  bool warning_Cell_resistance_difference = false;
  bool warning_High_cell_resistance = false;
  bool warning_High_BMCU_supply_voltage = false;
  bool warning_Low_BMCU_supply_voltage = false;
  bool warning_Low_SOC = false;
  bool warning_Balancing_required_OCV_model = false;
  bool warning_Charger_not_responding = false;
} DATALAYER_INFO_CELLPOWER;

typedef struct {
  uint8_t total_cell_count = 0;
  int16_t battery_12V = 0;
  uint8_t waterleakageSensor = 0;
  int8_t temperature_water_inlet = 0;
  int8_t powerRelayTemperature = 0;
  uint8_t batteryManagementMode = 0;
  uint8_t BMS_ign = 0;
  uint8_t batteryRelay = 0;
#ifdef DOUBLE_BATTERY
  uint8_t battery2_total_cell_count = 0;
  int16_t battery2_battery_12V = 0;
  uint8_t battery2_waterleakageSensor = 0;
  int8_t battery2_temperature_water_inlet = 0;
  int8_t battery2_powerRelayTemperature = 0;
  uint8_t battery2_batteryManagementMode = 0;
  uint8_t battery2_BMS_ign = 0;
  uint8_t battery2_batteryRelay = 0;
#endif  //DOUBLE BATTERY
} DATALAYER_INFO_KIAHYUNDAI64;

typedef struct {
  /** uint8_t */
  /** Contactor status */
  uint8_t status_contactor = 0;
  /** uint8_t */
  /** Contactor status */
  uint8_t hvil_status = 0;
  /** uint8_t */
  /** Negative contactor state */
  uint8_t packContNegativeState = 0;
  /** uint8_t */
  /** Positive contactor state */
  uint8_t packContPositiveState = 0;
  /** uint8_t */
  /** Set state of contactors */
  uint8_t packContactorSetState = 0;
  /** uint8_t */
  /** Battery pack allows closing of contacors */
  uint8_t packCtrsClosingAllowed = 0;
  /** uint8_t */
  /** Pyro test in progress */
  bool pyroTestInProgress = false;
  bool battery_packCtrsOpenNowRequested = false;
  bool battery_packCtrsOpenRequested = false;
  uint8_t battery_packCtrsRequestStatus = 0;
  bool battery_packCtrsResetRequestRequired = false;
  bool battery_dcLinkAllowedToEnergize = false;
  uint8_t BMS_SerialNumber[15] = {0};  //stores raw HEX values for ASCII chars
  uint8_t battery_beginning_of_life = 0;
  uint8_t battery_battTempPct = 0;
  uint16_t battery_dcdcLvBusVolt = 0;
  uint16_t battery_dcdcHvBusVolt = 0;
  uint16_t battery_dcdcLvOutputCurrent = 0;
  bool BMS352_mux = false;  // variable to store when 0x352 mux is present
  uint16_t battery_nominal_full_pack_energy = 0;
  uint16_t battery_nominal_full_pack_energy_m0 = 0;
  uint16_t battery_nominal_energy_remaining = 0;
  uint16_t battery_nominal_energy_remaining_m0 = 0;
  uint16_t battery_ideal_energy_remaining = 0;
  uint16_t battery_ideal_energy_remaining_m0 = 0;
  uint16_t battery_energy_to_charge_complete = 0;
  uint16_t battery_energy_to_charge_complete_m1 = 0;
  uint16_t battery_energy_buffer = 0;
  uint16_t battery_energy_buffer_m1 = 0;
  uint16_t battery_expected_energy_remaining = 0;
  uint16_t battery_expected_energy_remaining_m1 = 0;
  bool battery_full_charge_complete = false;
  bool battery_fully_charged = false;
  uint16_t battery_total_discharge = 0;
  uint16_t battery_total_charge = 0;
  uint16_t battery_BrickVoltageMax = 0;
  uint16_t battery_BrickVoltageMin = 0;
  uint8_t battery_BrickVoltageMaxNum = 0;
  uint8_t battery_BrickVoltageMinNum = 0;
  uint8_t battery_BrickTempMaxNum = 0;
  uint8_t battery_BrickTempMinNum = 0;
  uint8_t battery_BrickModelTMax = 0;
  uint8_t battery_BrickModelTMin = 0;
  uint16_t battery_packConfigMultiplexer = 0;
  uint16_t battery_moduleType = 0;
  uint16_t battery_reservedConfig = 0;
  uint32_t battery_packMass = 0;
  uint32_t battery_platformMaxBusVoltage = 0;
  uint32_t battery_bms_min_voltage = 0;
  uint32_t battery_bms_max_voltage = 0;
  uint32_t battery_max_charge_current = 0;
  uint32_t battery_max_discharge_current = 0;
  uint32_t battery_soc_min = 0;
  uint32_t battery_soc_max = 0;
  uint32_t battery_soc_ave = 0;
  uint32_t battery_soc_ui = 0;
  uint8_t battery_BMS_contactorState = 0;
  uint8_t battery_BMS_state = 0;
  uint8_t battery_BMS_hvState = 0;
  uint16_t battery_BMS_isolationResistance = 0;
  uint8_t battery_BMS_uiChargeStatus = 0;
  bool battery_BMS_diLimpRequest = false;
  uint16_t battery_BMS_chgPowerAvailable = 0;
  bool battery_BMS_pcsPwmEnabled = false;
  uint8_t battery_PCS_dcdcPrechargeStatus = 0;
  uint8_t battery_PCS_dcdc12VSupportStatus = 0;
  uint8_t battery_PCS_dcdcHvBusDischargeStatus = 0;
  uint8_t battery_PCS_dcdcMainState = 0;
  uint8_t battery_PCS_dcdcSubState = 0;
  bool battery_PCS_dcdcFaulted = false;
  bool battery_PCS_dcdcOutputIsLimited = false;
  uint16_t battery_PCS_dcdcMaxOutputCurrentAllowed = 0;
  uint8_t battery_PCS_dcdcPrechargeRtyCnt = 0;
  uint8_t battery_PCS_dcdc12VSupportRtyCnt = 0;
  uint8_t battery_PCS_dcdcDischargeRtyCnt = 0;
  uint8_t battery_PCS_dcdcPwmEnableLine = 0;
  uint8_t battery_PCS_dcdcSupportingFixedLvTarget = 0;
  uint8_t battery_PCS_dcdcPrechargeRestartCnt = 0;
  uint8_t battery_PCS_dcdcInitialPrechargeSubState = 0;
  uint16_t BMS_maxRegenPower = 0;
  uint16_t BMS_maxDischargePower = 0;
  uint16_t BMS_maxStationaryHeatPower = 0;
  uint16_t BMS_hvacPowerBudget = 0;
  uint8_t BMS_notEnoughPowerForHeatPump = 0;
  uint8_t BMS_powerLimitState = 0;
  uint8_t BMS_inverterTQF = 0;
  uint16_t BMS_powerDissipation = 0;
  uint8_t BMS_flowRequest = 0;
  uint16_t BMS_inletActiveCoolTargetT = 0;
  uint16_t BMS_inletPassiveTargetT = 0;
  uint16_t BMS_inletActiveHeatTargetT = 0;
  uint16_t BMS_packTMin = 0;
  uint16_t BMS_packTMax = 0;
  bool BMS_pcsNoFlowRequest = false;
  bool BMS_noFlowRequest = false;
  uint16_t PCS_dcdcTemp = 0;
  uint16_t PCS_ambientTemp = 0;
  uint16_t PCS_chgPhATemp = 0;
  uint16_t PCS_chgPhBTemp = 0;
  uint16_t PCS_chgPhCTemp = 0;
  uint16_t PCS_dcdcMaxLvOutputCurrent = 0;
  uint16_t PCS_dcdcCurrentLimit = 0;
  uint16_t PCS_dcdcLvOutputCurrentTempLimit = 0;
  uint16_t PCS_dcdcUnifiedCommand = 0;
  uint16_t PCS_dcdcCLAControllerOutput = 0;
  uint16_t PCS_dcdcTankVoltage = 0;
  uint16_t PCS_dcdcTankVoltageTarget = 0;
  uint16_t PCS_dcdcClaCurrentFreq = 0;
  uint16_t PCS_dcdcTCommMeasured = 0;
  uint16_t PCS_dcdcShortTimeUs = 0;
  uint16_t PCS_dcdcHalfPeriodUs = 0;
  uint16_t PCS_dcdcIntervalMaxFrequency = 0;
  uint16_t PCS_dcdcIntervalMaxHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvOutputCurr = 0;
  uint16_t PCS_dcdcIntervalMinFrequency = 0;
  uint16_t PCS_dcdcIntervalMinHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvOutputCurr = 0;
  uint32_t PCS_dcdc12vSupportLifetimekWh = 0;
  bool HVP_gpioPassivePyroDepl = false;
  bool HVP_gpioPyroIsoEn = false;
  bool HVP_gpioCpFaultIn = false;
  bool HVP_gpioPackContPowerEn = false;
  bool HVP_gpioHvCablesOk = false;
  bool HVP_gpioHvpSelfEnable = false;
  bool HVP_gpioLed = false;
  bool HVP_gpioCrashSignal = false;
  bool HVP_gpioShuntDataReady = false;
  bool HVP_gpioFcContPosAux = false;
  bool HVP_gpioFcContNegAux = false;
  bool HVP_gpioBmsEout = false;
  bool HVP_gpioCpFaultOut = false;
  bool HVP_gpioPyroPor = false;
  bool HVP_gpioShuntEn = false;
  bool HVP_gpioHvpVerEn = false;
  bool HVP_gpioPackCoontPosFlywheel = false;
  bool HVP_gpioCpLatchEnable = false;
  bool HVP_gpioPcsEnable = false;
  bool HVP_gpioPcsDcdcPwmEnable = false;
  bool HVP_gpioPcsChargePwmEnable = false;
  bool HVP_gpioFcContPowerEnable = false;
  bool HVP_gpioHvilEnable = false;
  bool HVP_gpioSecDrdy = false;
  uint16_t HVP_hvp1v5Ref = 0;
  uint16_t HVP_shuntCurrentDebug = 0;
  bool HVP_packCurrentMia = false;
  bool HVP_auxCurrentMia = false;
  bool HVP_currentSenseMia = false;
  bool HVP_shuntRefVoltageMismatch = false;
  bool HVP_shuntThermistorMia = false;
  uint8_t HVP_shuntHwMia = 0;
  uint16_t HVP_dcLinkVoltage = 0;
  uint16_t HVP_packVoltage = 0;
  uint16_t HVP_fcLinkVoltage = 0;
  uint16_t HVP_packContVoltage = 0;
  uint16_t HVP_packNegativeV = 0;
  uint16_t HVP_packPositiveV = 0;
  uint16_t HVP_pyroAnalog = 0;
  uint16_t HVP_dcLinkNegativeV = 0;
  uint16_t HVP_dcLinkPositiveV = 0;
  uint16_t HVP_fcLinkNegativeV = 0;
  uint16_t HVP_fcContCoilCurrent = 0;
  uint16_t HVP_fcContVoltage = 0;
  uint16_t HVP_hvilInVoltage = 0;
  uint16_t HVP_hvilOutVoltage = 0;
  uint16_t HVP_fcLinkPositiveV = 0;
  uint16_t HVP_packContCoilCurrent = 0;
  uint16_t HVP_battery12V = 0;
  uint16_t HVP_shuntRefVoltageDbg = 0;
  uint16_t HVP_shuntAuxCurrentDbg = 0;
  uint16_t HVP_shuntBarTempDbg = 0;
  uint16_t HVP_shuntAsicTempDbg = 0;
  uint8_t HVP_shuntAuxCurrentStatus = 0;
  uint8_t HVP_shuntBarTempStatus = 0;
  uint8_t HVP_shuntAsicTempStatus = 0;
} DATALAYER_INFO_TESLA;

typedef struct {
  /** uint8_t */
  /** Battery info, stores raw HEX values for ASCII chars */
  uint8_t BatterySerialNumber[15] = {0};
  uint8_t BatteryPartNumber[7] = {0};
  uint8_t BMSIDcode[8] = {0};
  /** uint8_t */
  /** Enum, ZE0 = 0, AZE0 = 1, ZE1 = 2 */
  uint8_t LEAF_gen = 0;
  /** uint16_t */
  /** 77Wh per gid. LEAF specific unit */
  uint16_t GIDS = 0;
  /** uint16_t */
  /** Max regen power in kW */
  uint16_t ChargePowerLimit = 0;
  /** int16_t */
  /** Max charge power in kW */
  int16_t MaxPowerForCharger = 0;
  /** bool */
  /** Interlock status */
  bool Interlock = false;
  /** int16_t */
  /** Insulation resistance, most likely kOhm */
  uint16_t Insulation = 0;
  /** uint8_t */
  /** battery_FAIL status */
  uint8_t RelayCutRequest = 0;
  /** uint8_t */
  /** battery_STATUS status */
  uint8_t FailsafeStatus = 0;
  /** bool */
  /** True if fully charged */
  bool Full = false;
  /** bool */
  /** True if battery empty */
  bool Empty = false;
  /** bool */
  /** Battery pack allows closing of contacors */
  bool MainRelayOn = false;
  /** bool */
  /** True if heater exists */
  bool HeatExist = false;
  /** bool */
  /** Heater stopped */
  bool HeatingStop = false;
  /** bool */
  /** Heater starting */
  bool HeatingStart = false;
  /** bool */
  /** Heat request sent*/
  bool HeaterSendRequest = false;
  /** bool */
  /** User requesting SOH reset via WebUI*/
  bool UserRequestSOHreset = false;
  /** bool */
  /** True if the crypto challenge response from BMS is signalling a failed attempt*/
  bool challengeFailed = false;
  /** uint32_t */
  /** Cryptographic challenge to be solved */
  uint32_t CryptoChallenge = 0;
  /** uint32_t */
  /** Solution for crypto challenge, MSBs */
  uint32_t SolvedChallengeMSB = 0;
  /** uint32_t */
  /** Solution for crypto challenge, LSBs */
  uint32_t SolvedChallengeLSB = 0;

} DATALAYER_INFO_NISSAN_LEAF;

typedef struct {
  /** uint8_t */
  /** Service disconnect switch status */
  bool SDSW = 0;
  /** uint8_t */
  /** Pilotline status */
  bool pilotline = 0;
  /** uint8_t */
  /** Transportation mode status */
  bool transportmode = 0;
  /** uint8_t */
  /** Componentprotection mode status */
  bool componentprotection = 0;
  /** uint8_t */
  /** Shutdown status */
  bool shutdown_active = 0;
  /** uint8_t */
  /** Battery heating status */
  bool battery_heating = 0;
  /** uint8_t */
  /** All realtime_ warnings have same enumeration, 0 = no fault, 1 = error level 1, 2 error level 2, 3 error level 3 */
  uint8_t rt_overcurrent = 0;
  uint8_t rt_CAN_fault = 0;
  uint8_t rt_overcharge = 0;
  uint8_t rt_SOC_high = 0;
  uint8_t rt_SOC_low = 0;
  uint8_t rt_SOC_jumping = 0;
  uint8_t rt_temp_difference = 0;
  uint8_t rt_cell_overtemp = 0;
  uint8_t rt_cell_undertemp = 0;
  uint8_t rt_battery_overvolt = 0;
  uint8_t rt_battery_undervol = 0;
  uint8_t rt_cell_overvolt = 0;
  uint8_t rt_cell_undervol = 0;
  uint8_t rt_cell_imbalance = 0;
  uint8_t rt_battery_unathorized = 0;
  /** uint8_t */
  /** HVIL status, 0 = Init, 1 = Closed, 2 = Open!, 3 = Fault */
  uint8_t HVIL = 0;
  /** uint8_t */
  /** 0 = HV inactive, 1 = HV active, 2 = Balancing, 3 = Extern charging, 4 = AC charging, 5 = Battery error, 6 = DC charging, 7 = init */
  uint8_t BMS_mode = 0;
  /** uint8_t */
  /** 1 = Battery display, 4 = Battery display OK, 4 = Display battery charging, 6 = Display battery check, 7 = Fault */
  uint8_t battery_diagnostic = 0;
  /** uint8_t */
  /** 0 = init, 1 = no open HV line detected, 2 = open HV line , 3 = fault */
  uint8_t status_HV_line = 0;
  /** uint8_t */
  /** 0 = OK, 1 = Not OK, 0x06 = init, 0x07 = fault */
  uint8_t warning_support = 0;
  /** uint32_t */
  /** Isolation resistance in kOhm */
  uint32_t isolation_resistance = 0;
  /** uint8_t */
  /** 0=Init, 1=BMS intermediate circuit voltage-free (U_Zwkr < 20V), 2=BMS intermediate circuit not voltage-free (U_Zwkr >/= 25V, hysteresis), 3=Error */
  uint8_t BMS_status_voltage_free = 0;
  /** uint8_t */
  /** 0 Component_IO, 1 Restricted_CompFkt_Isoerror_I, 2 Restricted_CompFkt_Isoerror_II, 3 Restricted_CompFkt_Interlock, 4 Restricted_CompFkt_SD, 5 Restricted_CompFkt_Performance red, 6 = No component function, 7 = Init */
  uint8_t BMS_error_status = 0;
  /** uint8_t */
  /** 0 init, 1 closed, 2 open, 3 fault */
  uint8_t BMS_Kl30c_Status = 0;
  /** bool */
  /** true if BMS requests error/warning light */
  bool BMS_OBD_MIL = 0;
  bool BMS_error_lamp_req = 0;
  bool BMS_warning_lamp_req = 0;
  int32_t BMS_voltage_intermediate_dV = 0;
  int32_t BMS_voltage_dV = 0;
  uint8_t balancing_active = 0;
  bool balancing_request = 0;
  bool charging_active = 0;
  float temp_points[18] = {0};
  uint16_t celltemperature_dC[56] = {0};
  uint16_t battery_temperature_dC = 0;
} DATALAYER_INFO_MEB;

typedef struct {
  uint16_t soc_bms = 0;
  uint16_t soc_calc = 0;
  uint16_t soc_rescaled = 0;
  uint16_t soh_bms = 0;
  uint16_t BECMsupplyVoltage = 0;

  uint16_t BECMBatteryVoltage = 0;
  uint16_t BECMBatteryCurrent = 0;
  uint16_t BECMUDynMaxLim = 0;
  uint16_t BECMUDynMinLim = 0;

  uint16_t HvBattPwrLimDcha1 = 0;
  uint16_t HvBattPwrLimDchaSoft = 0;
  uint16_t HvBattPwrLimDchaSlowAgi = 0;
  uint16_t HvBattPwrLimChrgSlowAgi = 0;

  uint8_t HVSysRlySts = 0;
  uint8_t HVSysDCRlySts1 = 0;
  uint8_t HVSysDCRlySts2 = 0;
  uint8_t HVSysIsoRMonrSts = 0;
  /** User requesting DTC reset via WebUI*/
  bool UserRequestDTCreset = false;
  /** User requesting DTC readout via WebUI*/
  bool UserRequestDTCreadout = false;
  /** User requesting BECM reset via WebUI*/
  bool UserRequestBECMecuReset = false;

} DATALAYER_INFO_VOLVO_POLESTAR;

typedef struct {
  /** uint16_t */
  /** Values WIP*/
  uint16_t battery_soc = 0;
  uint16_t battery_usable_soc = 0;
  uint16_t battery_soh = 0;
  uint16_t battery_pack_voltage = 0;
  uint16_t battery_max_cell_voltage = 0;
  uint16_t battery_min_cell_voltage = 0;
  uint16_t battery_12v = 0;
  uint16_t battery_avg_temp = 0;
  uint16_t battery_min_temp = 0;
  uint16_t battery_max_temp = 0;
  uint16_t battery_max_power = 0;
  uint16_t battery_interlock = 0;
  uint16_t battery_kwh = 0;
  uint16_t battery_current = 0;
  uint16_t battery_current_offset = 0;
  uint16_t battery_max_generated = 0;
  uint16_t battery_max_available = 0;
  uint16_t battery_current_voltage = 0;
  uint16_t battery_charging_status = 0;
  uint16_t battery_remaining_charge = 0;
  uint16_t battery_balance_capacity_total = 0;
  uint16_t battery_balance_time_total = 0;
  uint16_t battery_balance_capacity_sleep = 0;
  uint16_t battery_balance_time_sleep = 0;
  uint16_t battery_balance_capacity_wake = 0;
  uint16_t battery_balance_time_wake = 0;
  uint16_t battery_bms_state = 0;
  uint16_t battery_balance_switches = 0;
  uint16_t battery_energy_complete = 0;
  uint16_t battery_energy_partial = 0;
  uint16_t battery_slave_failures = 0;
  uint16_t battery_mileage = 0;
  uint16_t battery_fan_speed = 0;
  uint16_t battery_fan_period = 0;
  uint16_t battery_fan_control = 0;
  uint16_t battery_fan_duty = 0;
  uint16_t battery_temporisation = 0;
  uint16_t battery_time = 0;
  uint16_t battery_pack_time = 0;
  uint16_t battery_soc_min = 0;
  uint16_t battery_soc_max = 0;
} DATALAYER_INFO_ZOE_PH2;

class DataLayerExtended {
 public:
  DATALAYER_INFO_BOLTAMPERA boltampera;
  DATALAYER_INFO_BMWIX bmwix;
  DATALAYER_INFO_BMWPHEV bmwphev;
  DATALAYER_INFO_BMWI3 bmwi3;
  DATALAYER_INFO_BYDATTO3 bydAtto3;
  DATALAYER_INFO_CELLPOWER cellpower;
  DATALAYER_INFO_KIAHYUNDAI64 KiaHyundai64;
  DATALAYER_INFO_TESLA tesla;
  DATALAYER_INFO_NISSAN_LEAF nissanleaf;
  DATALAYER_INFO_MEB meb;
  DATALAYER_INFO_VOLVO_POLESTAR VolvoPolestar;
  DATALAYER_INFO_ZOE_PH2 zoePH2;
};

extern DataLayerExtended datalayer_extended;

#endif
