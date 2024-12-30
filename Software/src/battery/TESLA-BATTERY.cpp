#include "../include.h"
#ifdef TESLA_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For Advanced Battery Insights webpage
#include "../devboard/utils/events.h"
#include "TESLA-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
/* Credits: Most of the code comes from Per Carlen's bms_comms_tesla_model3.py (https://gitlab.com/pelle8/batt2gen24/) */

static unsigned long previousMillis50 = 0;  // will store last time a 50ms CAN Message was send
//0x221 545 VCFRONT_LVPowerState: "GenMsgCycleTime" 50ms
CAN_frame TESLA_221_1 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x221,
    .data = {0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96}};  //Contactor frame 221 - close contactors
CAN_frame TESLA_221_2 = {
    .FD = false,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x221,
    .data = {0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA}};  //Contactor Frame 221 - hv_up_for_drive

static uint16_t sendContactorClosingMessagesStill = 300;
static uint16_t battery_cell_max_v = 3300;
static uint16_t battery_cell_min_v = 3300;
static uint16_t battery_cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static bool cellvoltagesRead = false;
//0x3d2: 978 BMS_kwhCounter
static uint32_t battery_total_discharge = 0;
static uint32_t battery_total_charge = 0;
//0x352: 850 BMS_energyStatus
static uint16_t battery_energy_buffer = 0;                 // kWh
static uint16_t battery_energy_buffer_m1 = 0;              // kWh
static uint16_t battery_energy_to_charge_complete = 0;     // kWh
static uint16_t battery_energy_to_charge_complete_m1 = 0;  // kWh
static uint16_t battery_expected_energy_remaining = 0;     // kWh
static uint16_t battery_expected_energy_remaining_m1 = 0;  // kWh
static bool battery_full_charge_complete = false;          // Changed to bool
static bool battery_fully_charged = false;                 // Changed to bool
static uint16_t battery_ideal_energy_remaining = 0;        // kWh
static uint16_t battery_ideal_energy_remaining_m0 = 0;     // kWh
static uint16_t battery_nominal_energy_remaining = 0;      // kWh
static uint16_t battery_nominal_energy_remaining_m0 = 0;   // kWh
static uint16_t battery_nominal_full_pack_energy = 0;      // Kwh
static uint16_t battery_nominal_full_pack_energy_m0 = 0;   // Kwh
//0x132 306 HVBattAmpVolt
static uint16_t battery_volts = 0;                  // V
static int16_t battery_amps = 0;                    // A
static int16_t battery_raw_amps = 0;                // A
static uint16_t battery_charge_time_remaining = 0;  // Minutes
//0x252 594 BMS_powerAvailable
static uint16_t BMS_maxRegenPower = 0;           //rename from battery_regenerative_limit
static uint16_t BMS_maxDischargePower = 0;       // rename from battery_discharge_limit
static uint16_t BMS_maxStationaryHeatPower = 0;  //rename from battery_max_heat_park
static uint16_t BMS_hvacPowerBudget = 0;         //rename from battery_hvac_max_power
static uint8_t BMS_notEnoughPowerForHeatPump = 0;
static uint8_t BMS_powerLimitState = 0;
static uint8_t BMS_inverterTQF = 0;
//0x2d2: 722 BMSVAlimits
static uint16_t battery_max_discharge_current = 0;
static uint16_t battery_max_charge_current = 0;
static uint16_t battery_bms_max_voltage = 0;
static uint16_t battery_bms_min_voltage = 0;
//0x2b4: 692 PCS_dcdcRailStatus
static uint16_t battery_dcdcHvBusVolt = 0;  // Change name from battery_high_voltage to battery_dcdcHvBusVolt
static uint16_t battery_dcdcLvBusVolt = 0;  // Change name from battery_low_voltage to battery_dcdcLvBusVolt
static uint16_t battery_dcdcLvOutputCurrent =
    0;  // Change name from battery_output_current to battery_dcdcLvOutputCurrent
//0x292: 658 BMS_socStatus
static uint16_t battery_beginning_of_life = 0;  // kWh
static uint16_t battery_soc_min = 0;
static uint16_t battery_soc_max = 0;
static uint16_t battery_soc_ui = 0;  //Change name from battery_soc_vi to reflect DBC battery_soc_ui
static uint16_t battery_soc_ave = 0;
static uint8_t battery_battTempPct = 0;
//0x392: BMS_packConfig
static uint32_t battery_packMass = 0;
static uint32_t battery_platformMaxBusVoltage = 0;
static uint32_t battery_packConfigMultiplexer = 0;
static uint32_t battery_moduleType = 0;
static uint32_t battery_reservedConfig = 0;
//0x332: 818 BattBrickMinMax:BMS_bmbMinMax
static int16_t battery_max_temp = 0;  // C*
static int16_t battery_min_temp = 0;  // C*
static uint16_t battery_BrickVoltageMax = 0;
static uint16_t battery_BrickVoltageMin = 0;
static uint8_t battery_BrickTempMaxNum = 0;
static uint8_t battery_BrickTempMinNum = 0;
static uint8_t battery_BrickModelTMax = 0;
static uint8_t battery_BrickModelTMin = 0;
static uint8_t battery_BrickVoltageMaxNum = 0;  //rename from battery_max_vno
static uint8_t battery_BrickVoltageMinNum = 0;  //rename from battery_min_vno
//0x20A: 522 HVP_contactorState
static uint8_t battery_contactor = 0;  //State of contactor
static uint8_t battery_hvil_status = 0;
static uint8_t battery_packContNegativeState = 0;
static uint8_t battery_packContPositiveState = 0;
static uint8_t battery_packContactorSetState = 0;
static bool battery_packCtrsClosingAllowed = false;    // Change to bool
static bool battery_pyroTestInProgress = false;        // Change to bool
static bool battery_packCtrsOpenNowRequested = false;  // Change to bool
static bool battery_packCtrsOpenRequested = false;     // Change to bool
static uint8_t battery_packCtrsRequestStatus = 0;
static bool battery_packCtrsResetRequestRequired = false;  // Change to bool
static bool battery_dcLinkAllowedToEnergize = false;       // Change to bool
static bool battery_fcContNegativeAuxOpen = false;         // Change to bool
static uint8_t battery_fcContNegativeState = 0;
static bool battery_fcContPositiveAuxOpen = false;  // Change to bool
static uint8_t battery_fcContPositiveState = 0;
static uint8_t battery_fcContactorSetState = 0;
static bool battery_fcCtrsClosingAllowed = false;    // Change to bool
static bool battery_fcCtrsOpenNowRequested = false;  // Change to bool
static bool battery_fcCtrsOpenRequested = false;     // Change to bool
static uint8_t battery_fcCtrsRequestStatus = 0;
static bool battery_fcCtrsResetRequestRequired = false;  // Change to bool
static bool battery_fcLinkAllowedToEnergize = false;     // Change to bool
//0x212: 530 BMS_status
static bool battery_BMS_hvacPowerRequest = false;          //Change to bool
static bool battery_BMS_notEnoughPowerForDrive = false;    //Change to bool
static bool battery_BMS_notEnoughPowerForSupport = false;  //Change to bool
static bool battery_BMS_preconditionAllowed = false;       //Change to bool
static bool battery_BMS_updateAllowed = false;             //Change to bool
static bool battery_BMS_activeHeatingWorthwhile = false;   //Change to bool
static bool battery_BMS_cpMiaOnHvs = false;                //Change to bool
static uint8_t battery_BMS_contactorState = 0;
static uint8_t battery_BMS_state = 0;
static uint8_t battery_BMS_hvState = 0;
static uint16_t battery_BMS_isolationResistance = 0;
static bool battery_BMS_chargeRequest = false;    //Change to bool
static bool battery_BMS_keepWarmRequest = false;  //Change to bool
static uint8_t battery_BMS_uiChargeStatus = 0;
static bool battery_BMS_diLimpRequest = false;   //Change to bool
static bool battery_BMS_okToShipByAir = false;   //Change to bool
static bool battery_BMS_okToShipByLand = false;  //Change to bool
static uint32_t battery_BMS_chgPowerAvailable = 0;
static uint8_t battery_BMS_chargeRetryCount = 0;
static bool battery_BMS_pcsPwmEnabled = false;        //Change to bool
static bool battery_BMS_ecuLogUploadRequest = false;  //Change to bool
static uint8_t battery_BMS_minPackTemperature = 0;
// 0x224:548 PCS_dcdcStatus
static uint8_t battery_PCS_dcdcPrechargeStatus = 0;
static uint8_t battery_PCS_dcdc12VSupportStatus = 0;
static uint8_t battery_PCS_dcdcHvBusDischargeStatus = 0;
static uint16_t battery_PCS_dcdcMainState = 0;
static uint8_t battery_PCS_dcdcSubState = 0;
static bool battery_PCS_dcdcFaulted = false;          //Change to bool
static bool battery_PCS_dcdcOutputIsLimited = false;  //Change to bool
static uint32_t battery_PCS_dcdcMaxOutputCurrentAllowed = 0;
static uint8_t battery_PCS_dcdcPrechargeRtyCnt = 0;
static uint8_t battery_PCS_dcdc12VSupportRtyCnt = 0;
static uint8_t battery_PCS_dcdcDischargeRtyCnt = 0;
static uint8_t battery_PCS_dcdcPwmEnableLine = 0;
static uint8_t battery_PCS_dcdcSupportingFixedLvTarget = 0;
static uint8_t battery_PCS_ecuLogUploadRequest = 0;
static uint8_t battery_PCS_dcdcPrechargeRestartCnt = 0;
static uint8_t battery_PCS_dcdcInitialPrechargeSubState = 0;
//0x312: 786 BMS_thermalStatus
static uint16_t BMS_powerDissipation = 0;
static uint16_t BMS_flowRequest = 0;
static uint16_t BMS_inletActiveCoolTargetT = 0;
static uint16_t BMS_inletPassiveTargetT = 0;
static uint16_t BMS_inletActiveHeatTargetT = 0;
static uint16_t BMS_packTMin = 0;
static uint16_t BMS_packTMax = 0;
static bool BMS_pcsNoFlowRequest = false;
static bool BMS_noFlowRequest = false;
//0x2A4; 676 PCS_thermalStatus
static int16_t PCS_chgPhATemp = 0;
static int16_t PCS_chgPhBTemp = 0;
static int16_t PCS_chgPhCTemp = 0;
static int16_t PCS_dcdcTemp = 0;
static int16_t PCS_ambientTemp = 0;
//0x2C4; 708 PCS_logging
static uint16_t PCS_logMessageSelect = 0;
static uint16_t PCS_dcdcMaxLvOutputCurrent = 0;
static uint16_t PCS_dcdcCurrentLimit = 0;
static uint16_t PCS_dcdcLvOutputCurrentTempLimit = 0;
static uint16_t PCS_dcdcUnifiedCommand = 0;
static uint16_t PCS_dcdcCLAControllerOutput = 0;
static int16_t PCS_dcdcTankVoltage = 0;
static uint16_t PCS_dcdcTankVoltageTarget = 0;
static uint16_t PCS_dcdcClaCurrentFreq = 0;
static int16_t PCS_dcdcTCommMeasured = 0;
static uint16_t PCS_dcdcShortTimeUs = 0;
static uint16_t PCS_dcdcHalfPeriodUs = 0;
static uint16_t PCS_dcdcIntervalMaxFrequency = 0;
static uint16_t PCS_dcdcIntervalMaxHvBusVolt = 0;
static uint16_t PCS_dcdcIntervalMaxLvBusVolt = 0;
static uint16_t PCS_dcdcIntervalMaxLvOutputCurr = 0;
static uint16_t PCS_dcdcIntervalMinFrequency = 0;
static uint16_t PCS_dcdcIntervalMinHvBusVolt = 0;
static uint16_t PCS_dcdcIntervalMinLvBusVolt = 0;
static uint16_t PCS_dcdcIntervalMinLvOutputCurr = 0;
static uint32_t PCS_dcdc12vSupportLifetimekWh = 0;
//0x7AA: //1962 HVP_debugMessage:
static uint8_t HVP_debugMessageMultiplexer = 0;
static bool HVP_gpioPassivePyroDepl = false;       //Change to bool
static bool HVP_gpioPyroIsoEn = false;             //Change to bool
static bool HVP_gpioCpFaultIn = false;             //Change to bool
static bool HVP_gpioPackContPowerEn = false;       //Change to bool
static bool HVP_gpioHvCablesOk = false;            //Change to bool
static bool HVP_gpioHvpSelfEnable = false;         //Change to bool
static bool HVP_gpioLed = false;                   //Change to bool
static bool HVP_gpioCrashSignal = false;           //Change to bool
static bool HVP_gpioShuntDataReady = false;        //Change to bool
static bool HVP_gpioFcContPosAux = false;          //Change to bool
static bool HVP_gpioFcContNegAux = false;          //Change to bool
static bool HVP_gpioBmsEout = false;               //Change to bool
static bool HVP_gpioCpFaultOut = false;            //Change to bool
static bool HVP_gpioPyroPor = false;               //Change to bool
static bool HVP_gpioShuntEn = false;               //Change to bool
static bool HVP_gpioHvpVerEn = false;              //Change to bool
static bool HVP_gpioPackCoontPosFlywheel = false;  //Change to bool
static bool HVP_gpioCpLatchEnable = false;         //Change to bool
static bool HVP_gpioPcsEnable = false;             //Change to bool
static bool HVP_gpioPcsDcdcPwmEnable = false;      //Change to bool
static bool HVP_gpioPcsChargePwmEnable = false;    //Change to bool
static bool HVP_gpioFcContPowerEnable = false;     //Change to bool
static bool HVP_gpioHvilEnable = false;            //Change to bool
static bool HVP_gpioSecDrdy = false;               //Change to bool
static uint16_t HVP_hvp1v5Ref = 0;
static int16_t HVP_shuntCurrentDebug = 0;
static bool HVP_packCurrentMia = false;           //Change to bool
static bool HVP_auxCurrentMia = false;            //Change to bool
static bool HVP_currentSenseMia = false;          //Change to bool
static bool HVP_shuntRefVoltageMismatch = false;  //Change to bool
static bool HVP_shuntThermistorMia = false;       //Change to bool
static bool HVP_shuntHwMia = false;               //Change to bool
static int16_t HVP_dcLinkVoltage = 0;
static int16_t HVP_packVoltage = 0;
static int16_t HVP_fcLinkVoltage = 0;
static uint16_t HVP_packContVoltage = 0;
static int16_t HVP_packNegativeV = 0;
static int16_t HVP_packPositiveV = 0;
static uint16_t HVP_pyroAnalog = 0;
static int16_t HVP_dcLinkNegativeV = 0;
static int16_t HVP_dcLinkPositiveV = 0;
static int16_t HVP_fcLinkNegativeV = 0;
static uint16_t HVP_fcContCoilCurrent = 0;
static uint16_t HVP_fcContVoltage = 0;
static uint16_t HVP_hvilInVoltage = 0;
static uint16_t HVP_hvilOutVoltage = 0;
static int16_t HVP_fcLinkPositiveV = 0;
static uint16_t HVP_packContCoilCurrent = 0;
static uint16_t HVP_battery12V = 0;
static int16_t HVP_shuntRefVoltageDbg = 0;
static int16_t HVP_shuntAuxCurrentDbg = 0;
static int16_t HVP_shuntBarTempDbg = 0;
static int16_t HVP_shuntAsicTempDbg = 0;
static uint8_t HVP_shuntAuxCurrentStatus = 0;
static uint8_t HVP_shuntBarTempStatus = 0;
static uint8_t HVP_shuntAsicTempStatus = 0;
//0x3aa: HVP_alertMatrix1 Fault codes   // Change to bool
static bool battery_WatchdogReset = false;   //Warns if the processor has experienced a reset due to watchdog reset.
static bool battery_PowerLossReset = false;  //Warns if the processor has experienced a reset due to power loss.
static bool battery_SwAssertion = false;     //An internal software assertion has failed.
static bool battery_CrashEvent = false;      //Warns if the crash signal is detected by HVP
static bool battery_OverDchgCurrentFault = false;  //Warns if the pack discharge is above max discharge current limit
static bool battery_OverChargeCurrentFault =
    false;  //Warns if the pack discharge current is above max charge current limit
static bool battery_OverCurrentFault =
    false;  //Warns if the pack current (discharge or charge) is above max current limit.
static bool battery_OverTemperatureFault = false;  //A pack module temperature is above maximum temperature limit
static bool battery_OverVoltageFault = false;      //A brick voltage is above maximum voltage limit
static bool battery_UnderVoltageFault = false;     //A brick voltage is below minimum voltage limit
static bool battery_PrimaryBmbMiaFault =
    false;  //Warns if the voltage and temperature readings from primary BMB chain are mia
static bool battery_SecondaryBmbMiaFault =
    false;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
static bool battery_BmbMismatchFault =
    false;  //Warns if the primary and secondary BMB chain readings don't match with each other
static bool battery_BmsHviMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
static bool battery_CpMiaFault = false;       //Warns if the CP node is mia on HVS CAN
static bool battery_PcsMiaFault = false;      //The PCS node is mia on HVS CAN
static bool battery_BmsFault = false;         //Warns if the BMS ECU has faulted
static bool battery_PcsFault = false;         //Warns if the PCS ECU has faulted
static bool battery_CpFault = false;          //Warns if the CP ECU has faulted
static bool battery_ShuntHwMiaFault = false;  //Warns if the shunt current reading is not available
static bool battery_PyroMiaFault = false;     //Warns if the pyro squib is not connected
static bool battery_hvsMiaFault = false;      //Warns if the pack contactor hw fault
static bool battery_hviMiaFault = false;      //Warns if the FC contactor hw fault
static bool battery_Supply12vFault = false;  //Warns if the low voltage (12V) battery is below minimum voltage threshold
static bool battery_VerSupplyFault =
    false;                              //Warns if the Energy reserve voltage supply is below minimum voltage threshold
static bool battery_HvilFault = false;  //Warn if a High Voltage Inter Lock fault is detected
static bool battery_BmsHvsMiaFault = false;  //Warns if the BMS node is mia on HVS or HVI CAN
static bool battery_PackVoltMismatchFault =
    false;  //Warns if the pack voltage doesn't match approximately with sum of brick voltages
static bool battery_EnsMiaFault = false;         //Warns if the ENS line is not connected to HVC
static bool battery_PackPosCtrArcFault = false;  //Warns if the HVP detectes series arc at pack contactor
static bool battery_packNegCtrArcFault = false;  //Warns if the HVP detectes series arc at FC contactor
static bool battery_ShuntHwAndBmsMiaFault = false;
static bool battery_fcContHwFault = false;
static bool battery_robinOverVoltageFault = false;
static bool battery_packContHwFault = false;
static bool battery_pyroFuseBlown = false;
static bool battery_pyroFuseFailedToBlow = false;
static bool battery_CpilFault = false;
static bool battery_PackContactorFellOpen = false;
static bool battery_FcContactorFellOpen = false;
static bool battery_packCtrCloseBlocked = false;
static bool battery_fcCtrCloseBlocked = false;
static bool battery_packContactorForceOpen = false;
static bool battery_fcContactorForceOpen = false;
static bool battery_dcLinkOverVoltage = false;
static bool battery_shuntOverTemperature = false;
static bool battery_passivePyroDeploy = false;
static bool battery_logUploadRequest = false;
static bool battery_packCtrCloseFailed = false;
static bool battery_fcCtrCloseFailed = false;
static bool battery_shuntThermistorMia = false;
//0x320: 800 BMS_alertMatrix
static uint8_t battery_BMS_matrixIndex = 0;  // Changed to bool
static bool battery_BMS_a061_robinBrickOverVoltage = false;
static bool battery_BMS_a062_SW_BrickV_Imbalance = false;
static bool battery_BMS_a063_SW_ChargePort_Fault = false;
static bool battery_BMS_a064_SW_SOC_Imbalance = false;
static bool battery_BMS_a127_SW_shunt_SNA = false;
static bool battery_BMS_a128_SW_shunt_MIA = false;
static bool battery_BMS_a069_SW_Low_Power = false;
static bool battery_BMS_a130_IO_CAN_Error = false;
static bool battery_BMS_a071_SW_SM_TransCon_Not_Met = false;
static bool battery_BMS_a132_HW_BMB_OTP_Uncorrctbl = false;
static bool battery_BMS_a134_SW_Delayed_Ctr_Off = false;
static bool battery_BMS_a075_SW_Chg_Disable_Failure = false;
static bool battery_BMS_a076_SW_Dch_While_Charging = false;
static bool battery_BMS_a017_SW_Brick_OV = false;
static bool battery_BMS_a018_SW_Brick_UV = false;
static bool battery_BMS_a019_SW_Module_OT = false;
static bool battery_BMS_a021_SW_Dr_Limits_Regulation = false;
static bool battery_BMS_a022_SW_Over_Current = false;
static bool battery_BMS_a023_SW_Stack_OV = false;
static bool battery_BMS_a024_SW_Islanded_Brick = false;
static bool battery_BMS_a025_SW_PwrBalance_Anomaly = false;
static bool battery_BMS_a026_SW_HFCurrent_Anomaly = false;
static bool battery_BMS_a087_SW_Feim_Test_Blocked = false;
static bool battery_BMS_a088_SW_VcFront_MIA_InDrive = false;
static bool battery_BMS_a089_SW_VcFront_MIA = false;
static bool battery_BMS_a090_SW_Gateway_MIA = false;
static bool battery_BMS_a091_SW_ChargePort_MIA = false;
static bool battery_BMS_a092_SW_ChargePort_Mia_On_Hv = false;
static bool battery_BMS_a034_SW_Passive_Isolation = false;
static bool battery_BMS_a035_SW_Isolation = false;
static bool battery_BMS_a036_SW_HvpHvilFault = false;
static bool battery_BMS_a037_SW_Flood_Port_Open = false;
static bool battery_BMS_a158_SW_HVP_HVI_Comms = false;
static bool battery_BMS_a039_SW_DC_Link_Over_Voltage = false;
static bool battery_BMS_a041_SW_Power_On_Reset = false;
static bool battery_BMS_a042_SW_MPU_Error = false;
static bool battery_BMS_a043_SW_Watch_Dog_Reset = false;
static bool battery_BMS_a044_SW_Assertion = false;
static bool battery_BMS_a045_SW_Exception = false;
static bool battery_BMS_a046_SW_Task_Stack_Usage = false;
static bool battery_BMS_a047_SW_Task_Stack_Overflow = false;
static bool battery_BMS_a048_SW_Log_Upload_Request = false;
static bool battery_BMS_a169_SW_FC_Pack_Weld = false;
static bool battery_BMS_a050_SW_Brick_Voltage_MIA = false;
static bool battery_BMS_a051_SW_HVC_Vref_Bad = false;
static bool battery_BMS_a052_SW_PCS_MIA = false;
static bool battery_BMS_a053_SW_ThermalModel_Sanity = false;
static bool battery_BMS_a054_SW_Ver_Supply_Fault = false;
static bool battery_BMS_a176_SW_GracefulPowerOff = false;
static bool battery_BMS_a059_SW_Pack_Voltage_Sensing = false;
static bool battery_BMS_a060_SW_Leakage_Test_Failure = false;
static bool battery_BMS_a077_SW_Charger_Regulation = false;
static bool battery_BMS_a081_SW_Ctr_Close_Blocked = false;
static bool battery_BMS_a082_SW_Ctr_Force_Open = false;
static bool battery_BMS_a083_SW_Ctr_Close_Failure = false;
static bool battery_BMS_a084_SW_Sleep_Wake_Aborted = false;
static bool battery_BMS_a094_SW_Drive_Inverter_MIA = false;
static bool battery_BMS_a099_SW_BMB_Communication = false;
static bool battery_BMS_a105_SW_One_Module_Tsense = false;
static bool battery_BMS_a106_SW_All_Module_Tsense = false;
static bool battery_BMS_a107_SW_Stack_Voltage_MIA = false;
static bool battery_BMS_a121_SW_NVRAM_Config_Error = false;
static bool battery_BMS_a122_SW_BMS_Therm_Irrational = false;
static bool battery_BMS_a123_SW_Internal_Isolation = false;
static bool battery_BMS_a129_SW_VSH_Failure = false;
static bool battery_BMS_a131_Bleed_FET_Failure = false;
static bool battery_BMS_a136_SW_Module_OT_Warning = false;
static bool battery_BMS_a137_SW_Brick_UV_Warning = false;
static bool battery_BMS_a138_SW_Brick_OV_Warning = false;
static bool battery_BMS_a139_SW_DC_Link_V_Irrational = false;
static bool battery_BMS_a141_SW_BMB_Status_Warning = false;
static bool battery_BMS_a144_Hvp_Config_Mismatch = false;
static bool battery_BMS_a145_SW_SOC_Change = false;
static bool battery_BMS_a146_SW_Brick_Overdischarged = false;
static bool battery_BMS_a149_SW_Missing_Config_Block = false;
static bool battery_BMS_a151_SW_external_isolation = false;
static bool battery_BMS_a156_SW_BMB_Vref_bad = false;
static bool battery_BMS_a157_SW_HVP_HVS_Comms = false;
static bool battery_BMS_a159_SW_HVP_ECU_Error = false;
static bool battery_BMS_a161_SW_DI_Open_Request = false;
static bool battery_BMS_a162_SW_No_Power_For_Support = false;
static bool battery_BMS_a163_SW_Contactor_Mismatch = false;
static bool battery_BMS_a164_SW_Uncontrolled_Regen = false;
static bool battery_BMS_a165_SW_Pack_Partial_Weld = false;
static bool battery_BMS_a166_SW_Pack_Full_Weld = false;
static bool battery_BMS_a167_SW_FC_Partial_Weld = false;
static bool battery_BMS_a168_SW_FC_Full_Weld = false;
static bool battery_BMS_a170_SW_Limp_Mode = false;
static bool battery_BMS_a171_SW_Stack_Voltage_Sense = false;
static bool battery_BMS_a174_SW_Charge_Failure = false;
static bool battery_BMS_a179_SW_Hvp_12V_Fault = false;
static bool battery_BMS_a180_SW_ECU_reset_blocked = false;

#ifdef DOUBLE_BATTERY  //need to update for battery2

