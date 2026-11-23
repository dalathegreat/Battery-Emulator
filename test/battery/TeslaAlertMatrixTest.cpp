#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include "../../Software/src/battery/TESLA-BATTERY.h"
#include "../../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"
#include "../utils/utils.h"

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

// The bit positions below are a copy of the assignments the driver used before
// the alert table. The driver under test writes the datalayer. This file
// applies the copy and the two results have to match, including the SOC-change
// and ECU-reset-blocked decisions.

namespace {

struct OldBit {
  uint8_t mux;
  uint8_t byte_index;
  uint8_t bit;
};

const OldBit kOldBms[100] = {
    {0, 0, 4},  // BMS_a001_Pack_Config_Mismatch
    {0, 2, 4},  // BMS_a017_SW_Brick_OV
    {0, 2, 5},  // BMS_a018_SW_Brick_UV
    {0, 2, 6},  // BMS_a019_SW_Module_OT
    {0, 3, 0},  // BMS_a021_SW_Dr_Limits_Regulation
    {0, 3, 1},  // BMS_a022_SW_Over_Current
    {0, 3, 2},  // BMS_a023_SW_Stack_OV
    {0, 3, 3},  // BMS_a024_SW_Islanded_Brick
    {0, 3, 4},  // BMS_a025_SW_PwrBalance_Anomaly
    {0, 3, 5},  // BMS_a026_SW_HFCurrent_Anomaly
    {0, 4, 5},  // BMS_a034_SW_Passive_Isolation
    {0, 4, 6},  // BMS_a035_SW_Isolation
    {0, 4, 7},  // BMS_a036_SW_HvpHvilFault
    {0, 5, 0},  // BMS_a037_SW_Flood_Port_Open
    {0, 5, 2},  // BMS_a039_SW_DC_Link_Over_Voltage
    {0, 5, 4},  // BMS_a041_SW_Power_On_Reset
    {0, 5, 5},  // BMS_a042_SW_MPU_Error
    {0, 5, 6},  // BMS_a043_SW_Watch_Dog_Reset
    {0, 5, 7},  // BMS_a044_SW_Assertion
    {0, 6, 0},  // BMS_a045_SW_Exception
    {0, 6, 1},  // BMS_a046_SW_Task_Stack_Usage
    {0, 6, 2},  // BMS_a047_SW_Task_Stack_Overflow
    {0, 6, 3},  // BMS_a048_SW_Log_Upload_Request
    {0, 6, 5},  // BMS_a050_SW_Brick_Voltage_MIA
    {0, 6, 6},  // BMS_a051_SW_HVC_Vref_Bad
    {0, 6, 7},  // BMS_a052_SW_PCS_MIA
    {0, 7, 0},  // BMS_a053_SW_ThermalModel_Sanity
    {0, 7, 1},  // BMS_a054_SW_Ver_Supply_Fault
    {0, 7, 2},  // BMS_a055_SW_HvChain_Model_Fault
    {0, 7, 6},  // BMS_a059_SW_Pack_Voltage_Sensing
    {0, 7, 7},  // BMS_a060_SW_Leakage_Test_Failure
    {1, 0, 4},  // BMS_a061_robinBrickOverVoltage
    {1, 0, 5},  // BMS_a062_SW_BrickV_Imbalance
    {1, 0, 6},  // BMS_a063_SW_ChargePort_Fault
    {1, 0, 7},  // BMS_a064_SW_SOC_Imbalance
    {1, 1, 4},  // BMS_a069_SW_Low_Power
    {1, 1, 6},  // BMS_a071_SW_SM_TransCon_Not_Met
    {1, 2, 2},  // BMS_a075_SW_Chg_Disable_Failure
    {1, 2, 3},  // BMS_a076_SW_Dch_While_Charging
    {1, 2, 4},  // BMS_a077_SW_Charger_Regulation
    {1, 3, 0},  // BMS_a081_SW_Ctr_Close_Blocked
    {1, 3, 1},  // BMS_a082_SW_Ctr_Force_Open
    {1, 3, 2},  // BMS_a083_SW_Ctr_Close_Failure
    {1, 3, 3},  // BMS_a084_SW_Sleep_Wake_Aborted
    {1, 3, 6},  // BMS_a087_SW_Feim_Test_Blocked
    {1, 3, 7},  // BMS_a088_SW_VcFront_MIA_InDrive
    {1, 4, 0},  // BMS_a089_SW_VcFront_MIA
    {1, 4, 1},  // BMS_a090_SW_Gateway_MIA
    {1, 4, 2},  // BMS_a091_SW_ChargePort_MIA
    {1, 4, 3},  // BMS_a092_SW_ChargePort_Mia_On_Hv
    {1, 4, 5},  // BMS_a094_SW_Drive_Inverter_MIA
    {1, 5, 2},  // BMS_a099_SW_BMB_Communication
    {1, 6, 0},  // BMS_a105_SW_One_Module_Tsense
    {1, 6, 1},  // BMS_a106_SW_All_Module_Tsense
    {1, 6, 2},  // BMS_a107_SW_Stack_Voltage_MIA
    {2, 0, 4},  // BMS_a121_SW_NVRAM_Config_Error
    {2, 0, 5},  // BMS_a122_SW_BMS_Therm_Irrational
    {2, 0, 6},  // BMS_a123_SW_Internal_Isolation
    {2, 1, 1},  // BMS_a126_SW_Thermistor_Failure
    {2, 1, 2},  // BMS_a127_SW_shunt_SNA
    {2, 1, 3},  // BMS_a128_SW_shunt_MIA
    {2, 1, 4},  // BMS_a129_SW_VSH_Failure
    {2, 1, 5},  // BMS_a130_IO_CAN_Error
    {2, 1, 6},  // BMS_a131_Bleed_FET_Failure
    {2, 1, 7},  // BMS_a132_HW_BMB_OTP_Uncorrctbl
    {2, 2, 1},  // BMS_a134_SW_Delayed_Ctr_Off
    {2, 2, 2},  // BMS_a135_HW_BMB_Diagnostics_Failure
    {2, 2, 3},  // BMS_a136_SW_Module_OT_Warning
    {2, 2, 4},  // BMS_a137_SW_Brick_UV_Warning
    {2, 2, 5},  // BMS_a138_SW_Brick_OV_Warning
    {2, 2, 6},  // BMS_a139_SW_DC_Link_V_Irrational
    {2, 3, 0},  // BMS_a141_SW_BMB_Status_Warning
    {2, 3, 2},  // BMS_a143_SW_CAC_Change
    {2, 3, 3},  // BMS_a144_Hvp_Config_Mismatch
    {2, 3, 4},  // BMS_a145_SW_SOC_Change
    {2, 3, 5},  // BMS_a146_SW_Brick_Overdischarged
    {2, 4, 0},  // BMS_a149_SW_Missing_Config_Block
    {2, 4, 2},  // BMS_a151_SW_external_isolation
    {2, 4, 6},  // BMS_a155_SW_Weak_short_impedence
    {2, 4, 7},  // BMS_a156_SW_BMB_Vref_bad
    {2, 5, 0},  // BMS_a157_SW_HVP_HVS_Comms
    {2, 5, 1},  // BMS_a158_SW_HVP_HVI_Comms
    {2, 5, 2},  // BMS_a159_SW_HVP_ECU_Error
    {2, 5, 4},  // BMS_a161_SW_DI_Open_Request
    {2, 5, 5},  // BMS_a162_SW_No_Power_For_Support
    {2, 5, 6},  // BMS_a163_SW_Contactor_Mismatch
    {2, 5, 7},  // BMS_a164_SW_Uncontrolled_Regen
    {2, 6, 0},  // BMS_a165_SW_Pack_Partial_Weld
    {2, 6, 1},  // BMS_a166_SW_Pack_Full_Weld
    {2, 6, 2},  // BMS_a167_SW_FC_Partial_Weld
    {2, 6, 3},  // BMS_a168_SW_FC_Full_Weld
    {2, 6, 4},  // BMS_a169_SW_FC_Pack_Weld
    {2, 6, 5},  // BMS_a170_SW_Limp_Mode
    {2, 6, 6},  // BMS_a171_SW_Stack_Voltage_Sense
    {2, 7, 0},  // BMS_a173_SW_Charge_Component_Fault
    {2, 7, 1},  // BMS_a174_SW_Charge_Failure
    {2, 7, 3},  // BMS_a176_SW_GracefulPowerOff
    {2, 7, 5},  // BMS_a178_SW_Uncontrolled_Regen_PwrB
    {2, 7, 6},  // BMS_a179_SW_Hvp_12V_Fault
    {2, 7, 7},  // BMS_a180_SW_ECU_reset_blocked
};

const OldBit kOldPcs[94] = {
    {0, 0, 4},  // PCS_a001_chgHwInputOc
    {0, 0, 5},  // PCS_a002_chgHwOutputOc
    {0, 0, 6},  // PCS_a003_chgHwInputOv
    {0, 0, 7},  // PCS_a004_chgHwIntBusOv
    {0, 1, 0},  // PCS_a005_chgOutputOv
    {0, 1, 1},  // PCS_a006_chgPrechargeFailedScr
    {0, 1, 2},  // PCS_a007_chgPhaseTempHot
    {0, 1, 3},  // PCS_a008_chgPhaseOverTemp
    {0, 1, 4},  // PCS_a009_chgPfcCurrentRegulation
    {0, 1, 5},  // PCS_a010_chgIntBusVRegulation
    {0, 1, 6},  // PCS_a011_chgLlcCurrentRegulation
    {0, 1, 7},  // PCS_a012_chgPfcIBandTracerFault
    {0, 2, 0},  // PCS_a013_chgPrechargeFailedBoost
    {0, 2, 1},  // PCS_a014_chgTempRationality
    {0, 2, 2},  // PCS_a015_chg12vUv
    {0, 2, 3},  // PCS_a016_chgAllPhasesFaulted
    {0, 2, 4},  // PCS_a017_chgWallPowerRemoval
    {0, 2, 5},  // PCS_a018_chgUnknownGridConfig
    {0, 2, 6},  // PCS_a019_acChargePowerLimited
    {0, 2, 7},  // PCS_a020_chgEnableLineMismatch
    {0, 3, 0},  // PCS_a021_hvpMia
    {0, 3, 1},  // PCS_a022_bmsMia
    {0, 3, 2},  // PCS_a023_cpMia
    {0, 3, 3},  // PCS_a024_vcfrontMia
    {0, 3, 4},  // PCS_a025_cpu2Malfunction
    {0, 3, 5},  // PCS_a026_watchdogAlarmed
    {0, 3, 6},  // PCS_a027_chgInsufficientCooling
    {0, 3, 7},  // PCS_a028_chgOutputUv
    {0, 4, 0},  // PCS_a029_chgPowerRationality
    {0, 4, 1},  // PCS_a030_canRationality
    {0, 4, 2},  // PCS_a031_uiMia
    {0, 4, 3},  // PCS_a032_gtwMia
    {0, 4, 4},  // PCS_a033_hvBusUv
    {0, 4, 5},  // PCS_a034_hvBusOv
    {0, 4, 6},  // PCS_a035_lvBusUv
    {0, 4, 7},  // PCS_a036_lvBusOv
    {0, 5, 0},  // PCS_a037_resonantTankOc
    {0, 5, 1},  // PCS_a038_claFaulted
    {0, 5, 2},  // PCS_a039_sdModuleClkFault
    {0, 5, 3},  // PCS_a040_dcdcMaxPowerReached
    {0, 5, 4},  // PCS_a041_dcdcOverTemp
    {0, 5, 5},  // PCS_a042_dcdcEnableLineMismatch
    {0, 5, 6},  // PCS_a043_hvBusPrechargeFailure
    {0, 5, 7},  // PCS_a044_12vSupportRegulation
    {0, 6, 0},  // PCS_a045_hvBusLowImpedance
    {0, 6, 1},  // PCS_a046_hvBusHighImpedence
    {0, 6, 2},  // PCS_a047_lvBusLowImpedance
    {0, 6, 3},  // PCS_a048_lvBusHighImpedance
    {0, 6, 4},  // PCS_a049_dcdcTempRationality
    {0, 6, 5},  // PCS_a050_dcdc12VsupportFaulted
    {0, 6, 6},  // PCS_a051_chgIntBusUv
    {0, 6, 7},  // PCS_a052_acVoltageNotPresent
    {0, 7, 0},  // PCS_a053_chgInputVDropHigh
    {0, 7, 1},  // PCS_a054_chgInputVDropTooHigh
    {0, 7, 2},  // PCS_a055_chgLineImedanceHigh
    {0, 7, 3},  // PCS_a056_chgLineImedanceTooHigh
    {0, 7, 4},  // PCS_a057_chgInputOverFreq
    {0, 7, 5},  // PCS_a058_chgInputUnderFreq
    {0, 7, 6},  // PCS_a059_chgInputOvRms
    {0, 7, 7},  // PCS_a060_chgInputOvPeak
    {1, 0, 4},  // PCS_a061_chgVLineRationality
    {1, 0, 5},  // PCS_a062_chgILineRationality
    {1, 0, 6},  // PCS_a063_chgVOutRationality
    {1, 0, 7},  // PCS_a064_chgIOutRationality
    {1, 1, 0},  // PCS_a065_chgPllNotLocked
    {1, 1, 1},  // PCS_a066_dcdcHvRationality
    {1, 1, 2},  // PCS_a067_dcdcLvRationality
    {1, 1, 3},  // PCS_a068_dcdcTankvRationality
    {1, 1, 4},  // PCS_a069_chgPfcLineDidt
    {1, 1, 5},  // PCS_a070_chgPfcLineDvdt
    {1, 1, 6},  // PCS_a071_chgPfcILoopRationality
    {1, 1, 7},  // PCS_a072_cpu2ClaStopped
    {1, 2, 0},  // PCS_a073_unexpectedAcInputVoltage
    {1, 2, 1},  // PCS_a074_hvBusDischargeFailure
    {1, 2, 2},  // PCS_a075_hvBusDischargeTimeout
    {1, 2, 3},  // PCS_a076_dcdcEnDeassertedErr
    {1, 2, 4},  // PCS_a077_microGridEnergyLow
    {1, 2, 5},  // PCS_a078_chgStopDcdcTooHot
    {1, 2, 6},  // PCS_a079_eepromOperationError
    {1, 2, 7},  // PCS_a080_damagedPhaseDetected
    {1, 3, 0},  // PCS_a081_dcdcPchgTimeout
    {1, 3, 1},  // PCS_a082_dcdcPchgUnsafeDiVoltage
    {1, 3, 2},  // PCS_a083_triggerOdin
    {1, 3, 3},  // PCS_a084_dcdcPchgStartVoltages
    {1, 3, 4},  // PCS_a085_dcdcFetsNotSwitching
    {1, 3, 5},  // PCS_a086_dcdcInsufficientCooling
    {1, 3, 6},  // PCS_a087_nvramRecordStatusError
    {1, 3, 7},  // PCS_a088_pchgParameters
    {1, 4, 0},  // PCS_a089_hvBusDischargeIrrational
    {1, 4, 1},  // PCS_a090_expectedAcVoltageSourceMissing
    {1, 4, 2},  // PCS_a091_chgIntBusRationality
    {1, 4, 3},  // PCS_a092_chgPowerLimitedByBusRipple
    {1, 4, 4},  // PCS_a093_powerRailRationality
    {1, 4, 5},  // PCS_a094_pcsDcdcNeedService
};

const OldBit kOldCp[96] = {
    {0, 0, 4},  // CP_a001_canRx
    {0, 0, 5},  // CP_a002_canTx
    {0, 0, 6},  // CP_a003_canError
    {0, 0, 7},  // CP_a004_proximityRationality
    {0, 1, 0},  // CP_a005_gbdcLiveDisconnect
    {0, 1, 1},  // CP_a006_lostCommsBMS
    {0, 1, 2},  // CP_a007_watchdog
    {0, 1, 3},  // CP_a008_memoryError
    {0, 1, 4},  // CP_a009_coverOpen
    {0, 1, 5},  // CP_a010_pilotRationality
    {0, 1, 6},  // CP_a011_eeprom
    {0, 1, 7},  // CP_a012_ledDriver
    {0, 2, 0},  // CP_a013_lostCommsGTW
    {0, 2, 1},  // CP_a014_lostCommsCHG
    {0, 2, 2},  // CP_a015_apsVov
    {0, 2, 3},  // CP_a016_apsVuv
    {0, 2, 4},  // CP_a017_fiveVov
    {0, 2, 5},  // CP_a018_fiveVuv
    {0, 2, 6},  // CP_a019_threeVov
    {0, 2, 7},  // CP_a020_threeVuv
    {0, 3, 0},  // CP_a021_zeroVov
    {0, 3, 1},  // CP_a022_zeroVuv
    {0, 3, 2},  // CP_a023_gbdcSessionFailed
    {0, 3, 3},  // CP_a024_ledsUC
    {0, 3, 4},  // CP_a025_ledsOC
    {0, 3, 5},  // CP_a026_networkManagement
    {0, 3, 6},  // CP_a027_doorSensorOutOfSpec
    {0, 3, 7},  // CP_a028_insertEnableMismatch
    {0, 4, 0},  // CP_a029_doorClosedProxPilot
    {0, 4, 1},  // CP_a030_busOff
    {0, 4, 2},  // CP_a031_doorClosedCommandedOpen
    {0, 4, 3},  // CP_a032_doorOpenExpectedClosed
    {0, 4, 4},  // CP_a033_spiOpen
    {0, 4, 5},  // CP_a034_calibrationIncomplete
    {0, 4, 6},  // CP_a035_latchMovement_1
    {0, 4, 7},  // CP_a036_latchNotDisengaged
    {0, 5, 0},  // CP_a037_latchNotEngaged
    {0, 5, 1},  // CP_a038_latchNotBlocking
    {0, 5, 2},  // CP_a039_latchMovement_2
    {0, 5, 3},  // CP_a040_doNotUse
    {0, 5, 4},  // CP_a041_doorSensorUnplugged
    {0, 5, 5},  // CP_a042_doorAssemblyBroken
    {0, 5, 6},  // CP_a043_doorPotIrrational
    {0, 5, 7},  // CP_a044_lostCommsHVP
    {0, 6, 0},  // CP_a045_lostCommsVCSEC
    {0, 6, 1},  // CP_a046_lostCommsEVSE
    {0, 6, 2},  // CP_a047_lostCommsVCFRONT
    {0, 6, 3},  // CP_a048_lostCommsUI
    {0, 6, 4},  // CP_a049_multipleCablesDetected
    {0, 6, 5},  // CP_a050_latchNotConnected
    {0, 6, 6},  // CP_a051_doorInductiveSensorMIA
    {0, 6, 7},  // CP_a052_evseNotSupported
    {0, 7, 0},  // CP_a053_proxLatchedNoPilot
    {0, 7, 1},  // CP_a054_cableNotSecured
    {0, 7, 2},  // CP_a055_chargeStoppedNoPilot
    {0, 7, 3},  // CP_a056_proxDisconnected
    {0, 7, 4},  // CP_a057_evseFaulted
    {0, 7, 5},  // CP_a058_acChargingBlocked
    {0, 7, 6},  // CP_a059_swcanError
    {0, 7, 7},  // CP_a060_lostCommsPCS
    {1, 0, 4},  // CP_a061_uhfReceiverMIA
    {1, 0, 5},  // CP_a062_scOutOfService
    {1, 0, 6},  // CP_a063_scUpdateInProgress
    {1, 0, 7},  // CP_a064_superchargingBlocked
    {1, 1, 0},  // CP_a065_selfTestFailed
    {1, 1, 1},  // CP_a066_proxLatchedIdlePilot
    {1, 1, 2},  // CP_a067_gbdcConnFault
    {1, 1, 3},  // CP_a068_doorSensorMismatch
    {1, 1, 4},  // CP_a069_doorInductiveSensorError
    {1, 1, 5},  // CP_a070_doorInductiveSensorReset
    {1, 1, 6},  // CP_a071_exiDecodeFailure
    {1, 1, 7},  // CP_a072_v2gEvccTimeout
    {1, 2, 0},  // CP_a073_iecComboShutdown
    {1, 2, 1},  // CP_a074_failedToEstablishV2gComm
    {1, 2, 2},  // CP_a075_v2gCommsFailure
    {1, 2, 3},  // CP_a076_LDC1612errorWatchdog
    {1, 2, 4},  // CP_a077_invalidMacAddress
    {1, 2, 5},  // CP_a078_latchNotDisengagedCold
    {1, 2, 6},  // CP_a079_cableNotSecuredCold
    {1, 2, 7},  // CP_a080_taskStackOverflow
    {1, 3, 0},  // CP_a081_swException
    {1, 3, 1},  // CP_a082_powerOnReset
    {1, 3, 2},  // CP_a083_watchdogTraceData
    {1, 3, 3},  // CP_a084_proximityPeDisconnected
    {1, 3, 4},  // CP_a085_dcPinTempFaulted
    {1, 3, 5},  // CP_a086_dcPinTempIrrational
    {1, 3, 6},  // CP_a087_dcTempModelFault
    {1, 3, 7},  // CP_a088_dcTempModelDeviation
    {1, 4, 0},  // CP_a089_plcConfigMismatch
    {1, 4, 1},  // CP_a090_ccsEvseLowIso
    {1, 4, 2},  // CP_a091_wrongSuperchargerHandle
    {1, 4, 3},  // CP_a092_modemAppLoadFailed
    {1, 4, 4},  // CP_a093_modemLoadedWithReset
    {1, 4, 5},  // CP_a094_inductiveResetSuccessful
    {1, 4, 6},  // CP_a095_thermalDcLimitActive
    {1, 4, 7},  // CP_a096_pilotWake
};

void ApplyOld(bool* active, int count, const OldBit* table, const uint8_t* data) {
  const uint8_t mux = data[0] & 0x0F;
  for (int i = 0; i < count; i++) {
    if (table[i].mux == mux) {
      active[i] = (data[table[i].byte_index] >> table[i].bit) & 0x01;
    }
  }
}

CAN_frame AlertFrame(uint32_t id, const uint8_t* data) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  std::memcpy(frame.data.u8, data, 8);
  return frame;
}