static uint16_t battery2_cell_max_v = 3300;
static uint16_t battery2_cell_min_v = 3300;
static uint16_t battery2_cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static bool battery2_cellvoltagesRead = false;
//0x3d2: 978 BMS_kwhCounter
static uint32_t battery2_total_discharge = 0;
static uint32_t battery2_total_charge = 0;
//0x352: 850 BMS_energyStatus
static uint16_t battery2_energy_buffer = 0;
static uint16_t battery2_energy_buffer_m1 = 0;  // kWh
static uint16_t battery2_energy_to_charge_complete = 0;
static uint16_t battery2_energy_to_charge_complete_m1 = 0;  // kWh
static uint16_t battery2_expected_energy_remaining = 0;
static uint16_t battery2_expected_energy_remaining_m1 = 0;  // kWh
static bool battery2_full_charge_complete = false;
static bool battery2_fully_charged = false;
static uint16_t battery2_ideal_energy_remaining = 0;
static uint16_t battery2_ideal_energy_remaining_m0 = 0;  // kWh
static uint16_t battery2_nominal_energy_remaining = 0;
static uint16_t battery2_nominal_energy_remaining_m0 = 0;  // kWh
static uint16_t battery2_nominal_full_pack_energy = 0;
static uint16_t battery2_nominal_full_pack_energy_m0 = 0;  // Kwh
//0x132 306 HVBattAmpVolt
static uint16_t battery2_volts = 0;                  // V
static int16_t battery2_amps = 0;                    // A
static int16_t battery2_raw_amps = 0;                // A
static uint16_t battery2_charge_time_remaining = 0;  // Minutes
//0x252 594 BMS_powerAvailable
static uint16_t BMS2_maxRegenPower = 0;           //rename from battery_regenerative_limit
static uint16_t BMS2_maxDischargePower = 0;       // rename from battery_discharge_limit
static uint16_t BMS2_maxStationaryHeatPower = 0;  //rename from battery_max_heat_park
static uint16_t BMS2_hvacPowerBudget = 0;         //rename from battery_hvac_max_power
static uint8_t BMS2_notEnoughPowerForHeatPump = 0;
static uint8_t BMS2_powerLimitState = 0;
static uint8_t BMS2_inverterTQF = 0;
//0x2d2: 722 BMSVAlimits
static uint16_t battery2_max_discharge_current = 0;
static uint16_t battery2_max_charge_current = 0;
static uint16_t battery2_bms_max_voltage = 0;
static uint16_t battery2_bms_min_voltage = 0;
//0x2b4: 692 PCS_dcdcRailStatus
static uint16_t battery2_dcdcHvBusVolt = 0;        //update name
static uint16_t battery2_dcdcLvBusVolt = 0;        //update name
static uint16_t battery2_dcdcLvOutputCurrent = 0;  //update name
//0x292: 658 BMS_socStatus
static uint16_t battery2_beginning_of_life = 0;
static uint16_t battery2_soc_min = 0;
static uint16_t battery2_soc_max = 0;
static uint16_t battery2_soc_ui = 0;
static uint16_t battery2_soc_ave = 0;
static uint8_t battery2_battTempPct = 0;
//0x392: BMS_packConfig
static uint32_t battery2_packMass = 0;
static uint32_t battery2_platformMaxBusVoltage = 0;
static uint32_t battery2_packConfigMultiplexer = 0;
static uint32_t battery2_moduleType = 0;
static uint32_t battery2_reservedConfig = 0;
//0x332: 818 BattBrickMinMax:BMS_bmbMinMax
static int16_t battery2_max_temp = 0;  // C*
static int16_t battery2_min_temp = 0;  // C*
static uint16_t battery2_BrickVoltageMax = 0;
static uint16_t battery2_BrickVoltageMin = 0;
static uint8_t battery2_BrickTempMaxNum = 0;
static uint8_t battery2_BrickTempMinNum = 0;
static uint8_t battery2_BrickModelTMax = 0;
static uint8_t battery2_BrickModelTMin = 0;
static uint8_t battery2_BrickVoltageMaxNum = 0;  //rename from battery_max_vno
static uint8_t battery2_BrickVoltageMinNum = 0;  //rename from battery_min_vno
//0x20A: 522 HVP_contactorState
static uint8_t battery2_contactor = 0;  //State of contactor
static uint8_t battery2_hvil_status = 0;
static uint8_t battery2_packContNegativeState = 0;
static uint8_t battery2_packContPositiveState = 0;
static uint8_t battery2_packContactorSetState = 0;
static bool battery2_packCtrsClosingAllowed = false;
static bool battery2_pyroTestInProgress = false;
static bool battery2_packCtrsOpenNowRequested = false;
static bool battery2_packCtrsOpenRequested = false;
static uint8_t battery2_packCtrsRequestStatus = 0;
static bool battery2_packCtrsResetRequestRequired = false;
static bool battery2_dcLinkAllowedToEnergize = false;
static bool battery2_fcContNegativeAuxOpen = false;
static uint8_t battery2_fcContNegativeState = 0;
static bool battery2_fcContPositiveAuxOpen = false;
static uint8_t battery2_fcContPositiveState = 0;
static uint8_t battery2_fcContactorSetState = 0;
static bool battery2_fcCtrsClosingAllowed = false;
static bool battery2_fcCtrsOpenNowRequested = false;
static bool battery2_fcCtrsOpenRequested = false;
static uint8_t battery2_fcCtrsRequestStatus = 0;
static bool battery2_fcCtrsResetRequestRequired = false;
static bool battery2_fcLinkAllowedToEnergize = false;
//0x212: 530 BMS_status
static bool battery2_BMS_hvacPowerRequest = false;
static bool battery2_BMS_notEnoughPowerForDrive = false;
static bool battery2_BMS_notEnoughPowerForSupport = false;
static bool battery2_BMS_preconditionAllowed = false;
static bool battery2_BMS_updateAllowed = false;
static bool battery2_BMS_activeHeatingWorthwhile = false;
static bool battery2_BMS_cpMiaOnHvs = false;
static uint8_t battery2_BMS_contactorState = 0;
static uint8_t battery2_BMS_state = 0;
static uint8_t battery2_BMS_hvState = 0;
static uint16_t battery2_BMS_isolationResistance = 0;
static bool battery2_BMS_chargeRequest = false;
static bool battery2_BMS_keepWarmRequest = false;
static uint8_t battery2_BMS_uiChargeStatus = 0;
static bool battery2_BMS_diLimpRequest = false;
static bool battery2_BMS_okToShipByAir = false;
static bool battery2_BMS_okToShipByLand = false;
static uint32_t battery2_BMS_chgPowerAvailable = 0;
static uint8_t battery2_BMS_chargeRetryCount = 0;
static bool battery2_BMS_pcsPwmEnabled = false;
static bool battery2_BMS_ecuLogUploadRequest = false;
static uint8_t battery2_BMS_minPackTemperature = 0;
// 0x224:548 PCS_dcdcStatus
static uint8_t battery2_PCS_dcdcPrechargeStatus = 0;
static uint8_t battery2_PCS_dcdc12VSupportStatus = 0;
static uint8_t battery2_PCS_dcdcHvBusDischargeStatus = 0;
static uint16_t battery2_PCS_dcdcMainState = 0;
static uint8_t battery2_PCS_dcdcSubState = 0;
static bool battery2_PCS_dcdcFaulted = false;
static bool battery2_PCS_dcdcOutputIsLimited = false;
static uint32_t battery2_PCS_dcdcMaxOutputCurrentAllowed = 0;
static uint8_t battery2_PCS_dcdcPrechargeRtyCnt = 0;
static uint8_t battery2_PCS_dcdc12VSupportRtyCnt = 0;
static uint8_t battery2_PCS_dcdcDischargeRtyCnt = 0;
static uint8_t battery2_PCS_dcdcPwmEnableLine = 0;
static uint8_t battery2_PCS_dcdcSupportingFixedLvTarget = 0;
static uint8_t battery2_PCS_ecuLogUploadRequest = 0;
static uint8_t battery2_PCS_dcdcPrechargeRestartCnt = 0;
static uint8_t battery2_PCS_dcdcInitialPrechargeSubState = 0;
//0x312: 786 BMS_thermalStatus
static uint16_t BMS2_powerDissipation = 0;
static uint16_t BMS2_flowRequest = 0;
static uint16_t BMS2_inletActiveCoolTargetT = 0;
static uint16_t BMS2_inletPassiveTargetT = 0;
static uint16_t BMS2_inletActiveHeatTargetT = 0;
static uint16_t BMS2_packTMin = 0;
static uint16_t BMS2_packTMax = 0;
static bool BMS2_pcsNoFlowRequest = false;
static bool BMS2_noFlowRequest = false;
//0x2A4; 676 PCS_thermalStatus
static int16_t PCS2_chgPhATemp = 0;
static int16_t PCS2_chgPhBTemp = 0;
static int16_t PCS2_chgPhCTemp = 0;
static int16_t PCS2_dcdcTemp = 0;
static int16_t PCS2_ambientTemp = 0;
//0x2C4; 708 PCS_logging
static uint16_t PCS2_logMessageSelect = 0;
static uint16_t PCS2_dcdcMaxLvOutputCurrent = 0;
static uint16_t PCS2_dcdcCurrentLimit = 0;
static uint16_t PCS2_dcdcLvOutputCurrentTempLimit = 0;
static uint16_t PCS2_dcdcUnifiedCommand = 0;
static uint16_t PCS2_dcdcCLAControllerOutput = 0;
static int16_t PCS2_dcdcTankVoltage = 0;
static uint16_t PCS2_dcdcTankVoltageTarget = 0;
static uint16_t PCS2_dcdcClaCurrentFreq = 0;
static int16_t PCS2_dcdcTCommMeasured = 0;
static uint16_t PCS2_dcdcShortTimeUs = 0;
static uint16_t PCS2_dcdcHalfPeriodUs = 0;
static uint16_t PCS2_dcdcIntervalMaxFrequency = 0;
static uint16_t PCS2_dcdcIntervalMaxHvBusVolt = 0;
static uint16_t PCS2_dcdcIntervalMaxLvBusVolt = 0;
static uint16_t PCS2_dcdcIntervalMaxLvOutputCurr = 0;
static uint16_t PCS2_dcdcIntervalMinFrequency = 0;
static uint16_t PCS2_dcdcIntervalMinHvBusVolt = 0;
static uint16_t PCS2_dcdcIntervalMinLvBusVolt = 0;
static uint16_t PCS2_dcdcIntervalMinLvOutputCurr = 0;
static uint32_t PCS2_dcdc12vSupportLifetimekWh = 0;
//0x7AA: //1962 HVP_debugMessage:
static uint8_t HVP2_debugMessageMultiplexer = 0;
static bool HVP2_gpioPassivePyroDepl = false;
static bool HVP2_gpioPyroIsoEn = false;
static bool HVP2_gpioCpFaultIn = false;
static bool HVP2_gpioPackContPowerEn = false;
static bool HVP2_gpioHvCablesOk = false;
static bool HVP2_gpioHVPSelfEnable = false;
static bool HVP2_gpioLed = false;
static bool HVP2_gpioCrashSignal = false;
static bool HVP2_gpioShuntDataReady = false;
static bool HVP2_gpioFcContPosAux = false;
static bool HVP2_gpioFcContNegAux = false;
static bool HVP2_gpioBmsEout = false;
static bool HVP2_gpioCpFaultOut = false;
static bool HVP2_gpioPyroPor = false;
static bool HVP2_gpioShuntEn = false;
static bool HVP2_gpioHVPVerEn = false;
static bool HVP2_gpioPackCoontPosFlywheel = false;
static bool HVP2_gpioCpLatchEnable = false;
static bool HVP2_gpioPcsEnable = false;
static bool HVP2_gpioPcsDcdcPwmEnable = false;
static bool HVP2_gpioPcsChargePwmEnable = false;
static bool HVP2_gpioFcContPowerEnable = false;
static bool HVP2_gpioHvilEnable = false;
static bool HVP2_gpioSecDrdy = false;
static uint16_t HVP2_hvp1v5Ref = 0;
static int16_t HVP2_shuntCurrentDebug = 0;
static bool HVP2_packCurrentMia = false;
static bool HVP2_auxCurrentMia = false;
static bool HVP2_currentSenseMia = false;
static bool HVP2_shuntRefVoltageMismatch = false;
static bool HVP2_shuntThermistorMia = false;
static bool HVP2_shuntHwMia = false;
static int16_t HVP2_dcLinkVoltage = 0;
static int16_t HVP2_packVoltage = 0;
static int16_t HVP2_fcLinkVoltage = 0;
static uint16_t HVP2_packContVoltage = 0;
static int16_t HVP2_packNegativeV = 0;
static int16_t HVP2_packPositiveV = 0;
static uint16_t HVP2_pyroAnalog = 0;
static int16_t HVP2_dcLinkNegativeV = 0;
static int16_t HVP2_dcLinkPositiveV = 0;
static int16_t HVP2_fcLinkNegativeV = 0;
static uint16_t HVP2_fcContCoilCurrent = 0;
static uint16_t HVP2_fcContVoltage = 0;
static uint16_t HVP2_hvilInVoltage = 0;
static uint16_t HVP2_hvilOutVoltage = 0;
static int16_t HVP2_fcLinkPositiveV = 0;
static uint16_t HVP2_packContCoilCurrent = 0;
static uint16_t HVP2_battery12V = 0;
static int16_t HVP2_shuntRefVoltageDbg = 0;
static int16_t HVP2_shuntAuxCurrentDbg = 0;
static int16_t HVP2_shuntBarTempDbg = 0;
static int16_t HVP2_shuntAsicTempDbg = 0;
static uint8_t HVP2_shuntAuxCurrentStatus = 0;
static uint8_t HVP2_shuntBarTempStatus = 0;
static uint8_t HVP2_shuntAsicTempStatus = 0;
//0x3aa: HVP_alertMatrix1 Fault codes
static bool battery2_WatchdogReset = false;   //Warns if the processor has experienced a reset due to watchdog reset.
static bool battery2_PowerLossReset = false;  //Warns if the processor has experienced a reset due to power loss.
static bool battery2_SwAssertion = false;     //An internal software assertion has failed.
static bool battery2_CrashEvent = false;      //Warns if the crash signal is detected by HVP
static bool battery2_OverDchgCurrentFault = false;  //Warns if the pack discharge is above max discharge current limit
static bool battery2_OverChargeCurrentFault =
    false;  //Warns if the pack discharge current is above max charge current limit
static bool battery2_OverCurrentFault =
    false;  //Warns if the pack current (discharge or charge) is above max current limit.
static bool battery2_OverTemperatureFault = false;  //A pack module temperature is above maximum temperature limit
static bool battery2_OverVoltageFault = false;      //A brick voltage is above maximum voltage limit
static bool battery2_UnderVoltageFault = false;     //A brick voltage is below minimum voltage limit
static bool battery2_PrimaryBmbMiaFault =
    false;  //Warns if the voltage and temperature readings from primary BMB chain are mia
static bool battery2_SecondaryBmbMiaFault =
    false;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
static bool battery2_BmbMismatchFault =
    false;  //Warns if the primary and secondary BMB chain readings don't match with each other
static bool battery2_BmsHviMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
static bool battery2_CpMiaFault = false;       //Warns if the CP node is mia on HVS CAN
static bool battery2_PcsMiaFault = false;      //The PCS node is mia on HVS CAN
static bool battery2_BmsFault = false;         //Warns if the BMS ECU has faulted
static bool battery2_PcsFault = false;         //Warns if the PCS ECU has faulted
static bool battery2_CpFault = false;          //Warns if the CP ECU has faulted
static bool battery2_ShuntHwMiaFault = false;  //Warns if the shunt current reading is not available
static bool battery2_PyroMiaFault = false;     //Warns if the pyro squib is not connected
static bool battery2_hvsMiaFault = false;      //Warns if the pack contactor hw fault
static bool battery2_hviMiaFault = false;      //Warns if the FC contactor hw fault
static bool battery2_Supply12vFault =
    false;  //Warns if the low voltage (12V) battery is below minimum voltage threshold
static bool battery2_VerSupplyFault =
    false;                               //Warns if the Energy reserve voltage supply is below minimum voltage threshold
static bool battery2_HvilFault = false;  //Warn if a High Voltage Inter Lock fault is detected
static bool battery2_BmsHvsMiaFault = false;  //Warns if the BMS node is mia on HVS or HVI CAN
static bool battery2_PackVoltMismatchFault =
    false;  //Warns if the pack voltage doesn't match approximately with sum of brick voltages
static bool battery2_EnsMiaFault = false;         //Warns if the ENS line is not connected to HVC
static bool battery2_PackPosCtrArcFault = false;  //Warns if the HVP detectes series arc at pack contactor
static bool battery2_packNegCtrArcFault = false;  //Warns if the HVP detectes series arc at FC contactor
static bool battery2_ShuntHwAndBmsMiaFault = false;
static bool battery2_fcContHwFault = false;
static bool battery2_robinOverVoltageFault = false;
static bool battery2_packContHwFault = false;
static bool battery2_pyroFuseBlown = false;
static bool battery2_pyroFuseFailedToBlow = false;
static bool battery2_CpilFault = false;
static bool battery2_PackContactorFellOpen = false;
static bool battery2_FcContactorFellOpen = false;
static bool battery2_packCtrCloseBlocked = false;
static bool battery2_fcCtrCloseBlocked = false;
static bool battery2_packContactorForceOpen = false;
static bool battery2_fcContactorForceOpen = false;
static bool battery2_dcLinkOverVoltage = false;
static bool battery2_shuntOverTemperature = false;
static bool battery2_passivePyroDeploy = false;
static bool battery2_logUploadRequest = false;
static bool battery2_packCtrCloseFailed = false;
static bool battery2_fcCtrCloseFailed = false;
static bool battery2_shuntThermistorMia = false;
//0x320: 800 BMS_alertMatrix
static uint8_t battery2_BMS_matrixIndex = 4;
static bool battery2_BMS_a061_robinBrickOverVoltage = false;
static bool battery2_BMS_a062_SW_BrickV_Imbalance = false;
static bool battery2_BMS_a063_SW_ChargePort_Fault = false;
static bool battery2_BMS_a064_SW_SOC_Imbalance = false;
static bool battery2_BMS_a127_SW_shunt_SNA = false;
static bool battery2_BMS_a128_SW_shunt_MIA = false;
static bool battery2_BMS_a069_SW_Low_Power = false;
static bool battery2_BMS_a130_IO_CAN_Error = false;
static bool battery2_BMS_a071_SW_SM_TransCon_Not_Met = false;
static bool battery2_BMS_a132_HW_BMB_OTP_Uncorrctbl = false;
static bool battery2_BMS_a134_SW_Delayed_Ctr_Off = false;
static bool battery2_BMS_a075_SW_Chg_Disable_Failure = false;
static bool battery2_BMS_a076_SW_Dch_While_Charging = false;
static bool battery2_BMS_a017_SW_Brick_OV = false;
static bool battery2_BMS_a018_SW_Brick_UV = false;
static bool battery2_BMS_a019_SW_Module_OT = false;
static bool battery2_BMS_a021_SW_Dr_Limits_Regulation = false;
static bool battery2_BMS_a022_SW_Over_Current = false;
static bool battery2_BMS_a023_SW_Stack_OV = false;
static bool battery2_BMS_a024_SW_Islanded_Brick = false;
static bool battery2_BMS_a025_SW_PwrBalance_Anomaly = false;
static bool battery2_BMS_a026_SW_HFCurrent_Anomaly = false;
static bool battery2_BMS_a087_SW_Feim_Test_Blocked = false;
static bool battery2_BMS_a088_SW_VcFront_MIA_InDrive = false;
static bool battery2_BMS_a089_SW_VcFront_MIA = false;
static bool battery2_BMS_a090_SW_Gateway_MIA = false;
static bool battery2_BMS_a091_SW_ChargePort_MIA = false;
static bool battery2_BMS_a092_SW_ChargePort_Mia_On_Hv = false;
static bool battery2_BMS_a034_SW_Passive_Isolation = false;
static bool battery2_BMS_a035_SW_Isolation = false;
static bool battery2_BMS_a036_SW_HvpHvilFault = false;
static bool battery2_BMS_a037_SW_Flood_Port_Open = false;
static bool battery2_BMS_a158_SW_HVP_HVI_Comms = false;
static bool battery2_BMS_a039_SW_DC_Link_Over_Voltage = 1;
static bool battery2_BMS_a041_SW_Power_On_Reset = false;
static bool battery2_BMS_a042_SW_MPU_Error = false;
static bool battery2_BMS_a043_SW_Watch_Dog_Reset = false;
static bool battery2_BMS_a044_SW_Assertion = false;
static bool battery2_BMS_a045_SW_Exception = false;
static bool battery2_BMS_a046_SW_Task_Stack_Usage = false;
static bool battery2_BMS_a047_SW_Task_Stack_Overflow = false;
static bool battery2_BMS_a048_SW_Log_Upload_Request = false;
static bool battery2_BMS_a169_SW_FC_Pack_Weld = false;
static bool battery2_BMS_a050_SW_Brick_Voltage_MIA = false;
static bool battery2_BMS_a051_SW_HVC_Vref_Bad = false;
static bool battery2_BMS_a052_SW_PCS_MIA = false;
static bool battery2_BMS_a053_SW_ThermalModel_Sanity = false;
static bool battery2_BMS_a054_SW_Ver_Supply_Fault = false;
static bool battery2_BMS_a176_SW_GracefulPowerOff = false;
static bool battery2_BMS_a059_SW_Pack_Voltage_Sensing = false;
static bool battery2_BMS_a060_SW_Leakage_Test_Failure = false;
static bool battery2_BMS_a077_SW_Charger_Regulation = false;
static bool battery2_BMS_a081_SW_Ctr_Close_Blocked = false;
static bool battery2_BMS_a082_SW_Ctr_Force_Open = false;
static bool battery2_BMS_a083_SW_Ctr_Close_Failure = false;
static bool battery2_BMS_a084_SW_Sleep_Wake_Aborted = false;
static bool battery2_BMS_a094_SW_Drive_Inverter_MIA = false;
static bool battery2_BMS_a099_SW_BMB_Communication = false;
static bool battery2_BMS_a105_SW_One_Module_Tsense = false;
static bool battery2_BMS_a106_SW_All_Module_Tsense = false;
static bool battery2_BMS_a107_SW_Stack_Voltage_MIA = false;
static bool battery2_BMS_a121_SW_NVRAM_Config_Error = false;
static bool battery2_BMS_a122_SW_BMS_Therm_Irrational = false;
static bool battery2_BMS_a123_SW_Internal_Isolation = false;
static bool battery2_BMS_a129_SW_VSH_Failure = false;
static bool battery2_BMS_a131_Bleed_FET_Failure = false;
static bool battery2_BMS_a136_SW_Module_OT_Warning = false;
static bool battery2_BMS_a137_SW_Brick_UV_Warning = false;
static bool battery2_BMS_a138_SW_Brick_OV_Warning = false;
static bool battery2_BMS_a139_SW_DC_Link_V_Irrational = false;
static bool battery2_BMS_a141_SW_BMB_Status_Warning = false;
static bool battery2_BMS_a144_Hvp_Config_Mismatch = false;
static bool battery2_BMS_a145_SW_SOC_Change = false;
static bool battery2_BMS_a146_SW_Brick_Overdischarged = false;
static bool battery2_BMS_a149_SW_Missing_Config_Block = false;
static bool battery2_BMS_a151_SW_external_isolation = false;
static bool battery2_BMS_a156_SW_BMB_Vref_bad = false;
static bool battery2_BMS_a157_SW_HVP_HVS_Comms = false;
static bool battery2_BMS_a159_SW_HVP_ECU_Error = false;
static bool battery2_BMS_a16false_SW_DI_Open_Request = false;
static bool battery2_BMS_a162_SW_No_Power_For_Support = false;
static bool battery2_BMS_a163_SW_Contactor_Mismatch = false;
static bool battery2_BMS_a164_SW_Uncontrolled_Regen = false;
static bool battery2_BMS_a165_SW_Pack_Partial_Weld = false;
static bool battery2_BMS_a166_SW_Pack_Full_Weld = false;
static bool battery2_BMS_a167_SW_FC_Partial_Weld = false;
static bool battery2_BMS_a168_SW_FC_Full_Weld = false;
static bool battery2_BMS_a170_SW_Limp_Mode = false;
static bool battery2_BMS_a171_SW_Stack_Voltage_Sense = false;
static bool battery2_BMS_a174_SW_Charge_Failure = false;
static bool battery2_BMS_a179_SW_Hvp_12V_Fault = false;
static bool battery2_BMS_a180_SW_ECU_reset_blocked = false;

#endif  //DOUBLE_BATTERY

static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                      "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                      "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
static const char* contactorState[] = {"SNA",        "OPEN",       "PRECHARGE",   "BLOCKED",
                                       "PULLED_IN",  "OPENING",    "ECONOMIZED",  "WELDED",
                                       "UNKNOWN(8)", "UNKNOWN(9)", "UNKNOWN(10)", "UNKNOWN(11)"};
static const char* hvilStatusState[] = {"NOT OK",
                                        "STATUS_OK",
                                        "CURRENT_SOURCE_FAULT",
                                        "INTERNAL_OPEN_FAULT",
                                        "VEHICLE_OPEN_FAULT",
                                        "PENTHOUSE_LID_OPEN_FAULT",
                                        "UNKNOWN_LOCATION_OPEN_FAULT",
                                        "VEHICLE_NODE_FAULT",
                                        "NO_12V_SUPPLY",
                                        "VEHICLE_OR_PENTHOUSE_LID_OPENFAULT",
                                        "UNKNOWN(10)",
                                        "UNKNOWN(11)",
                                        "UNKNOWN(12)",
                                        "UNKNOWN(13)",
                                        "UNKNOWN(14)",
                                        "UNKNOWN(15)"};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery.status.soh_pptt = 9900;  //Tesla batteries do not send a SOH% value on bus. Hardcode to 99%

  datalayer.battery.status.real_soc = (battery_soc_ui * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = (battery_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery.status.current_dA = battery_amps;  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  // Define the allowed discharge power
  datalayer.battery.status.max_discharge_power_W = (battery_max_discharge_current * battery_volts);
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery.status.max_discharge_power_W > MAXDISCHARGEPOWERALLOWED) {
    datalayer.battery.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery_soc_ui > 990) {
    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery_soc_ui >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        RAMPDOWNPOWERALLOWED * (1 - (battery_soc_ui - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    } else {  //NCM/A
      if (battery_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
        datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }

  datalayer.battery.status.temperature_min_dC = battery_min_temp;

  datalayer.battery.status.temperature_max_dC = battery_max_temp;

  datalayer.battery.status.cell_max_voltage_mV = battery_cell_max_v;

  datalayer.battery.status.cell_min_voltage_mV = battery_cell_min_v;

  battery_cell_deviation_mV = (battery_cell_max_v - battery_cell_min_v);

  /* Value mapping is completed. Start to check all safeties */

  if (battery_hvil_status ==
      3) {  //INTERNAL_OPEN_FAULT - Someone disconnected a high voltage cable while battery was in use
    set_event(EVENT_INTERNAL_OPEN_FAULT, 0);
  } else {
    clear_event(EVENT_INTERNAL_OPEN_FAULT);
  }

#ifdef TESLA_MODEL_3Y_BATTERY
  // Autodetect algoritm for chemistry on 3/Y packs.
  // NCM/A batteries have 96s, LFP has 102-108s
  // Drawback with this check is that it takes 3-5minutes before all cells have been counted!
  if (datalayer.battery.info.number_of_cells > 101) {
    datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  }

  //Once cell chemistry is determined, set maximum and minimum total pack voltage safety limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
  } else {  // NCM/A chemistry
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
  }

  // During forced balancing request via webserver, we allow the battery to exceed normal safety parameters
  if (datalayer.battery.settings.user_requests_balancing) {
    datalayer.battery.status.real_soc = 9900;  //Force battery to show up as 99% when balancing
    datalayer.battery.info.max_design_voltage_dV = datalayer.battery.settings.balancing_max_pack_voltage_dV;
    datalayer.battery.info.max_cell_voltage_mV = datalayer.battery.settings.balancing_max_cell_voltage_mV;
    datalayer.battery.info.max_cell_voltage_deviation_mV =
        datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV;
    datalayer.battery.status.max_charge_power_W = datalayer.battery.settings.balancing_float_power_W;
  }
#endif  // TESLA_MODEL_3Y_BATTERY

  // Update webserver datalayer
  //0x20A
  datalayer_extended.tesla.status_contactor = battery_contactor;
  datalayer_extended.tesla.hvil_status = battery_hvil_status;
  datalayer_extended.tesla.packContNegativeState = battery_packContNegativeState;
  datalayer_extended.tesla.packContPositiveState = battery_packContPositiveState;
  datalayer_extended.tesla.packContactorSetState = battery_packContactorSetState;
  datalayer_extended.tesla.packCtrsClosingAllowed = battery_packCtrsClosingAllowed;
  datalayer_extended.tesla.pyroTestInProgress = battery_pyroTestInProgress;
  datalayer_extended.tesla.battery_packCtrsOpenNowRequested = battery_packCtrsOpenNowRequested;
  datalayer_extended.tesla.battery_packCtrsOpenRequested = battery_packCtrsOpenRequested;
  datalayer_extended.tesla.battery_packCtrsRequestStatus = battery_packCtrsRequestStatus;
  datalayer_extended.tesla.battery_packCtrsResetRequestRequired = battery_packCtrsResetRequestRequired;
  datalayer_extended.tesla.battery_dcLinkAllowedToEnergize = battery_dcLinkAllowedToEnergize;
  //0x2B4
  datalayer_extended.tesla.battery_dcdcLvBusVolt = battery_dcdcLvBusVolt;
  datalayer_extended.tesla.battery_dcdcHvBusVolt = battery_dcdcHvBusVolt;
  datalayer_extended.tesla.battery_dcdcLvOutputCurrent = battery_dcdcLvOutputCurrent;
  //0x352
  datalayer_extended.tesla.battery_nominal_full_pack_energy = battery_nominal_full_pack_energy;
  datalayer_extended.tesla.battery_nominal_full_pack_energy_m0 = battery_nominal_full_pack_energy_m0;
  datalayer_extended.tesla.battery_nominal_energy_remaining = battery_nominal_energy_remaining;
  datalayer_extended.tesla.battery_nominal_energy_remaining_m0 = battery_nominal_energy_remaining_m0;
  datalayer_extended.tesla.battery_ideal_energy_remaining = battery_ideal_energy_remaining;
  datalayer_extended.tesla.battery_ideal_energy_remaining_m0 = battery_ideal_energy_remaining_m0;
  datalayer_extended.tesla.battery_energy_to_charge_complete = battery_energy_to_charge_complete;
  datalayer_extended.tesla.battery_energy_to_charge_complete_m1 = battery_energy_to_charge_complete_m1;
  datalayer_extended.tesla.battery_energy_buffer = battery_energy_buffer;
  datalayer_extended.tesla.battery_energy_buffer_m1 = battery_energy_buffer_m1;
  datalayer_extended.tesla.battery_expected_energy_remaining_m1 = battery_expected_energy_remaining_m1;
  datalayer_extended.tesla.battery_full_charge_complete = battery_full_charge_complete;
  datalayer_extended.tesla.battery_fully_charged = battery_fully_charged;
  //0x3D2
  datalayer_extended.tesla.battery_total_discharge = battery_total_discharge;
  datalayer_extended.tesla.battery_total_charge = battery_total_charge;
  //0x392
  datalayer_extended.tesla.battery_moduleType = battery_moduleType;
  datalayer_extended.tesla.battery_packMass = battery_packMass;
  datalayer_extended.tesla.battery_platformMaxBusVoltage = battery_platformMaxBusVoltage;
  //0x2D2
  datalayer_extended.tesla.battery_bms_min_voltage = battery_bms_min_voltage;
  datalayer_extended.tesla.battery_bms_max_voltage = battery_bms_max_voltage;
  datalayer_extended.tesla.battery_max_charge_current = battery_max_charge_current;
  datalayer_extended.tesla.battery_max_discharge_current = battery_max_discharge_current;
  //0x292
  datalayer_extended.tesla.battery_beginning_of_life = battery_beginning_of_life;
  datalayer_extended.tesla.battery_battTempPct = battery_battTempPct;
  datalayer_extended.tesla.battery_soc_ave = battery_soc_ave;
  datalayer_extended.tesla.battery_soc_max = battery_soc_max;
  datalayer_extended.tesla.battery_soc_min = battery_soc_min;
  datalayer_extended.tesla.battery_soc_ui = battery_soc_ui;
  //0x332
  datalayer_extended.tesla.battery_BrickVoltageMax = battery_BrickVoltageMax;
  datalayer_extended.tesla.battery_BrickVoltageMin = battery_BrickVoltageMin;
  datalayer_extended.tesla.battery_BrickTempMaxNum = battery_BrickTempMaxNum;
  datalayer_extended.tesla.battery_BrickTempMinNum = battery_BrickTempMinNum;
  datalayer_extended.tesla.battery_BrickModelTMax = battery_BrickModelTMax;
  datalayer_extended.tesla.battery_BrickModelTMin = battery_BrickModelTMin;
  //0x212
  datalayer_extended.tesla.battery_BMS_isolationResistance = battery_BMS_isolationResistance;
  datalayer_extended.tesla.battery_BMS_contactorState = battery_BMS_contactorState;
  datalayer_extended.tesla.battery_BMS_state = battery_BMS_state;
  datalayer_extended.tesla.battery_BMS_hvState = battery_BMS_hvState;
  datalayer_extended.tesla.battery_BMS_uiChargeStatus = battery_BMS_uiChargeStatus;
  datalayer_extended.tesla.battery_BMS_diLimpRequest = battery_BMS_diLimpRequest;
  datalayer_extended.tesla.battery_BMS_chgPowerAvailable = battery_BMS_chgPowerAvailable;
  datalayer_extended.tesla.battery_BMS_pcsPwmEnabled = battery_BMS_pcsPwmEnabled;
  //0x224
  datalayer_extended.tesla.battery_PCS_dcdcPrechargeStatus = battery_PCS_dcdcPrechargeStatus;
  datalayer_extended.tesla.battery_PCS_dcdc12VSupportStatus = battery_PCS_dcdc12VSupportStatus;
  datalayer_extended.tesla.battery_PCS_dcdcHvBusDischargeStatus = battery_PCS_dcdcHvBusDischargeStatus;
  datalayer_extended.tesla.battery_PCS_dcdcMainState = battery_PCS_dcdcMainState;
  datalayer_extended.tesla.battery_PCS_dcdcSubState = battery_PCS_dcdcSubState;
  datalayer_extended.tesla.battery_PCS_dcdcFaulted = battery_PCS_dcdcFaulted;
  datalayer_extended.tesla.battery_PCS_dcdcOutputIsLimited = battery_PCS_dcdcOutputIsLimited;
  datalayer_extended.tesla.battery_PCS_dcdcMaxOutputCurrentAllowed = battery_PCS_dcdcMaxOutputCurrentAllowed;
  datalayer_extended.tesla.battery_PCS_dcdcPrechargeRtyCnt = battery_PCS_dcdcPrechargeRtyCnt;
  datalayer_extended.tesla.battery_PCS_dcdc12VSupportRtyCnt = battery_PCS_dcdc12VSupportRtyCnt;
  datalayer_extended.tesla.battery_PCS_dcdcDischargeRtyCnt = battery_PCS_dcdcDischargeRtyCnt;
  datalayer_extended.tesla.battery_PCS_dcdcPwmEnableLine = battery_PCS_dcdcPwmEnableLine;
  datalayer_extended.tesla.battery_PCS_dcdcSupportingFixedLvTarget = battery_PCS_dcdcSupportingFixedLvTarget;
  datalayer_extended.tesla.battery_PCS_dcdcPrechargeRestartCnt = battery_PCS_dcdcPrechargeRestartCnt;
  datalayer_extended.tesla.battery_PCS_dcdcInitialPrechargeSubState = battery_PCS_dcdcInitialPrechargeSubState;
  //0x252
  datalayer_extended.tesla.BMS_maxRegenPower = BMS_maxRegenPower;
  datalayer_extended.tesla.BMS_maxDischargePower = BMS_maxDischargePower;
  datalayer_extended.tesla.BMS_maxStationaryHeatPower = BMS_maxStationaryHeatPower;
  datalayer_extended.tesla.BMS_hvacPowerBudget = BMS_hvacPowerBudget;
  datalayer_extended.tesla.BMS_notEnoughPowerForHeatPump = BMS_notEnoughPowerForHeatPump;
  datalayer_extended.tesla.BMS_powerLimitState = BMS_powerLimitState;
  datalayer_extended.tesla.BMS_inverterTQF = BMS_inverterTQF;
  //0x312
  datalayer_extended.tesla.BMS_powerDissipation = BMS_powerDissipation;
  datalayer_extended.tesla.BMS_flowRequest = BMS_flowRequest;
  datalayer_extended.tesla.BMS_inletActiveCoolTargetT = BMS_inletActiveCoolTargetT;
  datalayer_extended.tesla.BMS_inletPassiveTargetT = BMS_inletPassiveTargetT;
  datalayer_extended.tesla.BMS_inletActiveHeatTargetT = BMS_inletActiveHeatTargetT;
  datalayer_extended.tesla.BMS_packTMin = BMS_packTMin;
  datalayer_extended.tesla.BMS_packTMax = BMS_packTMax;
  datalayer_extended.tesla.BMS_pcsNoFlowRequest = BMS_pcsNoFlowRequest;
  datalayer_extended.tesla.BMS_noFlowRequest = BMS_noFlowRequest;
  //0x2A4
  datalayer_extended.tesla.PCS_dcdcTemp = PCS_dcdcTemp;
  datalayer_extended.tesla.PCS_ambientTemp = PCS_ambientTemp;
  //0x2C4
  datalayer_extended.tesla.PCS_dcdcMaxLvOutputCurrent = PCS_dcdcMaxLvOutputCurrent;
  datalayer_extended.tesla.PCS_dcdcCurrentLimit = PCS_dcdcCurrentLimit;
  datalayer_extended.tesla.PCS_dcdcLvOutputCurrentTempLimit = PCS_dcdcLvOutputCurrentTempLimit;
  datalayer_extended.tesla.PCS_dcdcUnifiedCommand = PCS_dcdcUnifiedCommand;
  datalayer_extended.tesla.PCS_dcdcCLAControllerOutput = PCS_dcdcCLAControllerOutput;
  datalayer_extended.tesla.PCS_dcdcTankVoltage = PCS_dcdcTankVoltage;
  datalayer_extended.tesla.PCS_dcdcTankVoltageTarget = PCS_dcdcTankVoltageTarget;
  datalayer_extended.tesla.PCS_dcdcClaCurrentFreq = PCS_dcdcClaCurrentFreq;
  datalayer_extended.tesla.PCS_dcdcTCommMeasured = PCS_dcdcTCommMeasured;
  datalayer_extended.tesla.PCS_dcdcShortTimeUs = PCS_dcdcShortTimeUs;
  datalayer_extended.tesla.PCS_dcdcHalfPeriodUs = PCS_dcdcHalfPeriodUs;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxFrequency = PCS_dcdcIntervalMaxFrequency;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxHvBusVolt = PCS_dcdcIntervalMaxHvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxLvBusVolt = PCS_dcdcIntervalMaxLvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMaxLvOutputCurr = PCS_dcdcIntervalMaxLvOutputCurr;
  datalayer_extended.tesla.PCS_dcdcIntervalMinFrequency = PCS_dcdcIntervalMinFrequency;
  datalayer_extended.tesla.PCS_dcdcIntervalMinHvBusVolt = PCS_dcdcIntervalMinHvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMinLvBusVolt = PCS_dcdcIntervalMinLvBusVolt;
  datalayer_extended.tesla.PCS_dcdcIntervalMinLvOutputCurr = PCS_dcdcIntervalMinLvOutputCurr;
  datalayer_extended.tesla.PCS_dcdc12vSupportLifetimekWh = PCS_dcdc12vSupportLifetimekWh;
  //0x7AA
  datalayer_extended.tesla.HVP_gpioPassivePyroDepl = HVP_gpioPassivePyroDepl;
  datalayer_extended.tesla.HVP_gpioPyroIsoEn = HVP_gpioPyroIsoEn;
  datalayer_extended.tesla.HVP_gpioCpFaultIn = HVP_gpioCpFaultIn;
  datalayer_extended.tesla.HVP_gpioPackContPowerEn = HVP_gpioPackContPowerEn;
  datalayer_extended.tesla.HVP_gpioHvCablesOk = HVP_gpioHvCablesOk;
  datalayer_extended.tesla.HVP_gpioHvpSelfEnable = HVP_gpioHvpSelfEnable;
  datalayer_extended.tesla.HVP_gpioLed = HVP_gpioLed;
  datalayer_extended.tesla.HVP_gpioCrashSignal = HVP_gpioCrashSignal;
  datalayer_extended.tesla.HVP_gpioShuntDataReady = HVP_gpioShuntDataReady;
  datalayer_extended.tesla.HVP_gpioFcContPosAux = HVP_gpioFcContPosAux;
  datalayer_extended.tesla.HVP_gpioFcContNegAux = HVP_gpioFcContNegAux;
  datalayer_extended.tesla.HVP_gpioBmsEout = HVP_gpioBmsEout;
  datalayer_extended.tesla.HVP_gpioCpFaultOut = HVP_gpioCpFaultOut;
  datalayer_extended.tesla.HVP_gpioPyroPor = HVP_gpioPyroPor;
  datalayer_extended.tesla.HVP_gpioShuntEn = HVP_gpioShuntEn;
  datalayer_extended.tesla.HVP_gpioHvpVerEn = HVP_gpioHvpVerEn;
  datalayer_extended.tesla.HVP_gpioPackCoontPosFlywheel = HVP_gpioPackCoontPosFlywheel;
  datalayer_extended.tesla.HVP_gpioCpLatchEnable = HVP_gpioCpLatchEnable;
  datalayer_extended.tesla.HVP_gpioPcsEnable = HVP_gpioPcsEnable;
  datalayer_extended.tesla.HVP_gpioPcsDcdcPwmEnable = HVP_gpioPcsDcdcPwmEnable;
  datalayer_extended.tesla.HVP_gpioPcsChargePwmEnable = HVP_gpioPcsChargePwmEnable;
  datalayer_extended.tesla.HVP_gpioFcContPowerEnable = HVP_gpioFcContPowerEnable;
  datalayer_extended.tesla.HVP_gpioHvilEnable = HVP_gpioHvilEnable;
  datalayer_extended.tesla.HVP_gpioSecDrdy = HVP_gpioSecDrdy;
  datalayer_extended.tesla.HVP_hvp1v5Ref = HVP_hvp1v5Ref;
  datalayer_extended.tesla.HVP_shuntCurrentDebug = HVP_shuntCurrentDebug;
  datalayer_extended.tesla.HVP_packCurrentMia = HVP_packCurrentMia;
  datalayer_extended.tesla.HVP_auxCurrentMia = HVP_auxCurrentMia;
  datalayer_extended.tesla.HVP_currentSenseMia = HVP_currentSenseMia;
  datalayer_extended.tesla.HVP_shuntRefVoltageMismatch = HVP_shuntRefVoltageMismatch;
  datalayer_extended.tesla.HVP_shuntThermistorMia = HVP_shuntThermistorMia;
  datalayer_extended.tesla.HVP_shuntHwMia = HVP_shuntHwMia;
  datalayer_extended.tesla.HVP_dcLinkVoltage = HVP_dcLinkVoltage;
  datalayer_extended.tesla.HVP_packVoltage = HVP_packVoltage;
  datalayer_extended.tesla.HVP_fcLinkVoltage = HVP_fcLinkVoltage;
  datalayer_extended.tesla.HVP_packContVoltage = HVP_packContVoltage;
  datalayer_extended.tesla.HVP_packNegativeV = HVP_packNegativeV;
  datalayer_extended.tesla.HVP_packPositiveV = HVP_packPositiveV;
  datalayer_extended.tesla.HVP_pyroAnalog = HVP_pyroAnalog;
  datalayer_extended.tesla.HVP_dcLinkNegativeV = HVP_dcLinkNegativeV;
  datalayer_extended.tesla.HVP_dcLinkPositiveV = HVP_dcLinkPositiveV;
  datalayer_extended.tesla.HVP_fcLinkNegativeV = HVP_fcLinkNegativeV;
  datalayer_extended.tesla.HVP_fcContCoilCurrent = HVP_fcContCoilCurrent;
  datalayer_extended.tesla.HVP_fcContVoltage = HVP_fcContVoltage;
  datalayer_extended.tesla.HVP_hvilInVoltage = HVP_hvilInVoltage;
  datalayer_extended.tesla.HVP_hvilOutVoltage = HVP_hvilOutVoltage;
  datalayer_extended.tesla.HVP_fcLinkPositiveV = HVP_fcLinkPositiveV;
  datalayer_extended.tesla.HVP_packContCoilCurrent = HVP_packContCoilCurrent;
  datalayer_extended.tesla.HVP_battery12V = HVP_battery12V;
  datalayer_extended.tesla.HVP_shuntRefVoltageDbg = HVP_shuntRefVoltageDbg;
  datalayer_extended.tesla.HVP_shuntAuxCurrentDbg = HVP_shuntAuxCurrentDbg;
  datalayer_extended.tesla.HVP_shuntBarTempDbg = HVP_shuntBarTempDbg;
  datalayer_extended.tesla.HVP_shuntAsicTempDbg = HVP_shuntAsicTempDbg;
  datalayer_extended.tesla.HVP_shuntAuxCurrentStatus = HVP_shuntAuxCurrentStatus;
  datalayer_extended.tesla.HVP_shuntBarTempStatus = HVP_shuntBarTempStatus;
  datalayer_extended.tesla.HVP_shuntAsicTempStatus = HVP_shuntAsicTempStatus;

#ifdef DEBUG_LOG

  printFaultCodesIfActive();

  logging.print("STATUS: Contactor: ");
  logging.print(contactorText[battery_contactor]);  //Display what state the contactor is in
  logging.print(", HVIL: ");
  logging.print(hvilStatusState[battery_hvil_status]);
  logging.print(", NegativeState: ");
  logging.print(contactorState[battery_packContNegativeState]);
  logging.print(", PositiveState: ");
  logging.print(contactorState[battery_packContPositiveState]);
  logging.print(", setState: ");
  logging.print(contactorState[battery_packContactorSetState]);
  logging.print(", close allowed: ");
  logging.print(battery_packCtrsClosingAllowed);
  logging.print(", Pyrotest: ");
  logging.println(battery_pyroTestInProgress);

  logging.print("Battery values: ");
  logging.print("Real SOC: ");
  logging.print(battery_soc_ui / 10.0, 1);
  print_int_with_units(", Battery voltage: ", battery_volts, "V");
  print_int_with_units(", Battery HV current: ", (battery_amps * 0.1), "A");
  logging.print(", Fully charged?: ");
  if (battery_full_charge_complete)
    logging.print("YES, ");
  else
    logging.print("NO, ");
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    logging.print("LFP chemistry detected!");
  }
  logging.println("");
  logging.print("Cellstats, Max: ");
  logging.print(battery_cell_max_v);
  logging.print("mV (cell ");
  logging.print(battery_BrickVoltageMaxNum);
  logging.print("), Min: ");
  logging.print(battery_cell_min_v);
  logging.print("mV (cell ");
  logging.print(battery_BrickVoltageMinNum);
  logging.print("), Imbalance: ");
  logging.print(battery_cell_deviation_mV);
  logging.println("mV.");

  print_int_with_units("High Voltage Output Pins: ", battery_dcdcHvBusVolt, "V");
  logging.print(", ");
  print_int_with_units("Low Voltage: ", battery_dcdcLvBusVolt, "V");
  logging.println("");
  print_int_with_units("DC/DC 12V current: ", battery_dcdcLvOutputCurrent, "A");
  logging.println("");

  logging.println("Values passed to the inverter: ");
  print_SOC(" SOC: ", datalayer.battery.status.reported_soc);
  print_int_with_units(" Max discharge power: ", datalayer.battery.status.max_discharge_power_W, "W");
  logging.print(", ");
  print_int_with_units(" Max charge power: ", datalayer.battery.status.max_charge_power_W, "W");
  logging.println("");
  print_int_with_units(" Max temperature: ", ((int16_t)datalayer.battery.status.temperature_min_dC * 0.1), "°C");
  logging.print(", ");
  print_int_with_units(" Min temperature: ", ((int16_t)datalayer.battery.status.temperature_max_dC * 0.1), "°C");
  logging.println("");
#endif  //DEBUG_LOG
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  static uint16_t temp = 0;

  switch (rx_frame.ID) {
    case 0x352:                            // 850 BMS_energyStatus newer BMS
      mux = (rx_frame.data.u8[0] & 0x02);  //BMS_energyStatusIndex M : 0|2@1+ (1,0) [0|0] ""  X

      if (mux == 0) {
        battery_nominal_full_pack_energy_m0 =
            ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);  //16|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_nominal_energy_remaining_m0 =
            ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);  //32|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_ideal_energy_remaining_m0 =
            ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);  //48|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
      }
      if (mux == 1) {
        battery_fully_charged = (rx_frame.data.u8[1] & 0x01);  //15|1@1+ (1,0) [0|1]//to datalayer_extended
        battery_energy_buffer_m1 =
            (rx_frame.data.u8[3] | rx_frame.data.u8[2]);  //16|16@1+ (0.01,0) [0|0] "kWh"//to datalayer_extended
        battery_expected_energy_remaining_m1 =
            (rx_frame.data.u8[5] | rx_frame.data.u8[4]);  //32|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
        battery_energy_to_charge_complete_m1 =
            (rx_frame.data.u8[7] | rx_frame.data.u8[6]);  //48|16@1+ (0.02,0) [0|0] "kWh"//to datalayer_extended
      }
      if (mux == 2) {}
      // Additional information needed on this mux, example frame: 02 26 02 20 02 80 00 00 doesn't change
      // older BMS <2021 without mux
      battery_nominal_full_pack_energy =  //BMS_nominalFullPackEnergy : 0|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[1] & (0x07U)) << 8) | (_d[0] & (0xFFU));
          (((rx_frame.data.u8[1] & 0x07) << 8) | (rx_frame.data.u8[0]));  //Example 752 (75.2kWh)
      battery_nominal_energy_remaining =  //BMS_nominalEnergyRemaining : 11|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[2] & (0x3FU)) << 5) | ((_d[1] >> 3) & (0x1FU));
          (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0x1F) >> 3));  //Example 1247 * 0.1 = 124.7kWh
      battery_expected_energy_remaining =  //BMS_expectedEnergyRemaining : 22|11@1+ (0.1,0) [0|204.6] "KWh"// ((_d[4] & (0x01U)) << 10) | ((_d[3] & (0xFFU)) << 2) | ((_d[2] >> 6) & (0x03U));
          (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) |
           ((rx_frame.data.u8[2] & 0x03) >> 6));  //Example 622 (62.2kWh)
      battery_ideal_energy_remaining =  //BMS_idealEnergyRemaining : 33|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[5] & (0x0FU)) << 7) | ((_d[4] >> 1) & (0x7FU));
          (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0x7F) >> 1));  //Example 311 * 0.1 = 31.1kWh
      battery_energy_to_charge_complete =  // BMS_energyToChargeComplete : 44|11@1+ (0.1,0) [0|204.6] "KWh"// ((_d[6] & (0x7FU)) << 4) | ((_d[5] >> 4) & (0x0FU));
          (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0x0F) << 4));  //Example 147 * 0.1 = 14.7kWh
      battery_energy_buffer =  //BMS_energyBuffer : 55|8@1+ (0.1,0) [0|25.4] "KWh"// ((_d[7] & (0x7FU)) << 1) | ((_d[6] >> 7) & (0x01U));
          (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x01) >> 7));  //Example 1 * 0.1 = 0
      battery_full_charge_complete =  //BMS_fullChargeComplete : 63|1@1+ (1,0) [0|1] ""//((_d[7] >> 7) & (0x01U));
          ((rx_frame.data.u8[7] & 0x01) >> 7);
      break;
    case 0x20A:  //522 HVP_contactorState:
      battery_packContNegativeState =
          (rx_frame.data.u8[0] & 0x07);  //(_d[0] & (0x07U)); 0|3@1+ (1,0) [0|7] //to datalayer_extended
      battery_packContPositiveState =
          (rx_frame.data.u8[0] & 0x38) >> 3;  //((_d[0] >> 3) & (0x07U)); 3|3@1+ (1,0) [0|7] //to datalayer_extended
      battery_contactor = (rx_frame.data.u8[1] & 0x0F);  // 8|4@1+ (1,0) [0|9] //to datalayer_extended
      battery_packContactorSetState =
          (rx_frame.data.u8[1] & 0x0F);  //(_d[1] & (0x0FU)); 8|4@1+ (1,0) [0|9] //to datalayer_extended
      battery_packCtrsClosingAllowed =
          (rx_frame.data.u8[4] & 0x08) >> 3;  //((_d[4] >> 3) & (0x01U)); 35|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_pyroTestInProgress =
          (rx_frame.data.u8[4] & 0x20) >> 5;  //((_d[4] >> 5) & (0x01U));//37|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_hvil_status =
          (rx_frame.data.u8[5] & 0x0F);  //(_d[5] & (0x0FU));   //40|4@1+ (1,0) [0|9] //to datalayer_extended
      battery_packCtrsOpenNowRequested =
          ((rx_frame.data.u8[4] >> 1) & (0x01U));  //33|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_packCtrsOpenRequested =
          ((rx_frame.data.u8[4] >> 2) & (0x01U));  //34|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_packCtrsRequestStatus =
          ((rx_frame.data.u8[3] >> 6) & (0x03U));  //30|2@1+ (1,0) [0|2] //to datalayer_extended
      battery_packCtrsResetRequestRequired =
          (rx_frame.data.u8[4] & (0x01U));  //32|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_dcLinkAllowedToEnergize =
          ((rx_frame.data.u8[4] >> 4) & (0x01U));  //36|1@1+ (1,0) [0|1] //to datalayer_extended
      battery_fcContNegativeAuxOpen = ((rx_frame.data.u8[0] >> 7) & (0x01U));       //7|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcContNegativeState = ((rx_frame.data.u8[1] >> 4) & (0x07U));         //12|3@1+ (1,0) [0|7] ""  Receiver
      battery_fcContPositiveAuxOpen = ((rx_frame.data.u8[0] >> 6) & (0x01U));       //6|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcContPositiveState = (rx_frame.data.u8[2] & (0x07U));                //16|3@1+ (1,0) [0|7] ""  Receiver
      battery_fcContactorSetState = ((rx_frame.data.u8[2] >> 3) & (0x0FU));         //19|4@1+ (1,0) [0|9] ""  Receiver
      battery_fcCtrsClosingAllowed = ((rx_frame.data.u8[3] >> 5) & (0x01U));        //29|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsOpenNowRequested = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsOpenRequested = ((rx_frame.data.u8[3] >> 4) & (0x01U));         //28|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcCtrsRequestStatus = (rx_frame.data.u8[3] & (0x03U));                //24|2@1+ (1,0) [0|2] ""  Receiver
      battery_fcCtrsResetRequestRequired = ((rx_frame.data.u8[3] >> 2) & (0x01U));  //26|1@1+ (1,0) [0|1] ""  Receiver
      battery_fcLinkAllowedToEnergize = ((rx_frame.data.u8[5] >> 4) & (0x03U));     //44|2@1+ (1,0) [0|2] ""  Receiver
      break;
    case 0x212:  //530 BMS_status: 8
      battery_BMS_hvacPowerRequest = (rx_frame.data.u8[0] & (0x01U));
      battery_BMS_notEnoughPowerForDrive = ((rx_frame.data.u8[0] >> 1) & (0x01U));
      battery_BMS_notEnoughPowerForSupport = ((rx_frame.data.u8[0] >> 2) & (0x01U));
      battery_BMS_preconditionAllowed = ((rx_frame.data.u8[0] >> 3) & (0x01U));
      battery_BMS_updateAllowed = ((rx_frame.data.u8[0] >> 4) & (0x01U));
      battery_BMS_activeHeatingWorthwhile = ((rx_frame.data.u8[0] >> 5) & (0x01U));
      battery_BMS_cpMiaOnHvs = ((rx_frame.data.u8[0] >> 6) & (0x01U));
      battery_BMS_contactorState =
          (rx_frame.data.u8[1] &
           (0x07U));  //0 "SNA" 1 "OPEN" 2 "OPENING" 3 "CLOSING" 4 "CLOSED" 5 "WELDED" 6 "BLOCKED" ;
      battery_BMS_state =
          ((rx_frame.data.u8[1] >> 3) &
           (0x0FU));  //0 "STANDBY" 1 "DRIVE" 2 "SUPPORT" 3 "CHARGE" 4 "FEIM" 5 "CLEAR_FAULT" 6 "FAULT" 7 "WELD" 8 "TEST" 9 "SNA" ;
      battery_BMS_hvState =
          (rx_frame.data.u8[2] &
           (0x07U));  //0 "DOWN" 1 "COMING_UP" 2 "GOING_DOWN" 3 "UP_FOR_DRIVE" 4 "UP_FOR_CHARGE" 5 "UP_FOR_DC_CHARGE" 6 "UP" ;
      battery_BMS_isolationResistance =
          ((rx_frame.data.u8[3] & (0x1FU)) << 5) |
          ((rx_frame.data.u8[2] >> 3) & (0x1FU));  //19|10@1+ (10,0) [0|0] "kOhm"/to datalayer_extended
      battery_BMS_chargeRequest = ((rx_frame.data.u8[3] >> 5) & (0x01U));
      battery_BMS_keepWarmRequest = ((rx_frame.data.u8[3] >> 6) & (0x01U));
      battery_BMS_uiChargeStatus =
          (rx_frame.data.u8[4] &
           (0x07U));  // 0 "DISCONNECTED" 1 "NO_POWER" 2 "ABOUT_TO_CHARGE" 3 "CHARGING" 4 "CHARGE_COMPLETE" 5 "CHARGE_STOPPED" ;
      battery_BMS_diLimpRequest = ((rx_frame.data.u8[4] >> 3) & (0x01U));
      battery_BMS_okToShipByAir = ((rx_frame.data.u8[4] >> 4) & (0x01U));
      battery_BMS_okToShipByLand = ((rx_frame.data.u8[4] >> 5) & (0x01U));
      battery_BMS_chgPowerAvailable = ((rx_frame.data.u8[6] & (0x01U)) << 10) | ((rx_frame.data.u8[5] & (0xFFU)) << 2) |
                                      ((rx_frame.data.u8[4] >> 6) & (0x03U));  //38|11@1+ (0.125,0) [0|0] "kW"
      battery_BMS_chargeRetryCount = ((rx_frame.data.u8[6] >> 1) & (0x0FU));
      battery_BMS_pcsPwmEnabled = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      battery_BMS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      battery_BMS_minPackTemperature = (rx_frame.data.u8[7] & (0xFFU));  //56|8@1+ (0.5,-40) [0|0] "DegC
      break;
    case 0x224:                                                                   //548 PCS_dcdcStatus:
      battery_PCS_dcdcPrechargeStatus = (rx_frame.data.u8[0] & (0x03U));          //0 "IDLE" 1 "ACTIVE" 2 "FAULTED" ;
      battery_PCS_dcdc12VSupportStatus = ((rx_frame.data.u8[0] >> 2) & (0x03U));  //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      battery_PCS_dcdcHvBusDischargeStatus = ((rx_frame.data.u8[0] >> 4) & (0x03U));  //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      battery_PCS_dcdcMainState =
          ((rx_frame.data.u8[1] & (0x03U)) << 2) |
          ((rx_frame.data.u8[0] >> 6) &
           (0x03U));  //0 "STANDBY" 1 "12V_SUPPORT_ACTIVE" 2 "PRECHARGE_STARTUP" 3 "PRECHARGE_ACTIVE" 4 "DIS_HVBUS_ACTIVE" 5 "SHUTDOWN" 6 "FAULTED" ;
      battery_PCS_dcdcSubState =
          ((rx_frame.data.u8[1] >> 2) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      battery_PCS_dcdcFaulted = ((rx_frame.data.u8[1] >> 7) & (0x01U));
      battery_PCS_dcdcOutputIsLimited = ((rx_frame.data.u8[3] >> 4) & (0x01U));
      battery_PCS_dcdcMaxOutputCurrentAllowed = ((rx_frame.data.u8[5] & (0x01U)) << 11) |
                                                ((rx_frame.data.u8[4] & (0xFFU)) << 3) |
                                                ((rx_frame.data.u8[3] >> 5) & (0x07U));  //29|12@1+ (0.1,0) [0|0] "A"
      battery_PCS_dcdcPrechargeRtyCnt = ((rx_frame.data.u8[5] >> 1) & (0x07U));
      battery_PCS_dcdc12VSupportRtyCnt = ((rx_frame.data.u8[5] >> 4) & (0x0FU));
      battery_PCS_dcdcDischargeRtyCnt = (rx_frame.data.u8[6] & (0x0FU));
      battery_PCS_dcdcPwmEnableLine = ((rx_frame.data.u8[6] >> 4) & (0x01U));
      battery_PCS_dcdcSupportingFixedLvTarget = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      battery_PCS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      battery_PCS_dcdcPrechargeRestartCnt = (rx_frame.data.u8[7] & (0x07U));
      battery_PCS_dcdcInitialPrechargeSubState =
          ((rx_frame.data.u8[7] >> 3) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      break;
    case 0x252:  //Limit //594 BMS_powerAvailable:
      BMS_maxRegenPower = ((rx_frame.data.u8[1] << 8) |
                           rx_frame.data.u8[0]);  //0|16@1+ (0.01,0) [0|655.35] "kW"  //Example 4715 * 0.01 = 47.15kW
      BMS_maxDischargePower =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8[2]);  //16|16@1+ (0.013,0) [0|655.35] "kW"  //Example 2009 * 0.013 = 26.117???
      BMS_maxStationaryHeatPower =
          (((rx_frame.data.u8[5] & 0x03) << 8) |
           rx_frame.data.u8[4]);  //32|10@1+ (0.01,0) [0|10.23] "kW"  //Example 500 * 0.01 = 5kW
      BMS_hvacPowerBudget =
          (((rx_frame.data.u8[7] << 6) |
            ((rx_frame.data.u8[6] & 0xFC) >> 2)));  //50|10@1+ (0.02,0) [0|20.46] "kW"  //Example 1000 * 0.02 = 20kW?
      BMS_notEnoughPowerForHeatPump =
          ((rx_frame.data.u8[5] >> 2) & (0x01U));  //BMS_notEnoughPowerForHeatPump : 42|1@1+ (1,0) [0|1] ""  Receiver
      BMS_powerLimitState =
          (rx_frame.data.u8[6] &
           (0x01U));  //BMS_powerLimitsState : 48|1@1+ (1,0) [0|1] 0 "NOT_CALCULATED_FOR_DRIVE" 1 "CALCULATED_FOR_DRIVE"
      BMS_inverterTQF = ((rx_frame.data.u8[7] >> 4) & (0x03U));
      break;
    case 0x132:  //battery amps/volts //HVBattAmpVolt
      battery_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                      0.01;  //0|16@1+ (0.01,0) [0|655.35] "V"  //Example 37030mv * 0.01 = 370V
      battery_amps =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8
               [2]);  //SmoothBattCurrent : 16|16@1- (-0.1,0) [-3276.7|3276.7] "A"//Example 65492 (-4.3A) OR 225 (22.5A)
      battery_raw_amps =
          ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05 +
          822;  //RawBattCurrent : 32|16@1- (-0.05,822) [-1138.35|2138.4] "A"  //Example 10425 * -0.05 = ?
      battery_charge_time_remaining =
          (((rx_frame.data.u8[7] & 0x0F) << 8) |
           rx_frame.data.u8[6]);  //ChargeHoursRemaining : 48|12@1+ (1,0) [0|4095] "Min"  //Example 228 * 0.1 = 22.8min
      if (battery_charge_time_remaining == 4095) {
        battery_charge_time_remaining = 0;
      }
      break;
    case 0x3D2:  //TotalChargeDischarge:
      battery_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                 (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                                0.001;  //0|32@1+ (0.001,0) [0|4294970] "kWh"
      battery_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                              rx_frame.data.u8[4]) *
                             0.001;  //32|32@1+ (0.001,0) [0|4294970] "kWh"
      break;
    case 0x332:                            //min/max hist values //BattBrickMinMax:
      mux = (rx_frame.data.u8[0] & 0x03);  //BattBrickMultiplexer M : 0|2@1+ (1,0) [0|0] ""

      if (mux == 1)  //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        battery_cell_max_v = temp * 2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        battery_cell_min_v = temp * 2;
        cellvoltagesRead = true;
        //BattBrickVoltageMax m1 : 2|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[1] & (0x3FU)) << 6) | ((_d[0] >> 2) & (0x3FU));
        battery_BrickVoltageMax =
            ((rx_frame.data.u8[1] & (0x3F)) << 6) | ((rx_frame.data.u8[0] >> 2) & (0x3F));  //to datalayer_extended
        //BattBrickVoltageMin m1 : 16|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[3] & (0x0FU)) << 8) | (_d[2] & (0xFFU));
        battery_BrickVoltageMin =
            ((rx_frame.data.u8[3] & (0x0F)) << 8) | (rx_frame.data.u8[2] & (0xFF));  //to datalayer_extended
        //BattBrickVoltageMaxNum m1 : 32|7@1+ (1,1) [0|0] ""  Receiver
        battery_BrickVoltageMaxNum =
            1 + (rx_frame.data.u8[4] & 0x7F);  //(_d[4] & (0x7FU)); //This cell has highest voltage
        //BattBrickVoltageMinNum m1 : 40|7@1+ (1,1) [0|0] ""  Receiver
        battery_BrickVoltageMinNum =
            1 + (rx_frame.data.u8[5] & 0x7F);  //(_d[5] & (0x7FU)); //This cell has lowest voltage
      }
      if (mux == 0)  //Temperature sensors
      {              //BattBrickTempMax m0 : 16|8@1+ (0.5,-40) [0|0] "C" (_d[2] & (0xFFU));
        battery_max_temp = (rx_frame.data.u8[2] * 5) - 400;  //Temperature values have 40.0*C offset, 0.5*C /bit
        //BattBrickTempMin m0 : 24|8@1+ (0.5,-40) [0|0] "C" (_d[3] & (0xFFU));
        battery_min_temp =
            (rx_frame.data.u8[3] * 5) - 400;  //Multiply by 5 and remove offset to get C+1 (0x61*5=485-400=8.5*C)
        //BattBrickTempMaxNum m0 : 2|4@1+ (1,0) [0|0] "" ((_d[0] >> 2) & (0x0FU));
        battery_BrickTempMaxNum = ((rx_frame.data.u8[0] >> 2) & (0x0F));  //to datalayer_extended
        //BattBrickTempMinNum m0 : 8|4@1+ (1,0) [0|0] "" (_d[1] & (0x0FU));
        battery_BrickTempMinNum = (rx_frame.data.u8[1] & (0x0F));  //to datalayer_extended
        //BattBrickModelTMax m0 : 32|8@1+ (0.5,-40) [0|0] "C" (_d[4] & (0xFFU));
        battery_BrickModelTMax = (rx_frame.data.u8[4] & (0xFFU));  //to datalayer_extended
        //BattBrickModelTMin m0 : 40|8@1+ (0.5,-40) [0|0] "C" (_d[5] & (0xFFU));
        battery_BrickModelTMin = (rx_frame.data.u8[5] & (0xFFU));  //to datalayer_extended
      }
      break;
    case 0x312:  // 786 BMS_thermalStatus
      BMS_powerDissipation =
          ((rx_frame.data.u8[1] & (0x03U)) << 8) | (rx_frame.data.u8[0] & (0xFFU));  //0|10@1+ (0.02,0) [0|0] "kW"
      BMS_flowRequest = ((rx_frame.data.u8[2] & (0x01U)) << 6) |
                        ((rx_frame.data.u8[1] >> 2) & (0x3FU));  //10|7@1+ (0.3,0) [0|0] "LPM"
      BMS_inletActiveCoolTargetT = ((rx_frame.data.u8[3] & (0x03U)) << 7) |
                                   ((rx_frame.data.u8[2] >> 1) & (0x7FU));  //17|9@1+ (0.25,-25) [0|0] "DegC"
      BMS_inletPassiveTargetT = ((rx_frame.data.u8[4] & (0x07U)) << 6) |
                                ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //26|9@1+ (0.25,-25) [0|0] "DegC"  X
      BMS_inletActiveHeatTargetT = ((rx_frame.data.u8[5] & (0x0FU)) << 5) |
                                   ((rx_frame.data.u8[4] >> 3) & (0x1FU));  //35|9@1+ (0.25,-25) [0|0] "DegC"
      BMS_packTMin = ((rx_frame.data.u8[6] & (0x1FU)) << 4) |
                     ((rx_frame.data.u8[5] >> 4) & (0x0FU));  //44|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS_packTMax = ((rx_frame.data.u8[7] & (0x3FU)) << 3) |
                     ((rx_frame.data.u8[6] >> 5) & (0x07U));          //53|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS_pcsNoFlowRequest = ((rx_frame.data.u8[7] >> 6) & (0x01U));  // 62|1@1+ (1,0) [0|0] ""
      BMS_noFlowRequest = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""
      break;
    case 0x2A4:  //676 PCS_thermalStatus
      PCS_chgPhATemp =
          ((rx_frame.data.u8[1] & (0x07U)) << 8) | (rx_frame.data.u8[0] & (0xFFU));  //0|11@1- (0.1,40) [0|0] "C
      PCS_chgPhBTemp =
          ((rx_frame.data.u8[2] & (0x3FU)) << 5) | ((rx_frame.data.u8[1] >> 3) & (0x1FU));  //11|11@1- (0.1,40) [0|0] "C
      PCS_chgPhCTemp =
          ((rx_frame.data.u8[4] & (0x07U)) << 8) | (rx_frame.data.u8[3] & (0xFFU));  //24|11@1- (0.1,40) [0|0] "C"
      PCS_dcdcTemp = ((rx_frame.data.u8[5] & (0x3FU)) << 5) |
                     ((rx_frame.data.u8[4] >> 3) & (0x1FU));  //35|11@1- (0.1,40) [0|0] "C"
      PCS_ambientTemp =
          ((rx_frame.data.u8[7] & (0x07U)) << 8) | (rx_frame.data.u8[6] & (0xFFU));  //48|11@1- (0.1,40) [0|0] "C"
      break;
    case 0x2C4:  // 708 PCS_logging: not all frames are listed, just ones relating to dcdc
      mux = (rx_frame.data.u8[0] & (0x1FU));
      //PCS_logMessageSelect = (rx_frame.data.u8[0] & (0x1FU));  //0|5@1+ (1,0) [0|0] ""
      if (mux == 6) {
        PCS_dcdcMaxLvOutputCurrent = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                                     ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //m6 : 28|12@1+ (0.1,0) [0|0] "A"  X
        PCS_dcdcCurrentLimit = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  //m6 : 40|12@1+ (0.1,0) [0|0] "A"  X
        PCS_dcdcLvOutputCurrentTempLimit = ((rx_frame.data.u8[7] & (0xFFU)) << 4) |
                                           ((rx_frame.data.u8[6] >> 4) & (0x0FU));  //m6 : 52|12@1+ (0.1,0) [0|0] "A"  X
      }
      if (mux == 7) {
        PCS_dcdcUnifiedCommand = ((rx_frame.data.u8[1] & (0x7FU)) << 3) |
                                 ((rx_frame.data.u8[0] >> 5) & (0x07U));  //m7 : 5|10@1+ (0.001,0) [0|0] "1"  X
        PCS_dcdcCLAControllerOutput = ((rx_frame.data.u8[3] & (0x03U)) << 8) |
                                      (rx_frame.data.u8[2] & (0xFFU));  //m7 : 16|10@1+ (0.001,0) [0|0] "1"  X
        PCS_dcdcTankVoltage = ((rx_frame.data.u8[4] & (0x1FU)) << 6) |
                              ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //m7 : 26|11@1- (1,0) [0|0] "V"  X
        PCS_dcdcTankVoltageTarget = ((rx_frame.data.u8[5] & (0x7FU)) << 3) |
                                    ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m7 : 37|10@1+ (1,0) [0|0] "V"  X
        PCS_dcdcClaCurrentFreq = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                 (rx_frame.data.u8[6] & (0xFFU));  //P m7 : 48|12@1+ (0.0976563,0) [0|0] "kHz"  X
      }
      if (mux == 8) {
        PCS_dcdcTCommMeasured = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[1] & (0xFFU));  // m8 : 8|16@1- (0.00195313,0) [0|0] "us"  X
        PCS_dcdcShortTimeUs = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  // m8 : 24|16@1+ (0.000488281,0) [0|0] "us"  X
        PCS_dcdcHalfPeriodUs = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  // m8 : 40|16@1+ (0.000488281,0) [0|0] "us"  X
      }
      if (mux == 18) {
        PCS_dcdcIntervalMaxFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                       (rx_frame.data.u8[1] & (0xFFU));  // m18 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS_dcdcIntervalMaxHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                       (rx_frame.data.u8[3] & (0xFFU));  //m18 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMaxLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                       ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m18 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMaxLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                          (rx_frame.data.u8[6] & (0xFFU));  //m18 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 19) {
        PCS_dcdcIntervalMinFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                       (rx_frame.data.u8[1] & (0xFFU));  //m19 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS_dcdcIntervalMinHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                       (rx_frame.data.u8[3] & (0xFFU));  //m19 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMinLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                       ((rx_frame.data.u8[4] >> 5) & (0x07U));  //m19 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS_dcdcIntervalMinLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                          (rx_frame.data.u8[6] & (0xFFU));  // m19 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 22) {
        PCS_dcdc12vSupportLifetimekWh = ((rx_frame.data.u8[3] & (0xFFU)) << 16) |
                                        ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                        (rx_frame.data.u8[1] & (0xFFU));  // m22 : 8|24@1+ (0.01,0) [0|0] "kWh"  X
      }
      break;
    case 0x401:                     // Cell stats  //BrickVoltages
      mux = (rx_frame.data.u8[0]);  //MultiplexSelector M : 0|8@1+ (1,0) [0|0] ""
                                    //StatusFlags : 8|8@1+ (1,0) [0|0] ""
                                    //Brick0 m0 : 16|16@1+ (0.0001,0) [0|0] "V"
                                    //Brick1 m0 : 32|16@1+ (0.0001,0) [0|0] "V"
                                    //Brick2 m0 : 48|16@1+ (0.0001,0) [0|0] "V"
      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

      if (rx_frame.data.u8[1] == 0x2A)  // status byte must be 0x2A to read cellvoltages
      {
        // Example, frame3=0x89,frame2=0x1D = 35101 / 10 = 3510mV
        volts = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 10;
        datalayer.battery.status.cell_voltages_mV[mux * 3] = volts;
        volts = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) / 10;
        datalayer.battery.status.cell_voltages_mV[1 + mux * 3] = volts;
        volts = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) / 10;
        datalayer.battery.status.cell_voltages_mV[2 + mux * 3] = volts;

        // Track the max value of mux. If we've seen two 0 values for mux, we've probably gathered all
        // cell voltages. Then, 2 + mux_max * 3 + 1 is the number of cell voltages.
        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = 2 + 3 * mux_max + 1;
            // Increase the counter arbitrarily another time to make the initial if-statement evaluate to false
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x2d2:  //BMSVAlimits:
      battery_bms_min_voltage =
          ((rx_frame.data.u8[1] << 8) |
           rx_frame.data.u8[0]);  //0|16@1+ (0.01,0) [0|430] "V"  //Example 24148mv * 0.01 = 241.48 V
      battery_bms_max_voltage =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8[2]);  //16|16@1+ (0.01,0) [0|430] "V"  //Example 40282mv * 0.01 = 402.82 V
      battery_max_charge_current = (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) *
                                   0.1;  //32|14@1+ (0.1,0) [0|1638.2] "A"  //Example 1301? * 0.1 = 130.1?
      battery_max_discharge_current = (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) *
                                      0.128;  //48|14@1+ (0.128,0) [0|2096.9] "A"  //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:  //PCS_dcdcRailStatus:
      battery_dcdcLvBusVolt =
          (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);  //0|10@1+ (0.0390625,0) [0|39.9609] "V"
      battery_dcdcHvBusVolt = (((rx_frame.data.u8[2] & 0x3F) << 6) |
                               ((rx_frame.data.u8[1] & 0xFC) >> 2));  //10|12@1+ (0.146484,0) [0|599.854] "V"
      battery_dcdcLvOutputCurrent =
          (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]);  //24|12@1+ (0.1,0) [0|400] "A"
      break;
    case 0x292:                                                            //BMS_socStatus
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  //We are getting CAN messages from the BMS
      battery_beginning_of_life =
          (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[5]) * 0.1;          //40|10@1+ (0.1,0) [0|102.3] "kWh"
      battery_soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);  //0|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_ui =
          (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));  //10|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_max =
          (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));  //20|10@1+ (0.1,0) [0|102.3] "%"
      battery_soc_ave =
          ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));  //30|10@1+ (0.1,0) [0|102.3] "%"
      battery_battTempPct =
          (((rx_frame.data.u8[7] & 0x03) << 6) | (rx_frame.data.u8[6] & 0x3F) >> 2);  //50|8@1+ (0.4,0) [0|100] "%"
      break;
    case 0x392:  //BMS_packConfig
      mux = (rx_frame.data.u8[0] & (0xFF));
      if (mux == 1) {
        battery_packConfigMultiplexer = (rx_frame.data.u8[0] & (0xff));  //0|8@1+ (1,0) [0|1] ""
        battery_moduleType =
            (rx_frame.data.u8[1] &
             (0x07));  //8|3@1+ (1,0) [0|4] ""//0 "UNKNOWN" 1 "E3_NCT" 2 "E1_NCT" 3 "E3_CT" 4 "E1_CT" 5 "E1_CP" ;//to datalayer_extended
        battery_packMass = (rx_frame.data.u8[2]) + 300;  //16|8@1+ (1,300) [342|469] "kg"
        battery_platformMaxBusVoltage =
            (((rx_frame.data.u8[4] & 0x03) << 8) | (rx_frame.data.u8[3]));  //24|10@1+ (0.1,375) [0|0] "V"
      }
      if (mux == 0) {
        battery_reservedConfig =
            (rx_frame.data.u8[1] &
             (0x1F));  //8|5@1+ (1,0) [0|31] ""//0 "BMS_CONFIG_0" 1 "BMS_CONFIG_1" 10 "BMS_CONFIG_10" 11 "BMS_CONFIG_11" 12 "BMS_CONFIG_12" 13 "BMS_CONFIG_13" 14 "BMS_CONFIG_14" 15 "BMS_CONFIG_15" 16 "BMS_CONFIG_16" 17 "BMS_CONFIG_17" 18 "BMS_CONFIG_18" 19 "BMS_CONFIG_19" 2 "BMS_CONFIG_2" 20 "BMS_CONFIG_20" 21 "BMS_CONFIG_21" 22 "BMS_CONFIG_22" 23 "BMS_CONFIG_23" 24 "BMS_CONFIG_24" 25 "BMS_CONFIG_25" 26 "BMS_CONFIG_26" 27 "BMS_CONFIG_27" 28 "BMS_CONFIG_28" 29 "BMS_CONFIG_29" 3 "BMS_CONFIG_3" 30 "BMS_CONFIG_30" 31 "BMS_CONFIG_31" 4 "BMS_CONFIG_4" 5 "BMS_CONFIG_5" 6 "BMS_CONFIG_6" 7 "BMS_CONFIG_7" 8 "BMS_CONFIG_8" 9 "BMS_CONFIG_9" ;
      }
      break;
    case 0x7AA:  //1962 HVP_debugMessage:
      mux = (rx_frame.data.u8[0] & (0x0FU));
      //HVP_debugMessageMultiplexer = (rx_frame.data.u8[0] & (0x0FU));  //0|4@1+ (1,0) [0|6] ""
      if (mux == 0) {
        HVP_gpioPassivePyroDepl = ((rx_frame.data.u8[0] >> 4) & (0x01U));       //: 4|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPyroIsoEn = ((rx_frame.data.u8[0] >> 5) & (0x01U));             //: 5|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpFaultIn = ((rx_frame.data.u8[0] >> 6) & (0x01U));             //: 6|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPackContPowerEn = ((rx_frame.data.u8[0] >> 7) & (0x01U));       //: 7|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvCablesOk = (rx_frame.data.u8[1] & (0x01U));                   //: 8|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvpSelfEnable = ((rx_frame.data.u8[1] >> 1) & (0x01U));         //: 9|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioLed = ((rx_frame.data.u8[1] >> 2) & (0x01U));                   //: 10|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCrashSignal = ((rx_frame.data.u8[1] >> 3) & (0x01U));           //: 11|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioShuntDataReady = ((rx_frame.data.u8[1] >> 4) & (0x01U));        //: 12|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContPosAux = ((rx_frame.data.u8[1] >> 5) & (0x01U));          //: 13|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContNegAux = ((rx_frame.data.u8[1] >> 6) & (0x01U));          //: 14|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioBmsEout = ((rx_frame.data.u8[1] >> 7) & (0x01U));               //: 15|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpFaultOut = (rx_frame.data.u8[2] & (0x01U));                   //: 16|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPyroPor = ((rx_frame.data.u8[2] >> 1) & (0x01U));               //: 17|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioShuntEn = ((rx_frame.data.u8[2] >> 2) & (0x01U));               //: 18|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvpVerEn = ((rx_frame.data.u8[2] >> 3) & (0x01U));              //: 19|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPackCoontPosFlywheel = ((rx_frame.data.u8[2] >> 4) & (0x01U));  //: 20|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioCpLatchEnable = ((rx_frame.data.u8[2] >> 5) & (0x01U));         //: 21|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsEnable = ((rx_frame.data.u8[2] >> 6) & (0x01U));             //: 22|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsDcdcPwmEnable = ((rx_frame.data.u8[2] >> 7) & (0x01U));      //: 23|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioPcsChargePwmEnable = (rx_frame.data.u8[3] & (0x01U));           //: 24|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioFcContPowerEnable = ((rx_frame.data.u8[3] >> 1) & (0x01U));     //: 25|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioHvilEnable = ((rx_frame.data.u8[3] >> 2) & (0x01U));            //: 26|1@1+ (1,0) [0|1] ""  Receiver
        HVP_gpioSecDrdy = ((rx_frame.data.u8[3] >> 3) & (0x01U));               //: 27|1@1+ (1,0) [0|1] ""  Receiver
        HVP_hvp1v5Ref = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                        ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|3] "V"  Receiver
        HVP_shuntCurrentDebug = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[5] & (0xFFU));     //: 40|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP_packCurrentMia = (rx_frame.data.u8[7] & (0x01U));        //: 56|1@1+ (1,0) [0|1] ""  Receiver
        HVP_auxCurrentMia = ((rx_frame.data.u8[7] >> 1) & (0x01U));  //: 57|1@1+ (1,0) [0|1] ""  Receiver
        HVP_currentSenseMia = ((rx_frame.data.u8[7] >> 2) & (0x03U));          //: 58|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntRefVoltageMismatch = ((rx_frame.data.u8[7] >> 3) & (0x01U));  //: 59|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntThermistorMia = ((rx_frame.data.u8[7] >> 4) & (0x01U));       //: 60|1@1+ (1,0) [0|1] ""  Receiver
        HVP_shuntHwMia = ((rx_frame.data.u8[7] >> 5) & (0x01U));               //: 61|1@1+ (1,0) [0|1] ""  Receiver
      }
      if (mux == 1) {
        HVP_dcLinkVoltage = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP_packVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                          (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP_fcLinkVoltage = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
      }
      if (mux == 2) {
        HVP_packContVoltage = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                              ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_packNegativeV = ((rx_frame.data.u8[3] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[2] & (0xFFU));  //: 16|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_packPositiveV = ((rx_frame.data.u8[5] & (0xFFU)) << 8) |
                            (rx_frame.data.u8[4] & (0xFFU));  //: 32|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_pyroAnalog = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                         (rx_frame.data.u8[6] & (0xFFU));  //: 48|12@1+ (0.1,0) [0|3] "V"  Receiver
      }
      if (mux == 3) {
        HVP_dcLinkNegativeV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_dcLinkPositiveV = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_fcLinkNegativeV = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-550|550] "V"  Receiver
      }
      if (mux == 4) {
        HVP_fcContCoilCurrent = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                                ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP_fcContVoltage = ((rx_frame.data.u8[3] & (0x0FU)) << 8) |
                            (rx_frame.data.u8[2] & (0xFFU));  //: 16|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_hvilInVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                            ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_hvilOutVoltage = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                             (rx_frame.data.u8[5] & (0xFFU));  //: 40|12@1+ (0.1,0) [0|30] "V"  Receiver
      }
      if (mux == 5) {
        HVP_fcLinkPositiveV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP_packContCoilCurrent = ((rx_frame.data.u8[4] & (0x0FU)) << 8) |
                                  (rx_frame.data.u8[3] & (0xFFU));  //: 24|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP_battery12V = ((rx_frame.data.u8[5] & (0xFFU)) << 4) |
                         ((rx_frame.data.u8[4] >> 4) & (0x0FU));  //: 36|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP_shuntRefVoltageDbg = ((rx_frame.data.u8[7] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[6] & (0xFFU));  //: 48|16@1- (0.001,0) [-32.768|32.767] "V"  Receiver
      }
      if (mux == 6) {
        HVP_shuntAuxCurrentDbg = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP_shuntBarTempDbg = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                              (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP_shuntAsicTempDbg = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP_shuntAuxCurrentStatus = (rx_frame.data.u8[7] & (0x03U));       //: 56|2@1+ (1,0) [0|3] ""  Receiver
        HVP_shuntBarTempStatus = ((rx_frame.data.u8[7] >> 2) & (0x03U));   //: 58|2@1+ (1,0) [0|3] ""  Receiver
        HVP_shuntAsicTempStatus = ((rx_frame.data.u8[7] >> 4) & (0x03U));  //: 60|2@1+ (1,0) [0|3] ""  Receiver
      }
      break;
    case 0x3aa:  //HVP_alertMatrix1
      battery_WatchdogReset = (rx_frame.data.u8[0] & 0x01);
      battery_PowerLossReset = ((rx_frame.data.u8[0] & 0x02) >> 1);
      battery_SwAssertion = ((rx_frame.data.u8[0] & 0x04) >> 2);
      battery_CrashEvent = ((rx_frame.data.u8[0] & 0x08) >> 3);
      battery_OverDchgCurrentFault = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery_OverChargeCurrentFault = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery_OverCurrentFault = ((rx_frame.data.u8[0] & 0x40) >> 6);
      battery_OverTemperatureFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery_OverVoltageFault = (rx_frame.data.u8[1] & 0x01);
      battery_UnderVoltageFault = ((rx_frame.data.u8[1] & 0x02) >> 1);
      battery_PrimaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x04) >> 2);
      battery_SecondaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x08) >> 3);
      battery_BmbMismatchFault = ((rx_frame.data.u8[1] & 0x10) >> 4);
      battery_BmsHviMiaFault = ((rx_frame.data.u8[1] & 0x20) >> 5);
      battery_CpMiaFault = ((rx_frame.data.u8[1] & 0x40) >> 6);
      battery_PcsMiaFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery_BmsFault = (rx_frame.data.u8[2] & 0x01);
      battery_PcsFault = ((rx_frame.data.u8[2] & 0x02) >> 1);
      battery_CpFault = ((rx_frame.data.u8[2] & 0x04) >> 2);
      battery_ShuntHwMiaFault = ((rx_frame.data.u8[2] & 0x08) >> 3);
      battery_PyroMiaFault = ((rx_frame.data.u8[2] & 0x10) >> 4);
      battery_hvsMiaFault = ((rx_frame.data.u8[2] & 0x20) >> 5);
      battery_hviMiaFault = ((rx_frame.data.u8[2] & 0x40) >> 6);
      battery_Supply12vFault = ((rx_frame.data.u8[2] & 0x80) >> 7);
      battery_VerSupplyFault = (rx_frame.data.u8[3] & 0x01);
      battery_HvilFault = ((rx_frame.data.u8[3] & 0x02) >> 1);
      battery_BmsHvsMiaFault = ((rx_frame.data.u8[3] & 0x04) >> 2);
      battery_PackVoltMismatchFault = ((rx_frame.data.u8[3] & 0x08) >> 3);
      battery_EnsMiaFault = ((rx_frame.data.u8[3] & 0x10) >> 4);
      battery_PackPosCtrArcFault = ((rx_frame.data.u8[3] & 0x20) >> 5);
      battery_packNegCtrArcFault = ((rx_frame.data.u8[3] & 0x40) >> 6);
      battery_ShuntHwAndBmsMiaFault = ((rx_frame.data.u8[3] & 0x80) >> 7);
      battery_fcContHwFault = (rx_frame.data.u8[4] & 0x01);
      battery_robinOverVoltageFault = ((rx_frame.data.u8[4] & 0x02) >> 1);
      battery_packContHwFault = ((rx_frame.data.u8[4] & 0x04) >> 2);
      battery_pyroFuseBlown = ((rx_frame.data.u8[4] & 0x08) >> 3);
      battery_pyroFuseFailedToBlow = ((rx_frame.data.u8[4] & 0x10) >> 4);
      battery_CpilFault = ((rx_frame.data.u8[4] & 0x20) >> 5);
      battery_PackContactorFellOpen = ((rx_frame.data.u8[4] & 0x40) >> 6);
      battery_FcContactorFellOpen = ((rx_frame.data.u8[4] & 0x80) >> 7);
      battery_packCtrCloseBlocked = (rx_frame.data.u8[5] & 0x01);
      battery_fcCtrCloseBlocked = ((rx_frame.data.u8[5] & 0x02) >> 1);
      battery_packContactorForceOpen = ((rx_frame.data.u8[5] & 0x04) >> 2);
      battery_fcContactorForceOpen = ((rx_frame.data.u8[5] & 0x08) >> 3);
      battery_dcLinkOverVoltage = ((rx_frame.data.u8[5] & 0x10) >> 4);
      battery_shuntOverTemperature = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery_passivePyroDeploy = ((rx_frame.data.u8[5] & 0x40) >> 6);
      battery_logUploadRequest = ((rx_frame.data.u8[5] & 0x80) >> 7);
      battery_packCtrCloseFailed = (rx_frame.data.u8[6] & 0x01);
      battery_fcCtrCloseFailed = ((rx_frame.data.u8[6] & 0x02) >> 1);
      battery_shuntThermistorMia = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;
    case 0x320:  //800 BMS_alertMatrix                                                //BMS_alertMatrix 800 BMS_alertMatrix: 8 VEH
      mux = (rx_frame.data.u8[0] & (0x0F));
      if (mux == 0)
        ;
      {                                                                                   //mux0
        battery_BMS_matrixIndex = (rx_frame.data.u8[0] & (0x0F));                         // 0|4@1+ (1,0) [0|0] ""  X
        battery_BMS_a017_SW_Brick_OV = ((rx_frame.data.u8[2] >> 4) & (0x01));             //20|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a018_SW_Brick_UV = ((rx_frame.data.u8[2] >> 5) & (0x01));             //21|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a019_SW_Module_OT = ((rx_frame.data.u8[2] >> 6) & (0x01));            //22|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a021_SW_Dr_Limits_Regulation = (rx_frame.data.u8[3] & (0x01U));       //24|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a022_SW_Over_Current = ((rx_frame.data.u8[3] >> 1) & (0x01U));        //25|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a023_SW_Stack_OV = ((rx_frame.data.u8[3] >> 2) & (0x01U));            //26|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a024_SW_Islanded_Brick = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a025_SW_PwrBalance_Anomaly = ((rx_frame.data.u8[3] >> 4) & (0x01U));  //28|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a026_SW_HFCurrent_Anomaly = ((rx_frame.data.u8[3] >> 5) & (0x01U));   //29|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a034_SW_Passive_Isolation = ((rx_frame.data.u8[4] >> 5) & (0x01U));   //37|1@1+ (1,0) [0|0] ""  X ?
        battery_BMS_a035_SW_Isolation = ((rx_frame.data.u8[4] >> 6) & (0x01U));           //38|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a036_SW_HvpHvilFault = ((rx_frame.data.u8[4] >> 6) & (0x01U));        //39|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a037_SW_Flood_Port_Open = (rx_frame.data.u8[5] & (0x01U));            //40|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a039_SW_DC_Link_Over_Voltage = ((rx_frame.data.u8[5] >> 2) & (0x01U));  //42|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a041_SW_Power_On_Reset = ((rx_frame.data.u8[5] >> 4) & (0x01U));        //44|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a042_SW_MPU_Error = ((rx_frame.data.u8[5] >> 5) & (0x01U));             //45|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a043_SW_Watch_Dog_Reset = ((rx_frame.data.u8[5] >> 6) & (0x01U));       //46|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a044_SW_Assertion = ((rx_frame.data.u8[5] >> 7) & (0x01U));             //47|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a045_SW_Exception = (rx_frame.data.u8[6] & (0x01U));                    //48|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a046_SW_Task_Stack_Usage = ((rx_frame.data.u8[6] >> 1) & (0x01U));      //49|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a047_SW_Task_Stack_Overflow = ((rx_frame.data.u8[6] >> 2) & (0x01U));   //50|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a048_SW_Log_Upload_Request = ((rx_frame.data.u8[6] >> 3) & (0x01U));    //51|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a050_SW_Brick_Voltage_MIA = ((rx_frame.data.u8[6] >> 5) & (0x01U));     //53|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a051_SW_HVC_Vref_Bad = ((rx_frame.data.u8[6] >> 6) & (0x01U));          //54|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a052_SW_PCS_MIA = ((rx_frame.data.u8[6] >> 7) & (0x01U));               //55|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a053_SW_ThermalModel_Sanity = (rx_frame.data.u8[7] & (0x01U));          //56|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a054_SW_Ver_Supply_Fault = ((rx_frame.data.u8[7] >> 1) & (0x01U));      //57|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a059_SW_Pack_Voltage_Sensing = ((rx_frame.data.u8[7] >> 6) & (0x01U));  //62|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a060_SW_Leakage_Test_Failure = ((rx_frame.data.u8[7] >> 7) & (0x01U));  //63|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 1) {                                                                       //mux1
        battery_BMS_a061_robinBrickOverVoltage = ((rx_frame.data.u8[0] >> 4) & (0x01U));    //4|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a062_SW_BrickV_Imbalance = ((rx_frame.data.u8[0] >> 5) & (0x01U));      //5|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a063_SW_ChargePort_Fault = ((rx_frame.data.u8[0] >> 6) & (0x01U));      //6|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a064_SW_SOC_Imbalance = ((rx_frame.data.u8[0] >> 7) & (0x01U));         //7|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a069_SW_Low_Power = ((rx_frame.data.u8[1] >> 4) & (0x01U));             //12|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a071_SW_SM_TransCon_Not_Met = ((rx_frame.data.u8[1] >> 6) & (0x01U));   //14|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a075_SW_Chg_Disable_Failure = ((rx_frame.data.u8[2] >> 2) & (0x01U));   //18|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a076_SW_Dch_While_Charging = ((rx_frame.data.u8[2] >> 3) & (0x01U));    //19|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a077_SW_Charger_Regulation = ((rx_frame.data.u8[2] >> 4) & (0x01U));    //20|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a081_SW_Ctr_Close_Blocked = (rx_frame.data.u8[3] & (0x01U));            //24|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a082_SW_Ctr_Force_Open = ((rx_frame.data.u8[3] >> 1) & (0x01U));        //25|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a083_SW_Ctr_Close_Failure = ((rx_frame.data.u8[3] >> 2) & (0x01U));     //26|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a084_SW_Sleep_Wake_Aborted = ((rx_frame.data.u8[3] >> 3) & (0x01U));    //27|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a087_SW_Feim_Test_Blocked = ((rx_frame.data.u8[3] >> 6) & (0x01U));     //30|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a088_SW_VcFront_MIA_InDrive = ((rx_frame.data.u8[3] >> 7) & (0x01U));   //31|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a089_SW_VcFront_MIA = (rx_frame.data.u8[4] & (0x01U));                  //32|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a090_SW_Gateway_MIA = ((rx_frame.data.u8[4] >> 1) & (0x01U));           //33|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a091_SW_ChargePort_MIA = ((rx_frame.data.u8[4] >> 2) & (0x01U));        //34|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a092_SW_ChargePort_Mia_On_Hv = ((rx_frame.data.u8[4] >> 3) & (0x01U));  //35|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a094_SW_Drive_Inverter_MIA = ((rx_frame.data.u8[4] >> 5) & (0x01U));    //37|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a099_SW_BMB_Communication = ((rx_frame.data.u8[5] >> 2) & (0x01U));     //42|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a105_SW_One_Module_Tsense = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a106_SW_All_Module_Tsense = ((rx_frame.data.u8[6] >> 1) & (0x01U));     //49|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a107_SW_Stack_Voltage_MIA = ((rx_frame.data.u8[6] >> 2) & (0x01U));     //50|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 2) {                                                                       //mux2
        battery_BMS_a121_SW_NVRAM_Config_Error = ((rx_frame.data.u8[0] >> 4) & (0x01U));    // 4|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a122_SW_BMS_Therm_Irrational = ((rx_frame.data.u8[0] >> 5) & (0x01U));  //5|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a123_SW_Internal_Isolation = ((rx_frame.data.u8[0] >> 6) & (0x01U));    //6|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a127_SW_shunt_SNA = ((rx_frame.data.u8[1] >> 2) & (0x01U));             //10|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a128_SW_shunt_MIA = ((rx_frame.data.u8[1] >> 3) & (0x01U));             //11|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a129_SW_VSH_Failure = ((rx_frame.data.u8[1] >> 4) & (0x01U));           //12|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a130_IO_CAN_Error = ((rx_frame.data.u8[1] >> 5) & (0x01U));             //13|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a131_Bleed_FET_Failure = ((rx_frame.data.u8[1] >> 6) & (0x01U));        //14|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a132_HW_BMB_OTP_Uncorrctbl = ((rx_frame.data.u8[1] >> 7) & (0x01U));    //15|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a134_SW_Delayed_Ctr_Off = ((rx_frame.data.u8[2] >> 1) & (0x01U));       //17|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a136_SW_Module_OT_Warning = ((rx_frame.data.u8[2] >> 3) & (0x01U));     //19|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a137_SW_Brick_UV_Warning = ((rx_frame.data.u8[2] >> 4) & (0x01U));      //20|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a138_SW_Brick_OV_Warning = ((rx_frame.data.u8[2] >> 5) & (0x01U));      //21|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a139_SW_DC_Link_V_Irrational = ((rx_frame.data.u8[2] >> 6) & (0x01U));  //22|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a141_SW_BMB_Status_Warning = (rx_frame.data.u8[3] & (0x01U));           //24|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a144_Hvp_Config_Mismatch = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a145_SW_SOC_Change = ((rx_frame.data.u8[3] >> 4) & (0x01U));            //28|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a146_SW_Brick_Overdischarged = ((rx_frame.data.u8[3] >> 5) & (0x01U));  //29|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a149_SW_Missing_Config_Block = (rx_frame.data.u8[4] & (0x01U));         //32|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a151_SW_external_isolation = ((rx_frame.data.u8[4] >> 2) & (0x01U));    //34|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a156_SW_BMB_Vref_bad = ((rx_frame.data.u8[4] >> 7) & (0x01U));          //39|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a157_SW_HVP_HVS_Comms = (rx_frame.data.u8[5] & (0x01U));                //40|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a158_SW_HVP_HVI_Comms = ((rx_frame.data.u8[5] >> 1) & (0x01U));         //41|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a159_SW_HVP_ECU_Error = ((rx_frame.data.u8[5] >> 2) & (0x01U));         //42|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a161_SW_DI_Open_Request = ((rx_frame.data.u8[5] >> 4) & (0x01U));       //44|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a162_SW_No_Power_For_Support = ((rx_frame.data.u8[5] >> 5) & (0x01U));  //45|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a163_SW_Contactor_Mismatch = ((rx_frame.data.u8[5] >> 6) & (0x01U));    //46|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a164_SW_Uncontrolled_Regen = ((rx_frame.data.u8[5] >> 7) & (0x01U));    //47|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a165_SW_Pack_Partial_Weld = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a166_SW_Pack_Full_Weld = ((rx_frame.data.u8[6] >> 1) & (0x01U));        //49|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a167_SW_FC_Partial_Weld = ((rx_frame.data.u8[6] >> 2) & (0x01U));       //50|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a168_SW_FC_Full_Weld = ((rx_frame.data.u8[6] >> 3) & (0x01U));          //51|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a169_SW_FC_Pack_Weld = ((rx_frame.data.u8[6] >> 4) & (0x01U));          //52|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a170_SW_Limp_Mode = ((rx_frame.data.u8[6] >> 5) & (0x01U));             //53|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a171_SW_Stack_Voltage_Sense = ((rx_frame.data.u8[6] >> 6) & (0x01U));   //54|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a174_SW_Charge_Failure = ((rx_frame.data.u8[7] >> 1) & (0x01U));        //57|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a176_SW_GracefulPowerOff = ((rx_frame.data.u8[7] >> 3) & (0x01U));      //59|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a179_SW_Hvp_12V_Fault = ((rx_frame.data.u8[7] >> 6) & (0x01U));         //62|1@1+ (1,0) [0|0] ""  X
        battery_BMS_a180_SW_ECU_reset_blocked = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""  X
      }
      break;
    default:
      break;
  }
}