void ExpectSame(const uint8_t* packed, const bool* expected, int count, const char* which) {
  for (int i = 0; i < count; i++) {
    EXPECT_EQ(teslaAlertGet(packed, i), expected[i]) << which << " index " << i;
  }
}

struct Shadow {
  bool bms[100] = {};
  bool pcs[94] = {};
  bool cp[96] = {};

  void Apply(uint32_t id, const uint8_t* data) {
    if (id == 0x320) {
      ApplyOld(bms, 100, kOldBms, data);
    } else if (id == 0x3A4) {
      ApplyOld(pcs, 94, kOldPcs, data);
    } else if (id == 0x31E) {
      ApplyOld(cp, 96, kOldCp, data);
    }
  }

  void Expect(const DATALAYER_INFO_TESLA& live) const {
    ExpectSame(live.BMS_alertMatrixActive, bms, 100, "BMS");
    ExpectSame(live.PCS_alertMatrixActive, pcs, 94, "PCS");
    ExpectSame(live.CP_alertMatrixActive, cp, 96, "CP");
  }
};

void Feed(TeslaBattery* pack, DATALAYER_INFO_TESLA* live, Shadow* shadow, uint32_t id, const uint8_t* data) {
  shadow->Apply(id, data);
  pack->handle_incoming_can_frame(AlertFrame(id, data));
  pack->update_values();
  shadow->Expect(*live);
}

class TeslaAlertMatrixTest : public ::testing::Test {
 protected:
  void SetUp() override {
    user_selected_battery_type = BatteryType::TeslaModel3Y;
    user_selected_tesla_GTW_chassisType = 2;
    contactor_control_enabled = false;
    std::memset(datalayer_extended.tesla.BMS_alertMatrixActive, 0,
                sizeof(datalayer_extended.tesla.BMS_alertMatrixActive));
    std::memset(datalayer_extended.tesla.PCS_alertMatrixActive, 0,
                sizeof(datalayer_extended.tesla.PCS_alertMatrixActive));
    std::memset(datalayer_extended.tesla.CP_alertMatrixActive, 0,
                sizeof(datalayer_extended.tesla.CP_alertMatrixActive));
    pack_ = new TeslaBattery();
    pack_->setup();
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;
  }

  TeslaBattery* pack_ = nullptr;
};

TEST_F(TeslaAlertMatrixTest, CopiedTableKeepsTheControlBits) {
  // Written here on purpose. A generated list that shifted these two would still
  // match itself, and the reset / SOC logic would move with it.
  EXPECT_EQ(kOldBms[74].mux, 2);
  EXPECT_EQ(kOldBms[74].byte_index, 3);
  EXPECT_EQ(kOldBms[74].bit, 4);
  EXPECT_EQ(kOldBms[99].mux, 2);
  EXPECT_EQ(kOldBms[99].byte_index, 7);
  EXPECT_EQ(kOldBms[99].bit, 7);
  for (const OldBit& bit : kOldBms) {
    EXPECT_FALSE(bit.byte_index == 0 && bit.bit < 4);
  }
}