#ifdef DOUBLE_BATTERY  //Need to update battery2

void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  static uint16_t temp = 0;

  switch (rx_frame.ID) {
    case 0x352:                            // BMS_energyStatus // newer BMS >2021
      mux = (rx_frame.data.u8[0] & 0x02);  //BMS_energyStatusIndex M : 0|2@1+ (1,0) [0|0] ""  X

      if (mux == 0) {
        battery2_nominal_full_pack_energy_m0 =
            ((rx_frame.data.u8[3] << 8) |
             rx_frame.data.u8[2]);  //BMS_nominalFullPackEnergy m0 : 16|16@1+ (0.02,0) [0|0] "kWh"  X
        battery2_nominal_energy_remaining_m0 =
            ((rx_frame.data.u8[5] << 8) |
             rx_frame.data.u8[4]);  //BMS_nominalEnergyRemaining m0 : 32|16@1+ (0.02,0) [0|0] "kWh"  X
        battery2_ideal_energy_remaining_m0 =
            ((rx_frame.data.u8[7] << 8) |
             rx_frame.data.u8[6]);  //BMS_idealEnergyRemaining m0 : 48|16@1+ (0.02,0) [0|0] "kWh"  X
      }
      if (mux == 1) {
        battery2_fully_charged = (rx_frame.data.u8[1] & 0x01);  //BMS_fullyCharged m1 : 15|1@1+ (1,0) [0|1] ""  X
        battery2_energy_buffer_m1 =
            (rx_frame.data.u8[3] | rx_frame.data.u8[2]);  //BMS_energyBuffer m1 : 16|16@1+ (0.01,0) [0|0] "kWh"  X
        battery2_expected_energy_remaining_m1 =
            (rx_frame.data.u8[5] |
             rx_frame.data.u8[4]);  //BMS_expectedEnergyRemaining m1 : 32|16@1+ (0.02,0) [0|0] "kWh"  X
        battery2_energy_to_charge_complete_m1 =
            (rx_frame.data.u8[7] |
             rx_frame.data.u8[6]);  //BMS_energyToChargeComplete m1 : 48|16@1+ (0.02,0) [0|0] "kWh"  X
      }
      if (mux == 2) {}
      // Additional information needed on this mux, example frame: 02 26 02 20 02 80 00 00 doesn't change
      // older BMS <2021 without mux
      battery2_nominal_full_pack_energy =  //BMS_nominalFullPackEnergy : 0|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[1] & (0x07U)) << 8) | (_d[0] & (0xFFU));
          (((rx_frame.data.u8[1] & 0x07) << 8) | (rx_frame.data.u8[0]));  //Example 752 (75.2kWh)
      battery2_nominal_energy_remaining =  //BMS_nominalEnergyRemaining : 11|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[2] & (0x3FU)) << 5) | ((_d[1] >> 3) & (0x1FU));
          (((rx_frame.data.u8[2] & 0x3F) << 5) | ((rx_frame.data.u8[1] & 0x1F) >> 3));  //Example 1247 * 0.1 = 124.7kWh
      battery2_expected_energy_remaining =  //BMS_expectedEnergyRemaining : 22|11@1+ (0.1,0) [0|204.6] "KWh"// ((_d[4] & (0x01U)) << 10) | ((_d[3] & (0xFFU)) << 2) | ((_d[2] >> 6) & (0x03U));
          (((rx_frame.data.u8[4] & 0x01) << 10) | (rx_frame.data.u8[3] << 2) |
           ((rx_frame.data.u8[2] & 0x03) >> 6));  //Example 622 (62.2kWh)
      battery2_ideal_energy_remaining =  //BMS_idealEnergyRemaining : 33|11@1+ (0.1,0) [0|204.6] "KWh" //((_d[5] & (0x0FU)) << 7) | ((_d[4] >> 1) & (0x7FU));
          (((rx_frame.data.u8[5] & 0x0F) << 7) | ((rx_frame.data.u8[4] & 0x7F) >> 1));  //Example 311 * 0.1 = 31.1kWh
      battery2_energy_to_charge_complete =  // BMS_energyToChargeComplete : 44|11@1+ (0.1,0) [0|204.6] "KWh"// ((_d[6] & (0x7FU)) << 4) | ((_d[5] >> 4) & (0x0FU));
          (((rx_frame.data.u8[6] & 0x7F) << 4) | ((rx_frame.data.u8[5] & 0x0F) << 4));  //Example 147 * 0.1 = 14.7kWh
      battery2_energy_buffer =  //BMS_energyBuffer : 55|8@1+ (0.1,0) [0|25.4] "KWh"// ((_d[7] & (0x7FU)) << 1) | ((_d[6] >> 7) & (0x01U));
          (((rx_frame.data.u8[7] & 0x7F) << 1) | ((rx_frame.data.u8[6] & 0x01) >> 7));  //Example 1 * 0.1 = 0
      battery2_full_charge_complete =  //BMS_fullChargeComplete : 63|1@1+ (1,0) [0|1] ""//((_d[7] >> 7) & (0x01U));
          ((rx_frame.data.u8[7] & 0x01) >> 7);
      break;
    case 0x20A:  //522 HVP_contactorState:
      battery2_packContNegativeState = (rx_frame.data.u8[0] & 0x07);
      battery2_packContPositiveState = (rx_frame.data.u8[0] & 0x38) >> 3;
      battery2_contactor = (rx_frame.data.u8[1] & 0x0F);
      battery2_packContactorSetState = (rx_frame.data.u8[1] & 0x0F);
      battery2_packCtrsClosingAllowed = (rx_frame.data.u8[4] & 0x08) >> 3;
      battery2_pyroTestInProgress = (rx_frame.data.u8[4] & 0x20) >> 5;
      battery2_hvil_status = (rx_frame.data.u8[5] & 0x0F);
      battery2_packCtrsOpenNowRequested =
          ((rx_frame.data.u8[4] >> 1) & (0x01U));  //33|1@1+ (1,0) [0|1] //to datalayer_extended
      battery2_packCtrsOpenRequested =
          ((rx_frame.data.u8[4] >> 2) & (0x01U));  //34|1@1+ (1,0) [0|1] //to datalayer_extended
      battery2_packCtrsRequestStatus =
          ((rx_frame.data.u8[3] >> 6) & (0x03U));  //30|2@1+ (1,0) [0|2] //to datalayer_extended
      battery2_packCtrsResetRequestRequired =
          (rx_frame.data.u8[4] & (0x01U));  //32|1@1+ (1,0) [0|1] //to datalayer_extended
      battery2_dcLinkAllowedToEnergize =
          ((rx_frame.data.u8[4] >> 4) & (0x01U));  //36|1@1+ (1,0) [0|1] //to datalayer_extended
      battery2_fcContNegativeAuxOpen = ((rx_frame.data.u8[0] >> 7) & (0x01U));       //7|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcContNegativeState = ((rx_frame.data.u8[1] >> 4) & (0x07U));         //12|3@1+ (1,0) [0|7] ""  Receiver
      battery2_fcContPositiveAuxOpen = ((rx_frame.data.u8[0] >> 6) & (0x01U));       //6|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcContPositiveState = (rx_frame.data.u8[2] & (0x07U));                //16|3@1+ (1,0) [0|7] ""  Receiver
      battery2_fcContactorSetState = ((rx_frame.data.u8[2] >> 3) & (0x0FU));         //19|4@1+ (1,0) [0|9] ""  Receiver
      battery2_fcCtrsClosingAllowed = ((rx_frame.data.u8[3] >> 5) & (0x01U));        //29|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcCtrsOpenNowRequested = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcCtrsOpenRequested = ((rx_frame.data.u8[3] >> 4) & (0x01U));         //28|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcCtrsRequestStatus = (rx_frame.data.u8[3] & (0x03U));                //24|2@1+ (1,0) [0|2] ""  Receiver
      battery2_fcCtrsResetRequestRequired = ((rx_frame.data.u8[3] >> 2) & (0x01U));  //26|1@1+ (1,0) [0|1] ""  Receiver
      battery2_fcLinkAllowedToEnergize = ((rx_frame.data.u8[5] >> 4) & (0x03U));     //44|2@1+ (1,0) [0|2] ""  Receiver
      break;
    case 0x212:  //530 BMS_status: 8
      battery2_BMS_hvacPowerRequest = (rx_frame.data.u8[0] & (0x01U));
      battery2_BMS_notEnoughPowerForDrive = ((rx_frame.data.u8[0] >> 1) & (0x01U));
      battery2_BMS_notEnoughPowerForSupport = ((rx_frame.data.u8[0] >> 2) & (0x01U));
      battery2_BMS_preconditionAllowed = ((rx_frame.data.u8[0] >> 3) & (0x01U));
      battery2_BMS_updateAllowed = ((rx_frame.data.u8[0] >> 4) & (0x01U));
      battery2_BMS_activeHeatingWorthwhile = ((rx_frame.data.u8[0] >> 5) & (0x01U));
      battery2_BMS_cpMiaOnHvs = ((rx_frame.data.u8[0] >> 6) & (0x01U));
      battery2_BMS_contactorState =
          (rx_frame.data.u8[1] &
           (0x07U));  //0 "SNA" 1 "OPEN" 2 "OPENING" 3 "CLOSING" 4 "CLOSED" 5 "WELDED" 6 "BLOCKED" ;
      battery2_BMS_state =
          ((rx_frame.data.u8[1] >> 3) &
           (0x0FU));  //0 "STANDBY" 1 "DRIVE" 2 "SUPPORT" 3 "CHARGE" 4 "FEIM" 5 "CLEAR_FAULT" 6 "FAULT" 7 "WELD" 8 "TEST" 9 "SNA" ;
      battery2_BMS_hvState =
          (rx_frame.data.u8[2] &
           (0x07U));  //0 "DOWN" 1 "COMING_UP" 2 "GOING_DOWN" 3 "UP_FOR_DRIVE" 4 "UP_FOR_CHARGE" 5 "UP_FOR_DC_CHARGE" 6 "UP" ;
      battery2_BMS_isolationResistance =
          ((rx_frame.data.u8[3] & (0x1FU)) << 5) |
          ((rx_frame.data.u8[2] >> 3) & (0x1FU));  //19|10@1+ (10,0) [0|0] "kOhm"/to datalayer_extended
      battery2_BMS_chargeRequest = ((rx_frame.data.u8[3] >> 5) & (0x01U));
      battery2_BMS_keepWarmRequest = ((rx_frame.data.u8[3] >> 6) & (0x01U));
      battery2_BMS_uiChargeStatus =
          (rx_frame.data.u8[4] &
           (0x07U));  // 0 "DISCONNECTED" 1 "NO_POWER" 2 "ABOUT_TO_CHARGE" 3 "CHARGING" 4 "CHARGE_COMPLETE" 5 "CHARGE_STOPPED" ;
      battery2_BMS_diLimpRequest = ((rx_frame.data.u8[4] >> 3) & (0x01U));
      battery2_BMS_okToShipByAir = ((rx_frame.data.u8[4] >> 4) & (0x01U));
      battery2_BMS_okToShipByLand = ((rx_frame.data.u8[4] >> 5) & (0x01U));
      battery2_BMS_chgPowerAvailable = ((rx_frame.data.u8[6] & (0x01U)) << 10) |
                                       ((rx_frame.data.u8[5] & (0xFFU)) << 2) |
                                       ((rx_frame.data.u8[4] >> 6) & (0x03U));  //38|11@1+ (0.125,0) [0|0] "kW"
      battery2_BMS_chargeRetryCount = ((rx_frame.data.u8[6] >> 1) & (0x0FU));
      battery2_BMS_pcsPwmEnabled = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      battery2_BMS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      battery2_BMS_minPackTemperature = (rx_frame.data.u8[7] & (0xFFU));  //56|8@1+ (0.5,-40) [0|0] "DegC
      break;
    case 0x224:                                                                    //548 PCS_dcdcStatus:
      battery2_PCS_dcdcPrechargeStatus = (rx_frame.data.u8[0] & (0x03U));          //0 "IDLE" 1 "ACTIVE" 2 "FAULTED" ;
      battery2_PCS_dcdc12VSupportStatus = ((rx_frame.data.u8[0] >> 2) & (0x03U));  //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      battery2_PCS_dcdcHvBusDischargeStatus = ((rx_frame.data.u8[0] >> 4) & (0x03U));  //0 "IDLE" 1 "ACTIVE" 2 "FAULTED"
      battery2_PCS_dcdcMainState =
          ((rx_frame.data.u8[1] & (0x03U)) << 2) |
          ((rx_frame.data.u8[0] >> 6) &
           (0x03U));  //0 "STANDBY" 1 "12V_SUPPORT_ACTIVE" 2 "PRECHARGE_STARTUP" 3 "PRECHARGE_ACTIVE" 4 "DIS_HVBUS_ACTIVE" 5 "SHUTDOWN" 6 "FAULTED" ;
      battery2_PCS_dcdcSubState =
          ((rx_frame.data.u8[1] >> 2) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      battery2_PCS_dcdcFaulted = ((rx_frame.data.u8[1] >> 7) & (0x01U));
      battery2_PCS_dcdcOutputIsLimited = ((rx_frame.data.u8[3] >> 4) & (0x01U));
      battery2_PCS_dcdcMaxOutputCurrentAllowed = ((rx_frame.data.u8[5] & (0x01U)) << 11) |
                                                 ((rx_frame.data.u8[4] & (0xFFU)) << 3) |
                                                 ((rx_frame.data.u8[3] >> 5) & (0x07U));  //29|12@1+ (0.1,0) [0|0] "A"
      battery2_PCS_dcdcPrechargeRtyCnt = ((rx_frame.data.u8[5] >> 1) & (0x07U));
      battery2_PCS_dcdc12VSupportRtyCnt = ((rx_frame.data.u8[5] >> 4) & (0x0FU));
      battery2_PCS_dcdcDischargeRtyCnt = (rx_frame.data.u8[6] & (0x0FU));
      battery2_PCS_dcdcPwmEnableLine = ((rx_frame.data.u8[6] >> 4) & (0x01U));
      battery2_PCS_dcdcSupportingFixedLvTarget = ((rx_frame.data.u8[6] >> 5) & (0x01U));
      battery2_PCS_ecuLogUploadRequest = ((rx_frame.data.u8[6] >> 6) & (0x03U));
      battery2_PCS_dcdcPrechargeRestartCnt = (rx_frame.data.u8[7] & (0x07U));
      battery2_PCS_dcdcInitialPrechargeSubState =
          ((rx_frame.data.u8[7] >> 3) &
           (0x1FU));  //0 "PWR_UP_INIT" 1 "STANDBY" 2 "12V_SUPPORT_ACTIVE" 3 "DIS_HVBUS" 4 "PCHG_FAST_DIS_HVBUS" 5 "PCHG_SLOW_DIS_HVBUS" 6 "PCHG_DWELL_CHARGE" 7 "PCHG_DWELL_WAIT" 8 "PCHG_DI_RECOVERY_WAIT" 9 "PCHG_ACTIVE" 10 "PCHG_FLT_FAST_DIS_HVBUS" 11 "SHUTDOWN" 12 "12V_SUPPORT_FAULTED" 13 "DIS_HVBUS_FAULTED" 14 "PCHG_FAULTED" 15 "CLEAR_FAULTS" 16 "FAULTED" 17 "NUM" ;
      break;
    case 0x252:
      //Limits
      battery2_regenerative_limit =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 4715 * 0.01 = 47.15kW
      battery2_discharge_limit =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.013;  //Example 2009 * 0.013 = 26.117???
      battery2_max_heat_park =
          (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[4]) * 0.01;  //Example 500 * 0.01 = 5kW
      battery2_hvac_max_power =
          (((rx_frame.data.u8[7] << 6) | ((rx_frame.data.u8[6] & 0xFC) >> 2))) * 0.02;  //Example 1000 * 0.02 = 20kW?
      break;
    case 0x132:
      //battery amps/volts
      battery2_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  //Example 37030mv * 0.01 = 370V
      battery2_amps = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);  //Example 65492 (-4.3A) OR 225 (22.5A)
      battery2_raw_amps = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * -0.05;  //Example 10425 * -0.05 = ?
      battery2_charge_time_remaining =
          (((rx_frame.data.u8[7] & 0x0F) << 8) | rx_frame.data.u8[6]);  //Example 228 * 0.1 = 22.8min
      if (battery2_charge_time_remaining == 4095) {
        battery2_charge_time_remaining = 0;
      }
      break;
    case 0x3D2:  // total charge/discharge kwh
      battery2_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                  (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) *
                                 0.001;
      battery2_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                               rx_frame.data.u8[4]) *
                              0.001;
      break;
    case 0x332:  //min/max hist values //BattBrickMinMax:
      mux = (rx_frame.data.u8[0] & 0x03);

      if (mux == 1)  //Cell voltages
      {
        temp = ((rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[0] >> 2));
        temp = (temp & 0xFFF);
        battery2_cell_max_v = temp * 2;
        temp = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
        temp = (temp & 0xFFF);
        battery2_cell_min_v = temp * 2;
        battery2_cellvoltagesRead = true;
        //BattBrickVoltageMax m1 : 2|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[1] & (0x3FU)) << 6) | ((_d[0] >> 2) & (0x3FU));
        battery2_BrickVoltageMax =
            ((rx_frame.data.u8[1] & (0x3F)) << 6) | ((rx_frame.data.u8[0] >> 2) & (0x3F));  //to datalayer_extended
        //BattBrickVoltageMin m1 : 16|12@1+ (0.002,0) [0|0] "V"  Receiver ((_d[3] & (0x0FU)) << 8) | (_d[2] & (0xFFU));
        battery2_BrickVoltageMin =
            ((rx_frame.data.u8[3] & (0x0F)) << 8) | (rx_frame.data.u8[2] & (0xFF));  //to datalayer_extended
        //BattBrickVoltageMaxNum m1 : 32|7@1+ (1,1) [0|0] ""  Receiver
        battery2_BrickVoltageMaxNum = 1 + (rx_frame.data.u8[4] & 0x7F);  //This cell has highest voltage
        battery2_BrickVoltageMinNum = 1 + (rx_frame.data.u8[5] & 0x7F);  //This cell has lowest voltage
      }
      if (mux == 0)  //Temperature sensors
      {
        battery2_max_temp = (rx_frame.data.u8[2] * 5) - 400;  //Temperature values have 40.0*C offset, 0.5*C /bit
        battery2_min_temp =
            (rx_frame.data.u8[3] * 5) - 400;  //Multiply by 5 and remove offset to get C+1 (0x61*5=485-400=8.5*C)
        //BattBrickTempMaxNum m0 : 2|4@1+ (1,0) [0|0] "" ((_d[0] >> 2) & (0x0FU));
        battery2_BrickTempMaxNum = ((rx_frame.data.u8[0] >> 2) & (0x0F));  //to datalayer_extended
        //BattBrickTempMinNum m0 : 8|4@1+ (1,0) [0|0] "" (_d[1] & (0x0FU));
        battery2_BrickTempMinNum = (rx_frame.data.u8[1] & (0x0F));  //to datalayer_extended
        //BattBrickModelTMax m0 : 32|8@1+ (0.5,-40) [0|0] "C" (_d[4] & (0xFFU));
        battery2_BrickModelTMax = (rx_frame.data.u8[4] & (0xFFU));  //to datalayer_extended
        //BattBrickModelTMin m0 : 40|8@1+ (0.5,-40) [0|0] "C" (_d[5] & (0xFFU));
        battery2_BrickModelTMin = (rx_frame.data.u8[5] & (0xFFU));  //to datalayer_extended
      }
      break;
    case 0x312:  // 786 BMS_thermalStatus
      BMS2_powerDissipation =
          ((rx_frame.data.u8[1] & (0x03U)) << 8) | (rx_frame.data.u8[0] & (0xFFU));  //0|10@1+ (0.02,0) [0|0] "kW"
      BMS2_flowRequest = ((rx_frame.data.u8[2] & (0x01U)) << 6) |
                         ((rx_frame.data.u8[1] >> 2) & (0x3FU));  //10|7@1+ (0.3,0) [0|0] "LPM"
      BMS2_inletActiveCoolTargetT = ((rx_frame.data.u8[3] & (0x03U)) << 7) |
                                    ((rx_frame.data.u8[2] >> 1) & (0x7FU));  //17|9@1+ (0.25,-25) [0|0] "DegC"
      BMS2_inletPassiveTargetT = ((rx_frame.data.u8[4] & (0x07U)) << 6) |
                                 ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //26|9@1+ (0.25,-25) [0|0] "DegC"  X
      BMS2_inletActiveHeatTargetT = ((rx_frame.data.u8[5] & (0x0FU)) << 5) |
                                    ((rx_frame.data.u8[4] >> 3) & (0x1FU));  //35|9@1+ (0.25,-25) [0|0] "DegC"
      BMS2_packTMin = ((rx_frame.data.u8[6] & (0x1FU)) << 4) |
                      ((rx_frame.data.u8[5] >> 4) & (0x0FU));  //44|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS2_packTMax = ((rx_frame.data.u8[7] & (0x3FU)) << 3) |
                      ((rx_frame.data.u8[6] >> 5) & (0x07U));          //53|9@1+ (0.25,-25) [-25|100] "DegC"
      BMS2_pcsNoFlowRequest = ((rx_frame.data.u8[7] >> 6) & (0x01U));  // 62|1@1+ (1,0) [0|0] ""
      BMS2_noFlowRequest = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""
      break;
    case 0x2A4:  //676 PCS_thermalStatus
      PCS2_chgPhATemp =
          ((rx_frame.data.u8[1] & (0x07U)) << 8) | (rx_frame.data.u8[0] & (0xFFU));  //0|11@1- (0.1,40) [0|0] "C
      PCS2_chgPhBTemp =
          ((rx_frame.data.u8[2] & (0x3FU)) << 5) | ((rx_frame.data.u8[1] >> 3) & (0x1FU));  //11|11@1- (0.1,40) [0|0] "C
      PCS2_chgPhCTemp =
          ((rx_frame.data.u8[4] & (0x07U)) << 8) | (rx_frame.data.u8[3] & (0xFFU));  //24|11@1- (0.1,40) [0|0] "C"
      PCS2_dcdcTemp = ((rx_frame.data.u8[5] & (0x3FU)) << 5) |
                      ((rx_frame.data.u8[4] >> 3) & (0x1FU));  //35|11@1- (0.1,40) [0|0] "C"
      PCS2_ambientTemp =
          ((rx_frame.data.u8[7] & (0x07U)) << 8) | (rx_frame.data.u8[6] & (0xFFU));  //48|11@1- (0.1,40) [0|0] "C"
      break;
    case 0x2C4:  // 708 PCS_logging: not all frames are listed, just ones relating to dcdc
      mux = (rx_frame.data.u8[0] & (0x1FU));
      //PCS_logMessageSelect = (rx_frame.data.u8[0] & (0x1FU));  //0|5@1+ (1,0) [0|0] ""
      if (mux == 6) {
        PCS2_dcdcMaxLvOutputCurrent = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                                      ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //m6 : 28|12@1+ (0.1,0) [0|0] "A"  X
        PCS2_dcdcCurrentLimit = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                                (rx_frame.data.u8[5] & (0xFFU));  //m6 : 40|12@1+ (0.1,0) [0|0] "A"  X
        PCS2_dcdcLvOutputCurrentTempLimit =
            ((rx_frame.data.u8[7] & (0xFFU)) << 4) |
            ((rx_frame.data.u8[6] >> 4) & (0x0FU));  //m6 : 52|12@1+ (0.1,0) [0|0] "A"  X
      }
      if (mux == 7) {
        PCS2_dcdcUnifiedCommand = ((rx_frame.data.u8[1] & (0x7FU)) << 3) |
                                  ((rx_frame.data.u8[0] >> 5) & (0x07U));  //m7 : 5|10@1+ (0.001,0) [0|0] "1"  X
        PCS2_dcdcCLAControllerOutput = ((rx_frame.data.u8[3] & (0x03U)) << 8) |
                                       (rx_frame.data.u8[2] & (0xFFU));  //m7 : 16|10@1+ (0.001,0) [0|0] "1"  X
        PCS2_dcdcTankVoltage = ((rx_frame.data.u8[4] & (0x1FU)) << 6) |
                               ((rx_frame.data.u8[3] >> 2) & (0x3FU));  //m7 : 26|11@1- (1,0) [0|0] "V"  X
        PCS2_dcdcTankVoltageTarget = ((rx_frame.data.u8[5] & (0x7FU)) << 3) |
                                     ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m7 : 37|10@1+ (1,0) [0|0] "V"  X
        PCS2_dcdcClaCurrentFreq = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                  (rx_frame.data.u8[6] & (0xFFU));  //P m7 : 48|12@1+ (0.0976563,0) [0|0] "kHz"  X
      }
      if (mux == 8) {
        PCS2_dcdcTCommMeasured = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[1] & (0xFFU));  // m8 : 8|16@1- (0.00195313,0) [0|0] "us"  X
        PCS2_dcdcShortTimeUs = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[3] & (0xFFU));  // m8 : 24|16@1+ (0.000488281,0) [0|0] "us"  X
        PCS2_dcdcHalfPeriodUs = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[5] & (0xFFU));  // m8 : 40|16@1+ (0.000488281,0) [0|0] "us"  X
      }
      if (mux == 18) {
        PCS2_dcdcIntervalMaxFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                        (rx_frame.data.u8[1] & (0xFFU));  // m18 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS2_dcdcIntervalMaxHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                        (rx_frame.data.u8[3] & (0xFFU));  //m18 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS2_dcdcIntervalMaxLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                        ((rx_frame.data.u8[4] >> 5) & (0x07U));  // m18 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS2_dcdcIntervalMaxLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                           (rx_frame.data.u8[6] & (0xFFU));  //m18 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 19) {
        PCS2_dcdcIntervalMinFrequency = ((rx_frame.data.u8[2] & (0x0FU)) << 8) |
                                        (rx_frame.data.u8[1] & (0xFFU));  //m19 : 8|12@1+ (1,0) [0|0] "kHz"  X
        PCS2_dcdcIntervalMinHvBusVolt = ((rx_frame.data.u8[4] & (0x1FU)) << 8) |
                                        (rx_frame.data.u8[3] & (0xFFU));  //m19 : 24|13@1+ (0.1,0) [0|0] "V"  X
        PCS2_dcdcIntervalMinLvBusVolt = ((rx_frame.data.u8[5] & (0x3FU)) << 3) |
                                        ((rx_frame.data.u8[4] >> 5) & (0x07U));  //m19 : 37|9@1+ (0.1,0) [0|0] "V"  X
        PCS2_dcdcIntervalMinLvOutputCurr = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                                           (rx_frame.data.u8[6] & (0xFFU));  // m19 : 48|12@1+ (1,0) [0|0] "A"  X
      }
      if (mux == 22) {
        PCS2_dcdc12vSupportLifetimekWh = ((rx_frame.data.u8[3] & (0xFFU)) << 16) |
                                         ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                         (rx_frame.data.u8[1] & (0xFFU));  // m22 : 8|24@1+ (0.01,0) [0|0] "kWh"  X
      }
      break;
    case 0x401:  // Cell stats
      mux = (rx_frame.data.u8[0]);

      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

      if (rx_frame.data.u8[1] == 0x2A)  // status byte must be 0x2A to read cellvoltages
      {
        // Example, frame3=0x89,frame2=0x1D = 35101 / 10 = 3510mV
        volts = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 10;
        datalayer.battery.status.cell_voltages_mV[mux * 3] = volts;
        volts = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) / 10;
        datalayer.battery.status.cell_voltages_mV[1 + mux * 3] = volts;
        volts = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) / 10;
        datalayer.battery.status.cell_voltages_mV[2 + mux * 3] = volts;

        // Track the max value of mux. If we've seen two 0 values for mux, we've probably gathered all
        // cell voltages. Then, 2 + mux_max * 3 + 1 is the number of cell voltages.
        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = 2 + 3 * mux_max + 1;
            // Increase the counter arbitrarily another time to make the initial if-statement evaluate to false
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x2d2:  //BMSVAlimits:
      //Min / max limits
      battery2_bms_min_voltage =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);  //Example 24148mv * 0.01 = 241.48 V
      battery2_bms_max_voltage =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);  //Example 40282mv * 0.01 = 402.82 V
      battery2_max_charge_current =
          (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1;  //Example 1301? * 0.1 = 130.1?
      battery2_max_discharge_current =
          (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128;  //Example 430? * 0.128 = 55.4?
      break;
    case 0x2b4:  //PCS_dcdcRailStatus:
      battery2_dcdcLvBusVolt = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      battery2_dcdcHvBusVolt = ((((rx_frame.data.u8[2] & 0x3F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2)));
      battery2_dcdcLvOutputCurrent = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[3]);
      break;
    case 0x292:                                                             //BMS_socStatus
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  //We are getting CAN messages from the BMS
      battery2_beginning_of_life = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[5]) * 0.1;
      battery2_soc_min = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      battery2_soc_ui = (((rx_frame.data.u8[2] & 0x0F) << 6) | ((rx_frame.data.u8[1] & 0xFC) >> 2));
      battery2_soc_max = (((rx_frame.data.u8[3] & 0x3F) << 4) | ((rx_frame.data.u8[2] & 0xF0) >> 4));
      battery2_soc_ave = ((rx_frame.data.u8[4] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6));
      battery2_battTempPct = ((rx_frame.data.u8[7] & 0x03) << 6) | (rx_frame.data.u8[6] & (0x3F) >> 2);
      break;
    case 0x392:  //BMS_packConfig
      mux = (rx_frame.data.u8[0] & (0xFF));
      if (mux == 1) {
        battery2_packConfigMultiplexer = (rx_frame.data.u8[0] & (0xff));
        battery2_moduleType = (rx_frame.data.u8[1] & (0x07));
        battery2_packMass = (rx_frame.data.u8[2]);
        battery2_platformMaxBusVoltage = ((rx_frame.data.u8[4] & 0x03) << 8) | (rx_frame.data.u8[3]);
      }
      if (mux == 0) {
        battery2_reservedConfig = (rx_frame.data.u8[1] & (0x1F));
      }
      break;
    case 0x7AA:  //1962 HVP_debugMessage:
      mux = (rx_frame.data.u8[0] & (0x0FU));
      //HVP_debugMessageMultiplexer = (rx_frame.data.u8[0] & (0x0FU));  //0|4@1+ (1,0) [0|6] ""
      if (mux == 0) {
        HVP2_gpioPassivePyroDepl = ((rx_frame.data.u8[0] >> 4) & (0x01U));       //: 4|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPyroIsoEn = ((rx_frame.data.u8[0] >> 5) & (0x01U));             //: 5|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioCpFaultIn = ((rx_frame.data.u8[0] >> 6) & (0x01U));             //: 6|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPackContPowerEn = ((rx_frame.data.u8[0] >> 7) & (0x01U));       //: 7|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioHvCablesOk = (rx_frame.data.u8[1] & (0x01U));                   //: 8|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioHvpSelfEnable = ((rx_frame.data.u8[1] >> 1) & (0x01U));         //: 9|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioLed = ((rx_frame.data.u8[1] >> 2) & (0x01U));                   //: 10|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioCrashSignal = ((rx_frame.data.u8[1] >> 3) & (0x01U));           //: 11|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioShuntDataReady = ((rx_frame.data.u8[1] >> 4) & (0x01U));        //: 12|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioFcContPosAux = ((rx_frame.data.u8[1] >> 5) & (0x01U));          //: 13|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioFcContNegAux = ((rx_frame.data.u8[1] >> 6) & (0x01U));          //: 14|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioBmsEout = ((rx_frame.data.u8[1] >> 7) & (0x01U));               //: 15|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioCpFaultOut = (rx_frame.data.u8[2] & (0x01U));                   //: 16|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPyroPor = ((rx_frame.data.u8[2] >> 1) & (0x01U));               //: 17|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioShuntEn = ((rx_frame.data.u8[2] >> 2) & (0x01U));               //: 18|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioHvpVerEn = ((rx_frame.data.u8[2] >> 3) & (0x01U));              //: 19|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPackCoontPosFlywheel = ((rx_frame.data.u8[2] >> 4) & (0x01U));  //: 20|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioCpLatchEnable = ((rx_frame.data.u8[2] >> 5) & (0x01U));         //: 21|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPcsEnable = ((rx_frame.data.u8[2] >> 6) & (0x01U));             //: 22|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPcsDcdcPwmEnable = ((rx_frame.data.u8[2] >> 7) & (0x01U));      //: 23|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioPcsChargePwmEnable = (rx_frame.data.u8[3] & (0x01U));           //: 24|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioFcContPowerEnable = ((rx_frame.data.u8[3] >> 1) & (0x01U));     //: 25|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioHvilEnable = ((rx_frame.data.u8[3] >> 2) & (0x01U));            //: 26|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_gpioSecDrdy = ((rx_frame.data.u8[3] >> 3) & (0x01U));               //: 27|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_hvp1v5Ref = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                         ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|3] "V"  Receiver
        HVP2_shuntCurrentDebug = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                                 (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP2_packCurrentMia = (rx_frame.data.u8[7] & (0x01U));     //: 56|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_auxCurrentMia = ((rx_frame.data.u8[7] >> 1) & (0x01U));            //: 57|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_currentSenseMia = ((rx_frame.data.u8[7] >> 2) & (0x03U));          //: 58|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_shuntRefVoltageMismatch = ((rx_frame.data.u8[7] >> 3) & (0x01U));  //: 59|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_shuntThermistorMia = ((rx_frame.data.u8[7] >> 4) & (0x01U));       //: 60|1@1+ (1,0) [0|1] ""  Receiver
        HVP2_shuntHwMia = ((rx_frame.data.u8[7] >> 5) & (0x01U));               //: 61|1@1+ (1,0) [0|1] ""  Receiver
      }
      if (mux == 1) {
        HVP2_dcLinkVoltage = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                             (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP2_packVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                           (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
        HVP2_fcLinkVoltage = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                             (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-3276.8|3276.7] "V"  Receiver
      }
      if (mux == 2) {
        HVP2_packContVoltage = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                               ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP2_packNegativeV = ((rx_frame.data.u8[3] & (0xFFU)) << 8) |
                             (rx_frame.data.u8[2] & (0xFFU));  //: 16|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP2_packPositiveV = ((rx_frame.data.u8[5] & (0xFFU)) << 8) |
                             (rx_frame.data.u8[4] & (0xFFU));  //: 32|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP2_pyroAnalog = ((rx_frame.data.u8[7] & (0x0FU)) << 8) |
                          (rx_frame.data.u8[6] & (0xFFU));  //: 48|12@1+ (0.1,0) [0|3] "V"  Receiver
      }
      if (mux == 3) {
        HVP2_dcLinkNegativeV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP2_dcLinkPositiveV = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP2_fcLinkNegativeV = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.1,0) [-550|550] "V"  Receiver
      }
      if (mux == 4) {
        HVP2_fcContCoilCurrent = ((rx_frame.data.u8[1] & (0xFFU)) << 4) |
                                 ((rx_frame.data.u8[0] >> 4) & (0x0FU));  //: 4|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP2_fcContVoltage = ((rx_frame.data.u8[3] & (0x0FU)) << 8) |
                             (rx_frame.data.u8[2] & (0xFFU));  //: 16|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP2_hvilInVoltage = ((rx_frame.data.u8[4] & (0xFFU)) << 4) |
                             ((rx_frame.data.u8[3] >> 4) & (0x0FU));  //: 28|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP2_hvilOutVoltage = ((rx_frame.data.u8[6] & (0x0FU)) << 8) |
                              (rx_frame.data.u8[5] & (0xFFU));  //: 40|12@1+ (0.1,0) [0|30] "V"  Receiver
      }
      if (mux == 5) {
        HVP2_fcLinkPositiveV = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-550|550] "V"  Receiver
        HVP2_packContCoilCurrent = ((rx_frame.data.u8[4] & (0x0FU)) << 8) |
                                   (rx_frame.data.u8[3] & (0xFFU));  //: 24|12@1+ (0.1,0) [0|7.5] "A"  Receiver
        HVP2_battery12V = ((rx_frame.data.u8[5] & (0xFFU)) << 4) |
                          ((rx_frame.data.u8[4] >> 4) & (0x0FU));  //: 36|12@1+ (0.1,0) [0|30] "V"  Receiver
        HVP2_shuntRefVoltageDbg =
            ((rx_frame.data.u8[7] & (0xFFU)) << 8) |
            (rx_frame.data.u8[6] & (0xFFU));  //: 48|16@1- (0.001,0) [-32.768|32.767] "V"  Receiver
      }
      if (mux == 6) {
        HVP2_shuntAuxCurrentDbg = ((rx_frame.data.u8[2] & (0xFFU)) << 8) |
                                  (rx_frame.data.u8[1] & (0xFFU));  //: 8|16@1- (0.1,0) [-3276.8|3276.7] "A"  Receiver
        HVP2_shuntBarTempDbg = ((rx_frame.data.u8[4] & (0xFFU)) << 8) |
                               (rx_frame.data.u8[3] & (0xFFU));  //: 24|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP2_shuntAsicTempDbg = ((rx_frame.data.u8[6] & (0xFFU)) << 8) |
                                (rx_frame.data.u8[5] & (0xFFU));  //: 40|16@1- (0.01,0) [-327.67|327.67] "C"  Receiver
        HVP2_shuntAuxCurrentStatus = (rx_frame.data.u8[7] & (0x03U));       //: 56|2@1+ (1,0) [0|3] ""  Receiver
        HVP2_shuntBarTempStatus = ((rx_frame.data.u8[7] >> 2) & (0x03U));   //: 58|2@1+ (1,0) [0|3] ""  Receiver
        HVP2_shuntAsicTempStatus = ((rx_frame.data.u8[7] >> 4) & (0x03U));  //: 60|2@1+ (1,0) [0|3] ""  Receiver
      }
      break;
    case 0x3aa:  //HVP_alertMatrix1
      battery2_WatchdogReset = (rx_frame.data.u8[0] & 0x01);
      battery2_PowerLossReset = ((rx_frame.data.u8[0] & 0x02) >> 1);
      battery2_SwAssertion = ((rx_frame.data.u8[0] & 0x04) >> 2);
      battery2_CrashEvent = ((rx_frame.data.u8[0] & 0x08) >> 3);
      battery2_OverDchgCurrentFault = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery2_OverChargeCurrentFault = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery2_OverCurrentFault = ((rx_frame.data.u8[0] & 0x40) >> 6);
      battery2_OverTemperatureFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery2_OverVoltageFault = (rx_frame.data.u8[1] & 0x01);
      battery2_UnderVoltageFault = ((rx_frame.data.u8[1] & 0x02) >> 1);
      battery2_PrimaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x04) >> 2);
      battery2_SecondaryBmbMiaFault = ((rx_frame.data.u8[1] & 0x08) >> 3);
      battery2_BmbMismatchFault = ((rx_frame.data.u8[1] & 0x10) >> 4);
      battery2_BmsHviMiaFault = ((rx_frame.data.u8[1] & 0x20) >> 5);
      battery2_CpMiaFault = ((rx_frame.data.u8[1] & 0x40) >> 6);
      battery2_PcsMiaFault = ((rx_frame.data.u8[1] & 0x80) >> 7);
      battery2_BmsFault = (rx_frame.data.u8[2] & 0x01);
      battery2_PcsFault = ((rx_frame.data.u8[2] & 0x02) >> 1);
      battery2_CpFault = ((rx_frame.data.u8[2] & 0x04) >> 2);
      battery2_ShuntHwMiaFault = ((rx_frame.data.u8[2] & 0x08) >> 3);
      battery2_PyroMiaFault = ((rx_frame.data.u8[2] & 0x10) >> 4);
      battery2_hvsMiaFault = ((rx_frame.data.u8[2] & 0x20) >> 5);
      battery2_hviMiaFault = ((rx_frame.data.u8[2] & 0x40) >> 6);
      battery2_Supply12vFault = ((rx_frame.data.u8[2] & 0x80) >> 7);
      battery2_VerSupplyFault = (rx_frame.data.u8[3] & 0x01);
      battery2_HvilFault = ((rx_frame.data.u8[3] & 0x02) >> 1);
      battery2_BmsHvsMiaFault = ((rx_frame.data.u8[3] & 0x04) >> 2);
      battery2_PackVoltMismatchFault = ((rx_frame.data.u8[3] & 0x08) >> 3);
      battery2_EnsMiaFault = ((rx_frame.data.u8[3] & 0x10) >> 4);
      battery2_PackPosCtrArcFault = ((rx_frame.data.u8[3] & 0x20) >> 5);
      battery2_packNegCtrArcFault = ((rx_frame.data.u8[3] & 0x40) >> 6);
      battery2_ShuntHwAndBmsMiaFault = ((rx_frame.data.u8[3] & 0x80) >> 7);
      battery2_fcContHwFault = (rx_frame.data.u8[4] & 0x01);
      battery2_robinOverVoltageFault = ((rx_frame.data.u8[4] & 0x02) >> 1);
      battery2_packContHwFault = ((rx_frame.data.u8[4] & 0x04) >> 2);
      battery2_pyroFuseBlown = ((rx_frame.data.u8[4] & 0x08) >> 3);
      battery2_pyroFuseFailedToBlow = ((rx_frame.data.u8[4] & 0x10) >> 4);
      battery2_CpilFault = ((rx_frame.data.u8[4] & 0x20) >> 5);
      battery2_PackContactorFellOpen = ((rx_frame.data.u8[4] & 0x40) >> 6);
      battery2_FcContactorFellOpen = ((rx_frame.data.u8[4] & 0x80) >> 7);
      battery2_packCtrCloseBlocked = (rx_frame.data.u8[5] & 0x01);
      battery2_fcCtrCloseBlocked = ((rx_frame.data.u8[5] & 0x02) >> 1);
      battery2_packContactorForceOpen = ((rx_frame.data.u8[5] & 0x04) >> 2);
      battery2_fcContactorForceOpen = ((rx_frame.data.u8[5] & 0x08) >> 3);
      battery2_dcLinkOverVoltage = ((rx_frame.data.u8[5] & 0x10) >> 4);
      battery2_shuntOverTemperature = ((rx_frame.data.u8[5] & 0x20) >> 5);
      battery2_passivePyroDeploy = ((rx_frame.data.u8[5] & 0x40) >> 6);
      battery2_logUploadRequest = ((rx_frame.data.u8[5] & 0x80) >> 7);
      battery2_packCtrCloseFailed = (rx_frame.data.u8[6] & 0x01);
      battery2_fcCtrCloseFailed = ((rx_frame.data.u8[6] & 0x02) >> 1);
      battery2_shuntThermistorMia = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;
    case 0x320:  //800 BMS_alertMatrix                                                //BMS_alertMatrix 800 BMS_alertMatrix: 8 VEH
      mux = (rx_frame.data.u8[0] & (0x0F));
      if (mux == 0)
        ;
      {                                                                                    //mux0
        battery2_BMS_matrixIndex = (rx_frame.data.u8[0] & (0x0F));                         // 0|4@1+ (1,0) [0|0] ""  X
        battery2_BMS_a017_SW_Brick_OV = ((rx_frame.data.u8[2] >> 4) & (0x01));             //20|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a018_SW_Brick_UV = ((rx_frame.data.u8[2] >> 5) & (0x01));             //21|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a019_SW_Module_OT = ((rx_frame.data.u8[2] >> 6) & (0x01));            //22|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a021_SW_Dr_Limits_Regulation = (rx_frame.data.u8[3] & (0x01U));       //24|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a022_SW_Over_Current = ((rx_frame.data.u8[3] >> 1) & (0x01U));        //25|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a023_SW_Stack_OV = ((rx_frame.data.u8[3] >> 2) & (0x01U));            //26|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a024_SW_Islanded_Brick = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a025_SW_PwrBalance_Anomaly = ((rx_frame.data.u8[3] >> 4) & (0x01U));  //28|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a026_SW_HFCurrent_Anomaly = ((rx_frame.data.u8[3] >> 5) & (0x01U));   //29|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a034_SW_Passive_Isolation = ((rx_frame.data.u8[4] >> 5) & (0x01U));   //37|1@1+ (1,0) [0|0] ""  X ?
        battery2_BMS_a035_SW_Isolation = ((rx_frame.data.u8[4] >> 6) & (0x01U));           //38|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a036_SW_HvpHvilFault = ((rx_frame.data.u8[4] >> 6) & (0x01U));        //39|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a037_SW_Flood_Port_Open = (rx_frame.data.u8[5] & (0x01U));            //40|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a039_SW_DC_Link_Over_Voltage = ((rx_frame.data.u8[5] >> 2) & (0x01U));  //42|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a041_SW_Power_On_Reset = ((rx_frame.data.u8[5] >> 4) & (0x01U));        //44|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a042_SW_MPU_Error = ((rx_frame.data.u8[5] >> 5) & (0x01U));             //45|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a043_SW_Watch_Dog_Reset = ((rx_frame.data.u8[5] >> 6) & (0x01U));       //46|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a044_SW_Assertion = ((rx_frame.data.u8[5] >> 7) & (0x01U));             //47|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a045_SW_Exception = (rx_frame.data.u8[6] & (0x01U));                    //48|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a046_SW_Task_Stack_Usage = ((rx_frame.data.u8[6] >> 1) & (0x01U));      //49|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a047_SW_Task_Stack_Overflow = ((rx_frame.data.u8[6] >> 2) & (0x01U));   //50|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a048_SW_Log_Upload_Request = ((rx_frame.data.u8[6] >> 3) & (0x01U));    //51|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a050_SW_Brick_Voltage_MIA = ((rx_frame.data.u8[6] >> 5) & (0x01U));     //53|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a051_SW_HVC_Vref_Bad = ((rx_frame.data.u8[6] >> 6) & (0x01U));          //54|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a052_SW_PCS_MIA = ((rx_frame.data.u8[6] >> 7) & (0x01U));               //55|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a053_SW_ThermalModel_Sanity = (rx_frame.data.u8[7] & (0x01U));          //56|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a054_SW_Ver_Supply_Fault = ((rx_frame.data.u8[7] >> 1) & (0x01U));      //57|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a059_SW_Pack_Voltage_Sensing = ((rx_frame.data.u8[7] >> 6) & (0x01U));  //62|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a060_SW_Leakage_Test_Failure = ((rx_frame.data.u8[7] >> 7) & (0x01U));  //63|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 1) {                                                                        //mux1
        battery2_BMS_a061_robinBrickOverVoltage = ((rx_frame.data.u8[0] >> 4) & (0x01U));    //4|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a062_SW_BrickV_Imbalance = ((rx_frame.data.u8[0] >> 5) & (0x01U));      //5|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a063_SW_ChargePort_Fault = ((rx_frame.data.u8[0] >> 6) & (0x01U));      //6|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a064_SW_SOC_Imbalance = ((rx_frame.data.u8[0] >> 7) & (0x01U));         //7|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a069_SW_Low_Power = ((rx_frame.data.u8[1] >> 4) & (0x01U));             //12|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a071_SW_SM_TransCon_Not_Met = ((rx_frame.data.u8[1] >> 6) & (0x01U));   //14|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a075_SW_Chg_Disable_Failure = ((rx_frame.data.u8[2] >> 2) & (0x01U));   //18|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a076_SW_Dch_While_Charging = ((rx_frame.data.u8[2] >> 3) & (0x01U));    //19|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a077_SW_Charger_Regulation = ((rx_frame.data.u8[2] >> 4) & (0x01U));    //20|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a081_SW_Ctr_Close_Blocked = (rx_frame.data.u8[3] & (0x01U));            //24|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a082_SW_Ctr_Force_Open = ((rx_frame.data.u8[3] >> 1) & (0x01U));        //25|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a083_SW_Ctr_Close_Failure = ((rx_frame.data.u8[3] >> 2) & (0x01U));     //26|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a084_SW_Sleep_Wake_Aborted = ((rx_frame.data.u8[3] >> 3) & (0x01U));    //27|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a087_SW_Feim_Test_Blocked = ((rx_frame.data.u8[3] >> 6) & (0x01U));     //30|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a088_SW_VcFront_MIA_InDrive = ((rx_frame.data.u8[3] >> 7) & (0x01U));   //31|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a089_SW_VcFront_MIA = (rx_frame.data.u8[4] & (0x01U));                  //32|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a090_SW_Gateway_MIA = ((rx_frame.data.u8[4] >> 1) & (0x01U));           //33|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a091_SW_ChargePort_MIA = ((rx_frame.data.u8[4] >> 2) & (0x01U));        //34|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a092_SW_ChargePort_Mia_On_Hv = ((rx_frame.data.u8[4] >> 3) & (0x01U));  //35|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a094_SW_Drive_Inverter_MIA = ((rx_frame.data.u8[4] >> 5) & (0x01U));    //37|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a099_SW_BMB_Communication = ((rx_frame.data.u8[5] >> 2) & (0x01U));     //42|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a105_SW_One_Module_Tsense = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a106_SW_All_Module_Tsense = ((rx_frame.data.u8[6] >> 1) & (0x01U));     //49|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a107_SW_Stack_Voltage_MIA = ((rx_frame.data.u8[6] >> 2) & (0x01U));     //50|1@1+ (1,0) [0|0] ""  X
      }
      if (mux == 2) {                                                                        //mux2
        battery2_BMS_a121_SW_NVRAM_Config_Error = ((rx_frame.data.u8[0] >> 4) & (0x01U));    // 4|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a122_SW_BMS_Therm_Irrational = ((rx_frame.data.u8[0] >> 5) & (0x01U));  //5|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a123_SW_Internal_Isolation = ((rx_frame.data.u8[0] >> 6) & (0x01U));    //6|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a127_SW_shunt_SNA = ((rx_frame.data.u8[1] >> 2) & (0x01U));             //10|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a128_SW_shunt_MIA = ((rx_frame.data.u8[1] >> 3) & (0x01U));             //11|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a129_SW_VSH_Failure = ((rx_frame.data.u8[1] >> 4) & (0x01U));           //12|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a130_IO_CAN_Error = ((rx_frame.data.u8[1] >> 5) & (0x01U));             //13|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a131_Bleed_FET_Failure = ((rx_frame.data.u8[1] >> 6) & (0x01U));        //14|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a132_HW_BMB_OTP_Uncorrctbl = ((rx_frame.data.u8[1] >> 7) & (0x01U));    //15|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a134_SW_Delayed_Ctr_Off = ((rx_frame.data.u8[2] >> 1) & (0x01U));       //17|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a136_SW_Module_OT_Warning = ((rx_frame.data.u8[2] >> 3) & (0x01U));     //19|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a137_SW_Brick_UV_Warning = ((rx_frame.data.u8[2] >> 4) & (0x01U));      //20|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a138_SW_Brick_OV_Warning = ((rx_frame.data.u8[2] >> 5) & (0x01U));      //21|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a139_SW_DC_Link_V_Irrational = ((rx_frame.data.u8[2] >> 6) & (0x01U));  //22|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a141_SW_BMB_Status_Warning = (rx_frame.data.u8[3] & (0x01U));           //24|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a144_Hvp_Config_Mismatch = ((rx_frame.data.u8[3] >> 3) & (0x01U));      //27|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a145_SW_SOC_Change = ((rx_frame.data.u8[3] >> 4) & (0x01U));            //28|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a146_SW_Brick_Overdischarged = ((rx_frame.data.u8[3] >> 5) & (0x01U));  //29|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a149_SW_Missing_Config_Block = (rx_frame.data.u8[4] & (0x01U));         //32|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a151_SW_external_isolation = ((rx_frame.data.u8[4] >> 2) & (0x01U));    //34|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a156_SW_BMB_Vref_bad = ((rx_frame.data.u8[4] >> 7) & (0x01U));          //39|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a157_SW_HVP_HVS_Comms = (rx_frame.data.u8[5] & (0x01U));                //40|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a158_SW_HVP_HVI_Comms = ((rx_frame.data.u8[5] >> 1) & (0x01U));         //41|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a159_SW_HVP_ECU_Error = ((rx_frame.data.u8[5] >> 2) & (0x01U));         //42|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a161_SW_DI_Open_Request = ((rx_frame.data.u8[5] >> 4) & (0x01U));       //44|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a162_SW_No_Power_For_Support = ((rx_frame.data.u8[5] >> 5) & (0x01U));  //45|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a163_SW_Contactor_Mismatch = ((rx_frame.data.u8[5] >> 6) & (0x01U));    //46|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a164_SW_Uncontrolled_Regen = ((rx_frame.data.u8[5] >> 7) & (0x01U));    //47|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a165_SW_Pack_Partial_Weld = (rx_frame.data.u8[6] & (0x01U));            //48|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a166_SW_Pack_Full_Weld = ((rx_frame.data.u8[6] >> 1) & (0x01U));        //49|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a167_SW_FC_Partial_Weld = ((rx_frame.data.u8[6] >> 2) & (0x01U));       //50|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a168_SW_FC_Full_Weld = ((rx_frame.data.u8[6] >> 3) & (0x01U));          //51|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a169_SW_FC_Pack_Weld = ((rx_frame.data.u8[6] >> 4) & (0x01U));          //52|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a170_SW_Limp_Mode = ((rx_frame.data.u8[6] >> 5) & (0x01U));             //53|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a171_SW_Stack_Voltage_Sense = ((rx_frame.data.u8[6] >> 6) & (0x01U));   //54|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a174_SW_Charge_Failure = ((rx_frame.data.u8[7] >> 1) & (0x01U));        //57|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a176_SW_GracefulPowerOff = ((rx_frame.data.u8[7] >> 3) & (0x01U));      //59|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a179_SW_Hvp_12V_Fault = ((rx_frame.data.u8[7] >> 6) & (0x01U));         //62|1@1+ (1,0) [0|0] ""  X
        battery2_BMS_a180_SW_ECU_reset_blocked = ((rx_frame.data.u8[7] >> 7) & (0x01U));     //63|1@1+ (1,0) [0|0] ""  X
      }
      break;
    default:
      break;
  }
}