TEST_F(TeslaAlertMatrixTest, EveryBitAndRandomPayloadMatchesTheOldDecode) {
  Shadow shadow;
  const uint32_t ids[3] = {0x320, 0x3A4, 0x31E};
  const OldBit* tables[3] = {kOldBms, kOldPcs, kOldCp};
  const int counts[3] = {100, 94, 96};

  for (int which = 0; which < 3; which++) {
    for (int i = 0; i < counts[which]; i++) {
      uint8_t data[8] = {};
      const OldBit& bit = tables[which][i];
      data[0] = bit.mux;
      data[bit.byte_index] = static_cast<uint8_t>(data[bit.byte_index] | (1u << bit.bit));
      Feed(pack_, &datalayer_extended.tesla, &shadow, ids[which], data);
    }
    for (uint8_t mux = 0; mux < 3; mux++) {
      uint8_t data[8];
      std::memset(data, 0xFF, sizeof(data));
      data[0] = static_cast<uint8_t>((data[0] & 0xF0) | mux);
      Feed(pack_, &datalayer_extended.tesla, &shadow, ids[which], data);
      std::memset(data, 0x00, sizeof(data));
      data[0] = mux;
      Feed(pack_, &datalayer_extended.tesla, &shadow, ids[which], data);
    }
  }

  // A mux this matrix does not use must leave every earlier bit where it was.
  uint8_t untouched[8];
  std::memset(untouched, 0xFF, sizeof(untouched));
  untouched[0] = static_cast<uint8_t>((untouched[0] & 0xF0) | 0x0F);
  Feed(pack_, &datalayer_extended.tesla, &shadow, 0x320, untouched);
  Feed(pack_, &datalayer_extended.tesla, &shadow, 0x3A4, untouched);
  Feed(pack_, &datalayer_extended.tesla, &shadow, 0x31E, untouched);

  std::mt19937 rng(0xA17E320u);
  std::uniform_int_distribution<int> byte_dist(0, 255);
  for (int trial = 0; trial < 64; trial++) {
    uint8_t data[8];
    for (int b = 0; b < 8; b++) {
      data[b] = static_cast<uint8_t>(byte_dist(rng));
    }
    for (uint32_t id : ids) {
      Feed(pack_, &datalayer_extended.tesla, &shadow, id, data);
    }
  }

  // A second publish with no new frame keeps the bits. The old path recopied
  // its members here; the table path leaves the datalayer as the frame wrote it.
  pack_->update_values();
  shadow.Expect(datalayer_extended.tesla);
}

bool ResetRequestWasSent(TeslaBattery* pack) {
  clear_transmitted_frames();
  for (int i = 0; i < 20; i++) {
    pack->transmit_can(1000);
  }
  for (const CAN_frame& frame : get_transmitted_frames()) {
    if (frame.ID == 0x602 && frame.data.u8[0] == 0x02 && frame.data.u8[1] == 0x27 && frame.data.u8[2] == 0x05) {
      return true;
    }
  }
  return false;
}

CAN_frame ContactorOpen() {
  CAN_frame frame = {};
  frame.ID = 0x212;
  frame.DLC = 8;
  frame.data.u8[1] = 1;  // OPEN
  return frame;
}

TEST_F(TeslaAlertMatrixTest, SocChangeLatchesFromMux2Byte3Bit4) {
  uint8_t data[8] = {};
  data[0] = 2;
  data[3] = 0x10;  // bit 4, SOC change. Bit 2 (CAC change) stays clear.
  pack_->handle_incoming_can_frame(AlertFrame(0x320, data));
  pack_->update_values();
  EXPECT_TRUE(teslaAlertGet(datalayer_extended.tesla.BMS_alertMatrixActive, 74));
  EXPECT_FALSE(teslaAlertGet(datalayer_extended.tesla.BMS_alertMatrixActive, 72));
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY_SOC_RECALIBRATION)->state, EVENT_STATE_ACTIVE_LATCHED);

  data[3] = 0;
  pack_->handle_incoming_can_frame(AlertFrame(0x320, data));
  pack_->update_values();
  EXPECT_FALSE(teslaAlertGet(datalayer_extended.tesla.BMS_alertMatrixActive, 74));
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY_SOC_RECALIBRATION)->state, EVENT_STATE_ACTIVE_LATCHED);
}