void update_values_battery2() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery2.status.soh_pptt = 9900;  //Tesla batteries do not send a SOH% value on bus. Hardcode to 99%

  datalayer.battery2.status.real_soc = (battery2_soc_ui * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery2.status.voltage_dV = (battery2_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery2.status.current_dA = battery2_amps;  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery2.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery2.status.real_soc) / 10000) * datalayer.battery2.info.total_capacity_Wh);

  // Define the allowed discharge power
  datalayer.battery2.status.max_discharge_power_W = (battery2_max_discharge_current * battery2_volts);
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery2.status.max_discharge_power_W > MAXDISCHARGEPOWERALLOWED) {
    datalayer.battery2.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery2_soc_ui > 990) {
    datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery2_soc_ui >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery2.status.max_charge_power_W =
        MAXCHARGEPOWERALLOWED * (1 - (battery2_soc_ui - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
      if (battery2_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
        datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    } else {  //NCM/A
      if (battery2_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
        datalayer.battery2.status.max_charge_power_W = FLOAT_MAX_POWER_W;
      }
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery2.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }

  datalayer.battery2.status.temperature_min_dC = battery2_min_temp;

  datalayer.battery2.status.temperature_max_dC = battery2_max_temp;

  datalayer.battery2.status.cell_max_voltage_mV = battery2_cell_max_v;

  datalayer.battery2.status.cell_min_voltage_mV = battery2_cell_min_v;

  battery2_cell_deviation_mV = (battery2_cell_max_v - battery2_cell_min_v);

  /* Value mapping is completed. Start to check all safeties */

  if (battery2_hvil_status ==
      3) {  //INTERNAL_OPEN_FAULT - Someone disconnected a high voltage cable while battery was in use
    set_event(EVENT_INTERNAL_OPEN_FAULT, 2);
  } else {
    clear_event(EVENT_INTERNAL_OPEN_FAULT);
  }

#ifdef TESLA_MODEL_3Y_BATTERY
  // Autodetect algoritm for chemistry on 3/Y packs.
  // NCM/A batteries have 96s, LFP has 102-106s
  // Drawback with this check is that it takes 3-5minutes before all cells have been counted!
  if (datalayer.battery2.info.number_of_cells > 101) {
    datalayer.battery2.info.chemistry = battery_chemistry_enum::LFP;
  }

  //Once cell chemistry is determined, set maximum and minimum total pack voltage safety limits
  if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
    datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
    datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
    datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
  } else {  // NCM/A chemistry
    datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
    datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
    datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
  }
#endif  // TESLA_MODEL_3Y_BATTERY

#ifdef DEBUG_LOG

  printFaultCodesIfActive_battery2();

  logging.print("STATUS: Contactor: ");
  logging.print(contactorText[battery2_contactor]);  //Display what state the contactor is in
  logging.print(", HVIL: ");
  logging.print(hvilStatusState[battery2_hvil_status]);
  logging.print(", NegativeState: ");
  logging.print(contactorState[battery2_packContNegativeState]);
  logging.print(", PositiveState: ");
  logging.print(contactorState[battery2_packContPositiveState]);
  logging.print(", setState: ");
  logging.print(contactorState[battery2_packContactorSetState]);
  logging.print(", close allowed: ");
  logging.print(battery2_packCtrsClosingAllowed);
  logging.print(", Pyrotest: ");
  logging.println(battery2_pyroTestInProgress);

  logging.print("Battery2 values: ");
  logging.print("Real SOC: ");
  logging.print(battery2_soc_ui / 10.0, 1);
  print_int_with_units(", Battery2 voltage: ", battery2_volts, "V");
  print_int_with_units(", Battery2 HV current: ", (battery2_amps * 0.1), "A");
  logging.print(", Fully charged?: ");
  if (battery2_full_charge_complete)
    logging.print("YES, ");
  else
    logging.print("NO, ");
  if (datalayer.battery2.info.chemistry == battery_chemistry_enum::LFP) {
    logging.print("LFP chemistry detected!");
  }
  logging.println("");
  logging.print("Cellstats, Max: ");
  logging.print(battery2_cell_max_v);
  logging.print("mV (cell ");
  logging.print(battery2_max_vno);
  logging.print("), Min: ");
  logging.print(battery2_cell_min_v);
  logging.print("mV (cell ");
  logging.print(battery2_min_vno);
  logging.print("), Imbalance: ");
  logging.print(battery2_cell_deviation_mV);
  logging.println("mV.");

  print_int_with_units("High Voltage Output Pins: ", battery2_dcdcHvBusVolt, "V");
  logging.print(", ");
  print_int_with_units("Low Voltage: ", battery2_dcdcLvBusVolt, "V");
  logging.println("");
  print_int_with_units("DC/DC 12V current: ", battery2_dcdcLvOutputCurrent, "A");
  logging.println("");

  logging.println("Values passed to the inverter: ");
  print_SOC(" SOC: ", datalayer.battery2.status.reported_soc);
  print_int_with_units(" Max discharge power: ", datalayer.battery2.status.max_discharge_power_W, "W");
  logging.print(", ");
  print_int_with_units(" Max charge power: ", datalayer.battery2.status.max_charge_power_W, "W");
  logging.println("");
  print_int_with_units(" Max temperature: ", ((int16_t)datalayer.battery2.status.temperature_min_dC * 0.1), "°C");
  logging.print(", ");
  print_int_with_units(" Min temperature: ", ((int16_t)datalayer.battery2.status.temperature_max_dC * 0.1), "°C");
  logging.println("");
#endif  // DEBUG_LOG
}

#endif  //DOUBLE_BATTERY

#if defined(TESLA_MODEL_SX_BATTERY) || defined(EXP_TESLA_BMS_DIGITAL_HVIL)
CAN_frame can_msg_1CF[] = {
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x60, 0x69}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x80, 0x89}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xA0, 0xA9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xC0, 0xC9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0xE0, 0xE9}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x00, 0x09}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x20, 0x29}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x1CF, .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x40, 0x49}}};