TEST_F(TeslaAlertMatrixTest, EcuResetBlockedFollowsMux2Byte7Bit7) {
  pack_->handle_incoming_can_frame(ContactorOpen());
  pack_->reset_BMS();
  pack_->update_values();
  EXPECT_TRUE(ResetRequestWasSent(pack_));

  TeslaBattery* blocked = new TeslaBattery();
  blocked->setup();
  uint8_t data[8] = {};
  data[0] = 2;
  data[7] = 0x80;
  blocked->handle_incoming_can_frame(ContactorOpen());
  blocked->handle_incoming_can_frame(AlertFrame(0x320, data));
  blocked->reset_BMS();
  blocked->update_values();
  EXPECT_TRUE(teslaAlertGet(datalayer_extended.tesla.BMS_alertMatrixActive, 99));
  EXPECT_FALSE(ResetRequestWasSent(blocked));
  delete blocked;
}

TEST_F(TeslaAlertMatrixTest, SecondBatteryWritesItsOwnMatrix) {
  std::memset(datalayer_extended.tesla_2.BMS_alertMatrixActive, 0,
              sizeof(datalayer_extended.tesla_2.BMS_alertMatrixActive));
  TeslaBattery second(&datalayer.battery2, &datalayer_extended.tesla_2, CAN_NATIVE);
  second.setup();
  uint8_t data[8] = {};
  data[0] = 0;
  data[0] = static_cast<uint8_t>(data[0] | 0x10);  // BMS_a001, mux 0 byte 0 bit 4
  Shadow shadow;
  Feed(&second, &datalayer_extended.tesla_2, &shadow, 0x320, data);
  EXPECT_FALSE(teslaAlertGet(datalayer_extended.tesla.BMS_alertMatrixActive, 0));
  EXPECT_TRUE(teslaAlertGet(datalayer_extended.tesla_2.BMS_alertMatrixActive, 0));
}

TEST_F(TeslaAlertMatrixTest, LoggedAlertFramesMatchTheOldDecode) {
  namespace fs = std::filesystem;
  const fs::path dir(TEST_CAN_LOG_DIR);
  ASSERT_TRUE(fs::is_directory(dir));
  Shadow shadow;
  int alert_frames = 0;
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    for (const CAN_frame& frame : parse_can_log_file(entry.path())) {
      if (frame.ID != 0x320 && frame.ID != 0x3A4 && frame.ID != 0x31E) {
        continue;
      }
      uint8_t data[8] = {};
      const int n = frame.DLC < 8 ? frame.DLC : 8;
      std::memcpy(data, frame.data.u8, n);
      Feed(pack_, &datalayer_extended.tesla, &shadow, frame.ID, data);
      alert_frames++;
    }
  }
  EXPECT_GT(alert_frames, 0);
}

void ExpectOnlyThisBit(uint8_t* packed, int count, int index) {
  teslaAlertSet(packed, index, true);
  for (int i = 0; i < count; i++) {
    EXPECT_EQ(teslaAlertGet(packed, i), i == index) << count << " alerts, set " << index << ", read " << i;
  }
  teslaAlertSet(packed, index, false);
  EXPECT_FALSE(teslaAlertGet(packed, index));
}