CAN_frame can_msg_118[] = {
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x61, 0x80, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x62, 0x81, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x63, 0x82, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x64, 0x83, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x65, 0x84, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x66, 0x85, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x67, 0x86, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x68, 0x87, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x69, 0x88, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6A, 0x89, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6B, 0x8A, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6C, 0x8B, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6D, 0x8C, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6E, 0x8D, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x6F, 0x8E, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}},
    {.FD = false, .ext_ID = false, .DLC = 8, .ID = 0x118, .data = {0x70, 0x8F, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}}};

unsigned long lastSend1CF = 0;
unsigned long lastSend118 = 0;

int index_1CF = 0;
int index_118 = 0;
#endif  //defined(TESLA_MODEL_SX_BATTERY) || defined(EXP_TESLA_BMS_DIGITAL_HVIL)

void transmit_can_battery() {
  /*From bielec: My fist 221 message, to close the contactors is 0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96 and then, 
to cause "hv_up_for_drive" I send an additional 221 message 0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA  so 
two 221 messages are being continuously transmitted.   When I want to shut down, I stop the second message and only send 
the first, for a few cycles, then stop all  messages which causes the contactor to open. */

  unsigned long currentMillis = millis();

  if (!cellvoltagesRead) {
    return;  //All cellvoltages not read yet, do not proceed with contactor closing
  }

#if defined(TESLA_MODEL_SX_BATTERY) || defined(EXP_TESLA_BMS_DIGITAL_HVIL)
  if ((datalayer.system.status.inverter_allows_contactor_closing) && (datalayer.battery.status.bms_status != FAULT)) {
    if (currentMillis - lastSend1CF >= 10) {
      transmit_can_frame(&can_msg_1CF[index_1CF], can_config.battery);

#ifdef DOUBLE_BATTERY
      transmit_can_frame(&can_msg_1CF[index_1CF], can_config.battery_double);
#endif  // DOUBLE_BATTERY

      index_1CF = (index_1CF + 1) % 8;
      lastSend1CF = currentMillis;
    }

    if (currentMillis - lastSend118 >= 10) {
      transmit_can_frame(&can_msg_118[index_118], can_config.battery);
#ifdef DOUBLE_BATTERY
      transmit_can_frame(&can_msg_1CF[index_1CF], can_config.battery_double);
#endif  //DOUBLE_BATTERY

      index_118 = (index_118 + 1) % 16;
      lastSend118 = currentMillis;
    }
  } else {
    index_1CF = 0;
    index_118 = 0;
  }
#endif  //defined(TESLA_MODEL_SX_BATTERY) || defined(EXP_TESLA_BMS_DIGITAL_HVIL)

  //Send 30ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis50 >= INTERVAL_50_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis50));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis50 = currentMillis;

    if ((datalayer.system.status.inverter_allows_contactor_closing == true) &&
        (datalayer.battery.status.bms_status != FAULT)) {
      sendContactorClosingMessagesStill = 300;
      transmit_can_frame(&TESLA_221_1, can_config.battery);
      transmit_can_frame(&TESLA_221_2, can_config.battery);
#ifdef DOUBLE_BATTERY
      if (datalayer.system.status.battery2_allows_contactor_closing) {
        transmit_can_frame(&TESLA_221_1, can_config.battery_double);
        transmit_can_frame(&TESLA_221_2, can_config.battery_double);
      }
#endif        //DOUBLE_BATTERY
    } else {  // Faulted state, or inverter blocks contactor closing
      if (sendContactorClosingMessagesStill > 0) {
        transmit_can_frame(&TESLA_221_1, can_config.battery);
        sendContactorClosingMessagesStill--;

#ifdef DOUBLE_BATTERY
        transmit_can_frame(&TESLA_221_1, can_config.battery_double);
#endif  //DOUBLE_BATTERY
      }
    }
  }
}