// Packing itself: every index, then the byte edges and the short last bytes.
TEST(TeslaAlertPack, EveryIndexSetsAndClearsWithoutTouchingItsNeighbors) {
  uint8_t bms[(100 + 7) / 8] = {};
  uint8_t pcs[(94 + 7) / 8] = {};
  uint8_t cp[(96 + 7) / 8] = {};
  for (int i = 0; i < 100; i++) {
    ExpectOnlyThisBit(bms, 100, i);
  }
  for (int i = 0; i < 94; i++) {
    ExpectOnlyThisBit(pcs, 94, i);
  }
  for (int i = 0; i < 96; i++) {
    ExpectOnlyThisBit(cp, 96, i);
  }

  for (int i = 0; i < 8; i++) {
    teslaAlertSet(bms, i, true);
  }
  EXPECT_EQ(bms[0], 0xFF);
  teslaAlertSet(bms, 7, false);
  EXPECT_EQ(bms[0], 0x7F);
  EXPECT_FALSE(teslaAlertGet(bms, 7));
  EXPECT_TRUE(teslaAlertGet(bms, 6));
  EXPECT_FALSE(teslaAlertGet(bms, 8));

  // Index 99 is bit 3 of the last BMS byte. Bits 4..7 of that byte are not alerts.
  std::memset(bms, 0, sizeof(bms));
  teslaAlertSet(bms, 96, true);
  teslaAlertSet(bms, 99, true);
  EXPECT_EQ(bms[12], 0x09);
  teslaAlertSet(bms, 99, false);
  EXPECT_EQ(bms[12], 0x01);
  EXPECT_TRUE(teslaAlertGet(bms, 96));
  EXPECT_FALSE(teslaAlertGet(bms, 99));

  // PCS stops at 93, which is bit 5 of byte 11. The two bits above it stay clear.
  teslaAlertSet(pcs, 93, true);
  EXPECT_EQ(pcs[11], 0x20);
  EXPECT_EQ(pcs[11] & 0xC0, 0);
  teslaAlertSet(pcs, 93, false);
  EXPECT_EQ(pcs[11], 0);

  // CP stops on a byte boundary. Index 95 is the top bit of byte 11.
  teslaAlertSet(cp, 88, true);
  teslaAlertSet(cp, 95, true);
  EXPECT_EQ(cp[11], 0x81);
  teslaAlertSet(cp, 95, false);
  EXPECT_EQ(cp[11], 0x01);
  EXPECT_TRUE(teslaAlertGet(cp, 88));
  EXPECT_FALSE(teslaAlertGet(cp, 95));
}

TEST_F(TeslaAlertMatrixTest, DetailPageReadsPackedAlertBits) {
  static char manufacture_date[] = "";
  datalayer_extended.tesla.battery_manufactureDate = manufacture_date;
  const auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };

  EXPECT_NE(page().find("Active Faults: 0"), std::string::npos);
  EXPECT_EQ(page().find("data-dtc-code"), std::string::npos);

  teslaAlertSet(datalayer_extended.tesla.BMS_alertMatrixActive, 0, true);
  teslaAlertSet(datalayer_extended.tesla.BMS_alertMatrixActive, 7, true);
  teslaAlertSet(datalayer_extended.tesla.BMS_alertMatrixActive, 8, true);
  teslaAlertSet(datalayer_extended.tesla.BMS_alertMatrixActive, 99, true);
  teslaAlertSet(datalayer_extended.tesla.PCS_alertMatrixActive, 0, true);
  teslaAlertSet(datalayer_extended.tesla.PCS_alertMatrixActive, 93, true);
  teslaAlertSet(datalayer_extended.tesla.CP_alertMatrixActive, 95, true);

  const std::string html = page();
  EXPECT_NE(html.find("Active Faults: 7"), std::string::npos);
  for (const char* code : {"100", "107", "108", "199", "200", "293", "395"}) {
    EXPECT_NE(html.find(std::string("data-dtc-code='") + code + "'"), std::string::npos) << code;
  }
  EXPECT_EQ(html.find("data-dtc-code='106'"), std::string::npos);
  EXPECT_EQ(html.find("data-dtc-code='109'"), std::string::npos);
  EXPECT_EQ(html.find("data-dtc-code='294'"), std::string::npos);
}

}  // namespace