void print_int_with_units(char* header, int value, char* units) {
  logging.print(header);
  logging.print(value);
  logging.print(units);
}

void print_SOC(char* header, int SOC) {
  logging.print(header);
  logging.print(SOC / 100);
  logging.print(".");
  int hundredth = SOC % 100;
  if (hundredth < 10)
    logging.print(0);
  logging.print(hundredth);
  logging.println("%");
}

void printFaultCodesIfActive() {
  if (battery_packCtrsClosingAllowed == 0) {
    logging.println(
        "ERROR: Check high voltage connectors and interlock circuit! Closing contactor not allowed! Values: ");
  }
  if (battery_pyroTestInProgress == 1) {
    logging.println("ERROR: Please wait for Pyro Connection check to finish, HV cables successfully seated!");
  }
  if (datalayer.system.status.inverter_allows_contactor_closing == false) {
    logging.println(
        "ERROR: Solar inverter does not allow for contactor closing. Check communication connection to the inverter "
        "OR "
        "disable the inverter protocol to proceed with contactor closing");
  }
  // Check each symbol and print debug information if its value is 1
  // 0X3AA: 938 HVP_alertMatrix1
  printDebugIfActive(battery_WatchdogReset, "ERROR: The processor has experienced a reset due to watchdog reset");
  printDebugIfActive(battery_PowerLossReset, "ERROR: The processor has experienced a reset due to power loss");
  printDebugIfActive(battery_SwAssertion, "ERROR: An internal software assertion has failed");
  printDebugIfActive(battery_CrashEvent, "ERROR: crash signal is detected by HVP");
  printDebugIfActive(battery_OverDchgCurrentFault,
                     "ERROR: Pack discharge current is above the safe max discharge current limit!");
  printDebugIfActive(battery_OverChargeCurrentFault,
                     "ERROR: Pack charge current is above the safe max charge current limit!");
  printDebugIfActive(battery_OverCurrentFault, "ERROR: Pack current (discharge or charge) is above max current limit!");
  printDebugIfActive(battery_OverTemperatureFault,
                     "ERROR: A pack module temperature is above the max temperature limit!");
  printDebugIfActive(battery_OverVoltageFault, "ERROR: A brick voltage is above maximum voltage limit");
  printDebugIfActive(battery_UnderVoltageFault, "ERROR: A brick voltage is below minimum voltage limit");
  printDebugIfActive(battery_PrimaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from primary BMB chain are mia");
  printDebugIfActive(battery_SecondaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from secondary BMB chain are mia");
  printDebugIfActive(battery_BmbMismatchFault,
                     "ERROR: primary and secondary BMB chain readings don't match with each other");
  printDebugIfActive(battery_BmsHviMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  //printDebugIfActive(battery_CpMiaFault, "ERROR: CP node is mia on HVS CAN"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsMiaFault, "ERROR: PCS node is mia on HVS CAN");
  //printDebugIfActive(battery_BmsFault, "ERROR: BmsFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PcsFault, "ERROR: PcsFault is active");
  //printDebugIfActive(battery_CpFault, "ERROR: CpFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_ShuntHwMiaFault, "ERROR: shunt current reading is not available");
  printDebugIfActive(battery_PyroMiaFault, "ERROR: pyro squib is not connected");
  printDebugIfActive(battery_hvsMiaFault, "ERROR: pack contactor hw fault");
  printDebugIfActive(battery_hviMiaFault, "ERROR: FC contactor hw fault");
  printDebugIfActive(battery_Supply12vFault, "ERROR: Low voltage (12V) battery is below minimum voltage threshold");
  printDebugIfActive(battery_VerSupplyFault, "ERROR: Energy reserve voltage supply is below minimum voltage threshold");
  printDebugIfActive(battery_HvilFault, "ERROR: High Voltage Inter Lock fault is detected");
  printDebugIfActive(battery_BmsHvsMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  printDebugIfActive(battery_PackVoltMismatchFault,
                     "ERROR: Pack voltage doesn't match approximately with sum of brick voltages");
  //printDebugIfActive(battery_EnsMiaFault, "ERROR: ENS line is not connected to HVC"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackPosCtrArcFault, "ERROR: HVP detectes series arc at pack contactor");
  printDebugIfActive(battery_packNegCtrArcFault, "ERROR: HVP detectes series arc at FC contactor");
  printDebugIfActive(battery_ShuntHwAndBmsMiaFault, "ERROR: ShuntHwAndBmsMiaFault is active");
  printDebugIfActive(battery_fcContHwFault, "ERROR: fcContHwFault is active");
  printDebugIfActive(battery_robinOverVoltageFault, "ERROR: robinOverVoltageFault is active");
  printDebugIfActive(battery_packContHwFault, "ERROR: packContHwFault is active");
  printDebugIfActive(battery_pyroFuseBlown, "ERROR: pyroFuseBlown is active");
  printDebugIfActive(battery_pyroFuseFailedToBlow, "ERROR: pyroFuseFailedToBlow is active");
  //printDebugIfActive(battery_CpilFault, "ERROR: CpilFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery_PackContactorFellOpen, "ERROR: PackContactorFellOpen is active");
  printDebugIfActive(battery_FcContactorFellOpen, "ERROR: FcContactorFellOpen is active");
  printDebugIfActive(battery_packCtrCloseBlocked, "ERROR: packCtrCloseBlocked is active");
  printDebugIfActive(battery_fcCtrCloseBlocked, "ERROR: fcCtrCloseBlocked is active");
  printDebugIfActive(battery_packContactorForceOpen, "ERROR: packContactorForceOpen is active");
  printDebugIfActive(battery_fcContactorForceOpen, "ERROR: fcContactorForceOpen is active");
  printDebugIfActive(battery_dcLinkOverVoltage, "ERROR: dcLinkOverVoltage is active");
  printDebugIfActive(battery_shuntOverTemperature, "ERROR: shuntOverTemperature is active");
  printDebugIfActive(battery_passivePyroDeploy, "ERROR: passivePyroDeploy is active");
  printDebugIfActive(battery_logUploadRequest, "ERROR: logUploadRequest is active");
  printDebugIfActive(battery_packCtrCloseFailed, "ERROR: packCtrCloseFailed is active");
  printDebugIfActive(battery_fcCtrCloseFailed, "ERROR: fcCtrCloseFailed is active");
  printDebugIfActive(battery_shuntThermistorMia, "ERROR: shuntThermistorMia is active");
  // 0x320 800 BMS_alertMatrix
  printDebugIfActive(battery_BMS_a017_SW_Brick_OV, "ERROR: BMS_a017_SW_Brick_OV");
  printDebugIfActive(battery_BMS_a018_SW_Brick_UV, "ERROR: BMS_a018_SW_Brick_UV");
  printDebugIfActive(battery_BMS_a019_SW_Module_OT, "ERROR: BMS_a019_SW_Module_OT");
  printDebugIfActive(battery_BMS_a021_SW_Dr_Limits_Regulation, "ERROR: BMS_a021_SW_Dr_Limits_Regulation");
  printDebugIfActive(battery_BMS_a022_SW_Over_Current, "ERROR: BMS_a022_SW_Over_Current");
  printDebugIfActive(battery_BMS_a023_SW_Stack_OV, "ERROR: BMS_a023_SW_Stack_OV");
  printDebugIfActive(battery_BMS_a024_SW_Islanded_Brick, "ERROR: BMS_a024_SW_Islanded_Brick");
  printDebugIfActive(battery_BMS_a025_SW_PwrBalance_Anomaly, "ERROR: BMS_a025_SW_PwrBalance_Anomaly");
  printDebugIfActive(battery_BMS_a026_SW_HFCurrent_Anomaly, "ERROR: BMS_a026_SW_HFCurrent_Anomaly");
  printDebugIfActive(battery_BMS_a034_SW_Passive_Isolation, "ERROR: BMS_a034_SW_Passive_Isolation");
  printDebugIfActive(battery_BMS_a035_SW_Isolation, "ERROR: BMS_a035_SW_Isolation");
  printDebugIfActive(battery_BMS_a036_SW_HvpHvilFault, "ERROR: BMS_a036_SW_HvpHvilFault");
  printDebugIfActive(battery_BMS_a037_SW_Flood_Port_Open, "ERROR: BMS_a037_SW_Flood_Port_Open");
  printDebugIfActive(battery_BMS_a039_SW_DC_Link_Over_Voltage, "ERROR: BMS_a039_SW_DC_Link_Over_Voltage");
  printDebugIfActive(battery_BMS_a041_SW_Power_On_Reset, "ERROR: BMS_a041_SW_Power_On_Reset");
  printDebugIfActive(battery_BMS_a042_SW_MPU_Error, "ERROR: BMS_a042_SW_MPU_Error");
  printDebugIfActive(battery_BMS_a043_SW_Watch_Dog_Reset, "ERROR: BMS_a043_SW_Watch_Dog_Reset");
  printDebugIfActive(battery_BMS_a044_SW_Assertion, "ERROR: BMS_a044_SW_Assertion");
  printDebugIfActive(battery_BMS_a045_SW_Exception, "ERROR: BMS_a045_SW_Exception");
  printDebugIfActive(battery_BMS_a046_SW_Task_Stack_Usage, "ERROR: BMS_a046_SW_Task_Stack_Usage");
  printDebugIfActive(battery_BMS_a047_SW_Task_Stack_Overflow, "ERROR: BMS_a047_SW_Task_Stack_Overflow");
  printDebugIfActive(battery_BMS_a048_SW_Log_Upload_Request, "ERROR: BMS_a048_SW_Log_Upload_Request");
  printDebugIfActive(battery_BMS_a050_SW_Brick_Voltage_MIA, "ERROR: BMS_a050_SW_Brick_Voltage_MIA");
  printDebugIfActive(battery_BMS_a051_SW_HVC_Vref_Bad, "ERROR: BMS_a051_SW_HVC_Vref_Bad");
  printDebugIfActive(battery_BMS_a052_SW_PCS_MIA, "ERROR: BMS_a052_SW_PCS_MIA");
  printDebugIfActive(battery_BMS_a053_SW_ThermalModel_Sanity, "ERROR: BMS_a053_SW_ThermalModel_Sanity");
  printDebugIfActive(battery_BMS_a054_SW_Ver_Supply_Fault, "ERROR: BMS_a054_SW_Ver_Supply_Fault");
  printDebugIfActive(battery_BMS_a059_SW_Pack_Voltage_Sensing, "ERROR: BMS_a059_SW_Pack_Voltage_Sensing");
  printDebugIfActive(battery_BMS_a060_SW_Leakage_Test_Failure, "ERROR: BMS_a060_SW_Leakage_Test_Failure");
  printDebugIfActive(battery_BMS_a061_robinBrickOverVoltage, "ERROR: BMS_a061_robinBrickOverVoltage");
  printDebugIfActive(battery_BMS_a062_SW_BrickV_Imbalance, "ERROR: BMS_a062_SW_BrickV_Imbalance");
  printDebugIfActive(battery_BMS_a063_SW_ChargePort_Fault, "ERROR: BMS_a063_SW_ChargePort_Fault");
  printDebugIfActive(battery_BMS_a064_SW_SOC_Imbalance, "ERROR: BMS_a064_SW_SOC_Imbalance");
  printDebugIfActive(battery_BMS_a069_SW_Low_Power, "ERROR: BMS_a069_SW_Low_Power");
  printDebugIfActive(battery_BMS_a071_SW_SM_TransCon_Not_Met, "ERROR: BMS_a071_SW_SM_TransCon_Not_Met");
  printDebugIfActive(battery_BMS_a075_SW_Chg_Disable_Failure, "ERROR: BMS_a075_SW_Chg_Disable_Failure");
  printDebugIfActive(battery_BMS_a076_SW_Dch_While_Charging, "ERROR: BMS_a076_SW_Dch_While_Charging");
  printDebugIfActive(battery_BMS_a077_SW_Charger_Regulation, "ERROR: BMS_a077_SW_Charger_Regulation");
  printDebugIfActive(battery_BMS_a081_SW_Ctr_Close_Blocked, "ERROR: BMS_a081_SW_Ctr_Close_Blocked");
  printDebugIfActive(battery_BMS_a082_SW_Ctr_Force_Open, "ERROR: BMS_a082_SW_Ctr_Force_Open");
  printDebugIfActive(battery_BMS_a083_SW_Ctr_Close_Failure, "ERROR: BMS_a083_SW_Ctr_Close_Failure");
  printDebugIfActive(battery_BMS_a084_SW_Sleep_Wake_Aborted, "ERROR: BMS_a084_SW_Sleep_Wake_Aborted");
  printDebugIfActive(battery_BMS_a087_SW_Feim_Test_Blocked, "ERROR: BMS_a087_SW_Feim_Test_Blocked");
  printDebugIfActive(battery_BMS_a088_SW_VcFront_MIA_InDrive, "ERROR: BMS_a088_SW_VcFront_MIA_InDrive");
  printDebugIfActive(battery_BMS_a089_SW_VcFront_MIA, "ERROR: BMS_a089_SW_VcFront_MIA");
  printDebugIfActive(battery_BMS_a090_SW_Gateway_MIA, "ERROR: BMS_a090_SW_Gateway_MIA");
  printDebugIfActive(battery_BMS_a091_SW_ChargePort_MIA, "ERROR: BMS_a091_SW_ChargePort_MIA");
  printDebugIfActive(battery_BMS_a092_SW_ChargePort_Mia_On_Hv, "ERROR: BMS_a092_SW_ChargePort_Mia_On_Hv");
  printDebugIfActive(battery_BMS_a094_SW_Drive_Inverter_MIA, "ERROR: BMS_a094_SW_Drive_Inverter_MIA");
  printDebugIfActive(battery_BMS_a099_SW_BMB_Communication, "ERROR: BMS_a099_SW_BMB_Communication");
  printDebugIfActive(battery_BMS_a105_SW_One_Module_Tsense, "ERROR: BMS_a105_SW_One_Module_Tsense");
  printDebugIfActive(battery_BMS_a106_SW_All_Module_Tsense, "ERROR: BMS_a106_SW_All_Module_Tsense");
  printDebugIfActive(battery_BMS_a107_SW_Stack_Voltage_MIA, "ERROR: BMS_a107_SW_Stack_Voltage_MIA");
  printDebugIfActive(battery_BMS_a121_SW_NVRAM_Config_Error, "ERROR: BMS_a121_SW_NVRAM_Config_Error");
  printDebugIfActive(battery_BMS_a122_SW_BMS_Therm_Irrational, "ERROR: BMS_a122_SW_BMS_Therm_Irrational");
  printDebugIfActive(battery_BMS_a123_SW_Internal_Isolation, "ERROR: BMS_a123_SW_Internal_Isolation");
  printDebugIfActive(battery_BMS_a127_SW_shunt_SNA, "ERROR: BMS_a127_SW_shunt_SNA");
  printDebugIfActive(battery_BMS_a128_SW_shunt_MIA, "ERROR: BMS_a128_SW_shunt_MIA");
  printDebugIfActive(battery_BMS_a129_SW_VSH_Failure, "ERROR: BMS_a129_SW_VSH_Failure");
  printDebugIfActive(battery_BMS_a130_IO_CAN_Error, "ERROR: BMS_a130_IO_CAN_Error");
  printDebugIfActive(battery_BMS_a131_Bleed_FET_Failure, "ERROR: BMS_a131_Bleed_FET_Failure");
  printDebugIfActive(battery_BMS_a132_HW_BMB_OTP_Uncorrctbl, "ERROR: BMS_a132_HW_BMB_OTP_Uncorrctbl");
  printDebugIfActive(battery_BMS_a134_SW_Delayed_Ctr_Off, "ERROR: BMS_a134_SW_Delayed_Ctr_Off");
  printDebugIfActive(battery_BMS_a136_SW_Module_OT_Warning, "ERROR: BMS_a136_SW_Module_OT_Warning");
  printDebugIfActive(battery_BMS_a137_SW_Brick_UV_Warning, "ERROR: BMS_a137_SW_Brick_UV_Warning");
  printDebugIfActive(battery_BMS_a139_SW_DC_Link_V_Irrational, "ERROR: BMS_a139_SW_DC_Link_V_Irrational");
  printDebugIfActive(battery_BMS_a141_SW_BMB_Status_Warning, "ERROR: BMS_a141_SW_BMB_Status_Warning");
  printDebugIfActive(battery_BMS_a144_Hvp_Config_Mismatch, "ERROR: BMS_a144_Hvp_Config_Mismatch");
  printDebugIfActive(battery_BMS_a145_SW_SOC_Change, "ERROR: BMS_a145_SW_SOC_Change");
  printDebugIfActive(battery_BMS_a146_SW_Brick_Overdischarged, "ERROR: BMS_a146_SW_Brick_Overdischarged");
  printDebugIfActive(battery_BMS_a149_SW_Missing_Config_Block, "ERROR: BMS_a149_SW_Missing_Config_Block");
  printDebugIfActive(battery_BMS_a151_SW_external_isolation, "ERROR: BMS_a151_SW_external_isolation");
  printDebugIfActive(battery_BMS_a156_SW_BMB_Vref_bad, "ERROR: BMS_a156_SW_BMB_Vref_bad");
  printDebugIfActive(battery_BMS_a157_SW_HVP_HVS_Comms, "ERROR: BMS_a157_SW_HVP_HVS_Comms");
  printDebugIfActive(battery_BMS_a158_SW_HVP_HVI_Comms, "ERROR: BMS_a158_SW_HVP_HVI_Comms");
  printDebugIfActive(battery_BMS_a159_SW_HVP_ECU_Error, "ERROR: BMS_a159_SW_HVP_ECU_Error");
  printDebugIfActive(battery_BMS_a161_SW_DI_Open_Request, "ERROR: BMS_a161_SW_DI_Open_Request");
  printDebugIfActive(battery_BMS_a162_SW_No_Power_For_Support, "ERROR: BMS_a162_SW_No_Power_For_Support");
  printDebugIfActive(battery_BMS_a163_SW_Contactor_Mismatch, "ERROR: BMS_a163_SW_Contactor_Mismatch");
  printDebugIfActive(battery_BMS_a164_SW_Uncontrolled_Regen, "ERROR: BMS_a164_SW_Uncontrolled_Regen");
  printDebugIfActive(battery_BMS_a165_SW_Pack_Partial_Weld, "ERROR: BMS_a165_SW_Pack_Partial_Weld");
  printDebugIfActive(battery_BMS_a166_SW_Pack_Full_Weld, "ERROR: BMS_a166_SW_Pack_Full_Weld");
  printDebugIfActive(battery_BMS_a167_SW_FC_Partial_Weld, "ERROR: BMS_a167_SW_FC_Partial_Weld");
  printDebugIfActive(battery_BMS_a168_SW_FC_Full_Weld, "ERROR: BMS_a168_SW_FC_Full_Weld");
  printDebugIfActive(battery_BMS_a169_SW_FC_Pack_Weld, "ERROR: BMS_a169_SW_FC_Pack_Weld");
  printDebugIfActive(battery_BMS_a170_SW_Limp_Mode, "ERROR: BMS_a170_SW_Limp_Mode");
  printDebugIfActive(battery_BMS_a171_SW_Stack_Voltage_Sense, "ERROR: BMS_a171_SW_Stack_Voltage_Sense");
  printDebugIfActive(battery_BMS_a174_SW_Charge_Failure, "ERROR: BMS_a174_SW_Charge_Failure");
  printDebugIfActive(battery_BMS_a176_SW_GracefulPowerOff, "ERROR: BMS_a176_SW_GracefulPowerOff");
  printDebugIfActive(battery_BMS_a179_SW_Hvp_12V_Fault, "ERROR: BMS_a179_SW_Hvp_12V_Fault");
  printDebugIfActive(battery_BMS_a180_SW_ECU_reset_blocked, "ERROR: BMS_a180_SW_ECU_reset_blocked");
}

#ifdef DOUBLE_BATTERY  //need to update battery2
void printFaultCodesIfActive_battery2() {
  if (battery2_packCtrsClosingAllowed == 0) {
    logging.println(
        "ERROR: Check high voltage connectors and interlock circuit! Closing contactor not allowed! Values: ");
  }
  if (battery2_pyroTestInProgress == 1) {
    logging.println("ERROR: Please wait for Pyro Connection check to finish, HV cables successfully seated!");
  }
  if (datalayer.system.status.inverter_allows_contactor_closing == false) {
    logging.println(
        "ERROR: Solar inverter does not allow for contactor closing. Check communication connection to the inverter "
        "OR "
        "disable the inverter protocol to proceed with contactor closing");
  }
  // Check each symbol and print debug information if its value is 1
  printDebugIfActive(battery2_WatchdogReset, "ERROR: The processor has experienced a reset due to watchdog reset");
  printDebugIfActive(battery2_PowerLossReset, "ERROR: The processor has experienced a reset due to power loss");
  printDebugIfActive(battery2_SwAssertion, "ERROR: An internal software assertion has failed");
  printDebugIfActive(battery2_CrashEvent, "ERROR: crash signal is detected by HVP");
  printDebugIfActive(battery2_OverDchgCurrentFault,
                     "ERROR: Pack discharge current is above the safe max discharge current limit!");
  printDebugIfActive(battery2_OverChargeCurrentFault,
                     "ERROR: Pack charge current is above the safe max charge current limit!");
  printDebugIfActive(battery2_OverCurrentFault,
                     "ERROR: Pack current (discharge or charge) is above max current limit!");
  printDebugIfActive(battery2_OverTemperatureFault,
                     "ERROR: A pack module temperature is above the max temperature limit!");
  printDebugIfActive(battery2_OverVoltageFault, "ERROR: A brick voltage is above maximum voltage limit");
  printDebugIfActive(battery2_UnderVoltageFault, "ERROR: A brick voltage is below minimum voltage limit");
  printDebugIfActive(battery2_PrimaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from primary BMB chain are mia");
  printDebugIfActive(battery2_SecondaryBmbMiaFault,
                     "ERROR: voltage and temperature readings from secondary BMB chain are mia");
  printDebugIfActive(battery2_BmbMismatchFault,
                     "ERROR: primary and secondary BMB chain readings don't match with each other");
  printDebugIfActive(battery2_BmsHviMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  //printDebugIfActive(battery2_CpMiaFault, "ERROR: CP node is mia on HVS CAN"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PcsMiaFault, "ERROR: PCS node is mia on HVS CAN");
  //printDebugIfActive(battery2_BmsFault, "ERROR: BmsFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PcsFault, "ERROR: PcsFault is active");
  //printDebugIfActive(battery2_CpFault, "ERROR: CpFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_ShuntHwMiaFault, "ERROR: shunt current reading is not available");
  printDebugIfActive(battery2_PyroMiaFault, "ERROR: pyro squib is not connected");
  printDebugIfActive(battery2_hvsMiaFault, "ERROR: pack contactor hw fault");
  printDebugIfActive(battery2_hviMiaFault, "ERROR: FC contactor hw fault");
  printDebugIfActive(battery2_Supply12vFault, "ERROR: Low voltage (12V) battery is below minimum voltage threshold");
  printDebugIfActive(battery2_VerSupplyFault,
                     "ERROR: Energy reserve voltage supply is below minimum voltage threshold");
  printDebugIfActive(battery2_HvilFault, "ERROR: High Voltage Inter Lock fault is detected");
  printDebugIfActive(battery2_BmsHvsMiaFault, "ERROR: BMS node is mia on HVS or HVI CAN");
  printDebugIfActive(battery2_PackVoltMismatchFault,
                     "ERROR: Pack voltage doesn't match approximately with sum of brick voltages");
  //printDebugIfActive(battery2_EnsMiaFault, "ERROR: ENS line is not connected to HVC"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PackPosCtrArcFault, "ERROR: HVP detectes series arc at pack contactor");
  printDebugIfActive(battery2_packNegCtrArcFault, "ERROR: HVP detectes series arc at FC contactor");
  printDebugIfActive(battery2_ShuntHwAndBmsMiaFault, "ERROR: ShuntHwAndBmsMiaFault is active");
  printDebugIfActive(battery2_fcContHwFault, "ERROR: fcContHwFault is active");
  printDebugIfActive(battery2_robinOverVoltageFault, "ERROR: robinOverVoltageFault is active");
  printDebugIfActive(battery2_packContHwFault, "ERROR: packContHwFault is active");
  printDebugIfActive(battery2_pyroFuseBlown, "ERROR: pyroFuseBlown is active");
  printDebugIfActive(battery2_pyroFuseFailedToBlow, "ERROR: pyroFuseFailedToBlow is active");
  //printDebugIfActive(battery2_CpilFault, "ERROR: CpilFault is active"); //Uncommented due to not affecting usage
  printDebugIfActive(battery2_PackContactorFellOpen, "ERROR: PackContactorFellOpen is active");
  printDebugIfActive(battery2_FcContactorFellOpen, "ERROR: FcContactorFellOpen is active");
  printDebugIfActive(battery2_packCtrCloseBlocked, "ERROR: packCtrCloseBlocked is active");
  printDebugIfActive(battery2_fcCtrCloseBlocked, "ERROR: fcCtrCloseBlocked is active");
  printDebugIfActive(battery2_packContactorForceOpen, "ERROR: packContactorForceOpen is active");
  printDebugIfActive(battery2_fcContactorForceOpen, "ERROR: fcContactorForceOpen is active");
  printDebugIfActive(battery2_dcLinkOverVoltage, "ERROR: dcLinkOverVoltage is active");
  printDebugIfActive(battery2_shuntOverTemperature, "ERROR: shuntOverTemperature is active");
  printDebugIfActive(battery2_passivePyroDeploy, "ERROR: passivePyroDeploy is active");
  printDebugIfActive(battery2_logUploadRequest, "ERROR: logUploadRequest is active");
  printDebugIfActive(battery2_packCtrCloseFailed, "ERROR: packCtrCloseFailed is active");
  printDebugIfActive(battery2_fcCtrCloseFailed, "ERROR: fcCtrCloseFailed is active");
  printDebugIfActive(battery2_shuntThermistorMia, "ERROR: shuntThermistorMia is active");
  // 0x320 800 BMS_alertMatrix
  printDebugIfActive(battery2_BMS_a017_SW_Brick_OV, "ERROR: BMS_a017_SW_Brick_OV");
  printDebugIfActive(battery2_BMS_a018_SW_Brick_UV, "ERROR: BMS_a018_SW_Brick_UV");
  printDebugIfActive(battery2_BMS_a019_SW_Module_OT, "ERROR: BMS_a019_SW_Module_OT");
  printDebugIfActive(battery2_BMS_a021_SW_Dr_Limits_Regulation, "ERROR: BMS_a021_SW_Dr_Limits_Regulation");
  printDebugIfActive(battery2_BMS_a022_SW_Over_Current, "ERROR: BMS_a022_SW_Over_Current");
  printDebugIfActive(battery2_BMS_a023_SW_Stack_OV, "ERROR: BMS_a023_SW_Stack_OV");
  printDebugIfActive(battery2_BMS_a024_SW_Islanded_Brick, "ERROR: BMS_a024_SW_Islanded_Brick");
  printDebugIfActive(battery2_BMS_a025_SW_PwrBalance_Anomaly, "ERROR: BMS_a025_SW_PwrBalance_Anomaly");
  printDebugIfActive(battery2_BMS_a026_SW_HFCurrent_Anomaly, "ERROR: BMS_a026_SW_HFCurrent_Anomaly");
  printDebugIfActive(battery2_BMS_a034_SW_Passive_Isolation, "ERROR: BMS_a034_SW_Passive_Isolation");
  printDebugIfActive(battery2_BMS_a035_SW_Isolation, "ERROR: BMS_a035_SW_Isolation");
  printDebugIfActive(battery2_BMS_a036_SW_HvpHvilFault, "ERROR: BMS_a036_SW_HvpHvilFault");
  printDebugIfActive(battery2_BMS_a037_SW_Flood_Port_Open, "ERROR: BMS_a037_SW_Flood_Port_Open");
  printDebugIfActive(battery2_BMS_a039_SW_DC_Link_Over_Voltage, "ERROR: BMS_a039_SW_DC_Link_Over_Voltage");
  printDebugIfActive(battery2_BMS_a041_SW_Power_On_Reset, "ERROR: BMS_a041_SW_Power_On_Reset");
  printDebugIfActive(battery2_BMS_a042_SW_MPU_Error, "ERROR: BMS_a042_SW_MPU_Error");
  printDebugIfActive(battery2_BMS_a043_SW_Watch_Dog_Reset, "ERROR: BMS_a043_SW_Watch_Dog_Reset");
  printDebugIfActive(battery2_BMS_a044_SW_Assertion, "ERROR: BMS_a044_SW_Assertion");
  printDebugIfActive(battery2_BMS_a045_SW_Exception, "ERROR: BMS_a045_SW_Exception");
  printDebugIfActive(battery2_BMS_a046_SW_Task_Stack_Usage, "ERROR: BMS_a046_SW_Task_Stack_Usage");
  printDebugIfActive(battery2_BMS_a047_SW_Task_Stack_Overflow, "ERROR: BMS_a047_SW_Task_Stack_Overflow");
  printDebugIfActive(battery2_BMS_a048_SW_Log_Upload_Request, "ERROR: BMS_a048_SW_Log_Upload_Request");
  printDebugIfActive(battery2_BMS_a050_SW_Brick_Voltage_MIA, "ERROR: BMS_a050_SW_Brick_Voltage_MIA");
  printDebugIfActive(battery2_BMS_a051_SW_HVC_Vref_Bad, "ERROR: BMS_a051_SW_HVC_Vref_Bad");
  printDebugIfActive(battery2_BMS_a052_SW_PCS_MIA, "ERROR: BMS_a052_SW_PCS_MIA");
  printDebugIfActive(battery2_BMS_a053_SW_ThermalModel_Sanity, "ERROR: BMS_a053_SW_ThermalModel_Sanity");
  printDebugIfActive(battery2_BMS_a054_SW_Ver_Supply_Fault, "ERROR: BMS_a054_SW_Ver_Supply_Fault");
  printDebugIfActive(battery2_BMS_a059_SW_Pack_Voltage_Sensing, "ERROR: BMS_a059_SW_Pack_Voltage_Sensing");
  printDebugIfActive(battery2_BMS_a060_SW_Leakage_Test_Failure, "ERROR: BMS_a060_SW_Leakage_Test_Failure");
  printDebugIfActive(battery2_BMS_a061_robinBrickOverVoltage, "ERROR: BMS_a061_robinBrickOverVoltage");
  printDebugIfActive(battery2_BMS_a062_SW_BrickV_Imbalance, "ERROR: BMS_a062_SW_BrickV_Imbalance");
  printDebugIfActive(battery2_BMS_a063_SW_ChargePort_Fault, "ERROR: BMS_a063_SW_ChargePort_Fault");
  printDebugIfActive(battery2_BMS_a064_SW_SOC_Imbalance, "ERROR: BMS_a064_SW_SOC_Imbalance");
  printDebugIfActive(battery2_BMS_a069_SW_Low_Power, "ERROR: BMS_a069_SW_Low_Power");
  printDebugIfActive(battery2_BMS_a071_SW_SM_TransCon_Not_Met, "ERROR: BMS_a071_SW_SM_TransCon_Not_Met");
  printDebugIfActive(battery2_BMS_a075_SW_Chg_Disable_Failure, "ERROR: BMS_a075_SW_Chg_Disable_Failure");
  printDebugIfActive(battery2_BMS_a076_SW_Dch_While_Charging, "ERROR: BMS_a076_SW_Dch_While_Charging");
  printDebugIfActive(battery2_BMS_a077_SW_Charger_Regulation, "ERROR: BMS_a077_SW_Charger_Regulation");
  printDebugIfActive(battery2_BMS_a081_SW_Ctr_Close_Blocked, "ERROR: BMS_a081_SW_Ctr_Close_Blocked");
  printDebugIfActive(battery2_BMS_a082_SW_Ctr_Force_Open, "ERROR: BMS_a082_SW_Ctr_Force_Open");
  printDebugIfActive(battery2_BMS_a083_SW_Ctr_Close_Failure, "ERROR: BMS_a083_SW_Ctr_Close_Failure");
  printDebugIfActive(battery2_BMS_a084_SW_Sleep_Wake_Aborted, "ERROR: BMS_a084_SW_Sleep_Wake_Aborted");
  printDebugIfActive(battery2_BMS_a087_SW_Feim_Test_Blocked, "ERROR: BMS_a087_SW_Feim_Test_Blocked");
  printDebugIfActive(battery2_BMS_a088_SW_VcFront_MIA_InDrive, "ERROR: BMS_a088_SW_VcFront_MIA_InDrive");
  printDebugIfActive(battery2_BMS_a089_SW_VcFront_MIA, "ERROR: BMS_a089_SW_VcFront_MIA");
  printDebugIfActive(battery2_BMS_a090_SW_Gateway_MIA, "ERROR: BMS_a090_SW_Gateway_MIA");
  printDebugIfActive(battery2_BMS_a091_SW_ChargePort_MIA, "ERROR: BMS_a091_SW_ChargePort_MIA");
  printDebugIfActive(battery2_BMS_a092_SW_ChargePort_Mia_On_Hv, "ERROR: BMS_a092_SW_ChargePort_Mia_On_Hv");
  printDebugIfActive(battery2_BMS_a094_SW_Drive_Inverter_MIA, "ERROR: BMS_a094_SW_Drive_Inverter_MIA");
  printDebugIfActive(battery2_BMS_a099_SW_BMB_Communication, "ERROR: BMS_a099_SW_BMB_Communication");
  printDebugIfActive(battery2_BMS_a105_SW_One_Module_Tsense, "ERROR: BMS_a105_SW_One_Module_Tsense");
  printDebugIfActive(battery2_BMS_a106_SW_All_Module_Tsense, "ERROR: BMS_a106_SW_All_Module_Tsense");
  printDebugIfActive(battery2_BMS_a107_SW_Stack_Voltage_MIA, "ERROR: BMS_a107_SW_Stack_Voltage_MIA");
  printDebugIfActive(battery2_BMS_a121_SW_NVRAM_Config_Error, "ERROR: BMS_a121_SW_NVRAM_Config_Error");
  printDebugIfActive(battery2_BMS_a122_SW_BMS_Therm_Irrational, "ERROR: BMS_a122_SW_BMS_Therm_Irrational");
  printDebugIfActive(battery2_BMS_a123_SW_Internal_Isolation, "ERROR: BMS_a123_SW_Internal_Isolation");
  printDebugIfActive(battery2_BMS_a127_SW_shunt_SNA, "ERROR: BMS_a127_SW_shunt_SNA");
  printDebugIfActive(battery2_BMS_a128_SW_shunt_MIA, "ERROR: BMS_a128_SW_shunt_MIA");
  printDebugIfActive(battery2_BMS_a129_SW_VSH_Failure, "ERROR: BMS_a129_SW_VSH_Failure");
  printDebugIfActive(battery2_BMS_a130_IO_CAN_Error, "ERROR: BMS_a130_IO_CAN_Error");
  printDebugIfActive(battery2_BMS_a131_Bleed_FET_Failure, "ERROR: BMS_a131_Bleed_FET_Failure");
  printDebugIfActive(battery2_BMS_a132_HW_BMB_OTP_Uncorrctbl, "ERROR: BMS_a132_HW_BMB_OTP_Uncorrctbl");
  printDebugIfActive(battery2_BMS_a134_SW_Delayed_Ctr_Off, "ERROR: BMS_a134_SW_Delayed_Ctr_Off");
  printDebugIfActive(battery2_BMS_a136_SW_Module_OT_Warning, "ERROR: BMS_a136_SW_Module_OT_Warning");
  printDebugIfActive(battery2_BMS_a137_SW_Brick_UV_Warning, "ERROR: BMS_a137_SW_Brick_UV_Warning");
  printDebugIfActive(battery2_BMS_a139_SW_DC_Link_V_Irrational, "ERROR: BMS_a139_SW_DC_Link_V_Irrational");
  printDebugIfActive(battery2_BMS_a141_SW_BMB_Status_Warning, "ERROR: BMS_a141_SW_BMB_Status_Warning");
  printDebugIfActive(battery2_BMS_a144_Hvp_Config_Mismatch, "ERROR: BMS_a144_Hvp_Config_Mismatch");
  printDebugIfActive(battery2_BMS_a145_SW_SOC_Change, "ERROR: BMS_a145_SW_SOC_Change");
  printDebugIfActive(battery2_BMS_a146_SW_Brick_Overdischarged, "ERROR: BMS_a146_SW_Brick_Overdischarged");
  printDebugIfActive(battery2_BMS_a149_SW_Missing_Config_Block, "ERROR: BMS_a149_SW_Missing_Config_Block");
  printDebugIfActive(battery2_BMS_a151_SW_external_isolation, "ERROR: BMS_a151_SW_external_isolation");
  printDebugIfActive(battery2_BMS_a156_SW_BMB_Vref_bad, "ERROR: BMS_a156_SW_BMB_Vref_bad");
  printDebugIfActive(battery2_BMS_a157_SW_HVP_HVS_Comms, "ERROR: BMS_a157_SW_HVP_HVS_Comms");
  printDebugIfActive(battery2_BMS_a158_SW_HVP_HVI_Comms, "ERROR: BMS_a158_SW_HVP_HVI_Comms");
  printDebugIfActive(battery2_BMS_a159_SW_HVP_ECU_Error, "ERROR: BMS_a159_SW_HVP_ECU_Error");
  printDebugIfActive(battery2_BMS_a161_SW_DI_Open_Request, "ERROR: BMS_a161_SW_DI_Open_Request");
  printDebugIfActive(battery2_BMS_a162_SW_No_Power_For_Support, "ERROR: BMS_a162_SW_No_Power_For_Support");
  printDebugIfActive(battery2_BMS_a163_SW_Contactor_Mismatch, "ERROR: BMS_a163_SW_Contactor_Mismatch");
  printDebugIfActive(battery2_BMS_a164_SW_Uncontrolled_Regen, "ERROR: BMS_a164_SW_Uncontrolled_Regen");
  printDebugIfActive(battery2_BMS_a165_SW_Pack_Partial_Weld, "ERROR: BMS_a165_SW_Pack_Partial_Weld");
  printDebugIfActive(battery2_BMS_a166_SW_Pack_Full_Weld, "ERROR: BMS_a166_SW_Pack_Full_Weld");
  printDebugIfActive(battery2_BMS_a167_SW_FC_Partial_Weld, "ERROR: BMS_a167_SW_FC_Partial_Weld");
  printDebugIfActive(battery2_BMS_a168_SW_FC_Full_Weld, "ERROR: BMS_a168_SW_FC_Full_Weld");
  printDebugIfActive(battery2_BMS_a169_SW_FC_Pack_Weld, "ERROR: BMS_a169_SW_FC_Pack_Weld");
  printDebugIfActive(battery2_BMS_a170_SW_Limp_Mode, "ERROR: BMS_a170_SW_Limp_Mode");
  printDebugIfActive(battery2_BMS_a171_SW_Stack_Voltage_Sense, "ERROR: BMS_a171_SW_Stack_Voltage_Sense");
  printDebugIfActive(battery2_BMS_a174_SW_Charge_Failure, "ERROR: BMS_a174_SW_Charge_Failure");
  printDebugIfActive(battery2_BMS_a176_SW_GracefulPowerOff, "ERROR: BMS_a176_SW_GracefulPowerOff");
  printDebugIfActive(battery2_BMS_a179_SW_Hvp_12V_Fault, "ERROR: BMS_a179_SW_Hvp_12V_Fault");
  printDebugIfActive(battery2_BMS_a180_SW_ECU_reset_blocked, "ERROR: BMS_a180_SW_ECU_reset_blocked");
}
#endif  //DOUBLE_BATTERY

void printDebugIfActive(uint8_t symbol, const char* message) {
  if (symbol == 1) {
    logging.println(message);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  datalayer.system.status.battery_allows_contactor_closing = true;

#ifdef TESLA_MODEL_SX_BATTERY  // Always use NCM/A mode on S/X packs
  strncpy(datalayer.system.info.battery_protocol, "Tesla Model S/X", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_SX_NCMA;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#endif  // DOUBLE_BATTERY
#endif  // TESLA_MODEL_SX_BATTERY

#ifdef TESLA_MODEL_3Y_BATTERY  // Model 3/Y can be either LFP or NCM/A
  strncpy(datalayer.system.info.battery_protocol, "Tesla Model 3/Y", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
#ifdef LFP_CHEMISTRY
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_LFP;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_LFP;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_LFP;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_LFP;
#endif  // DOUBLE_BATTERY
#else   // Startup in NCM/A mode
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery2.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_3Y_NCMA;
  datalayer.battery2.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_NCA_NCM;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_NCA_NCM;
#endif  // DOUBLE_BATTERY
#endif  // !LFP_CHEMISTRY
#endif  // TESLA_MODEL_3Y_BATTERY
}

#endif  // TESLA_BATTERY
