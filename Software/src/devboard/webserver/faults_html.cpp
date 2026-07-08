#include "faults_html.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../battery/Battery.h"

// Authoritative alert names, mapped from tesla-m3-pack-findings (firmware 2019.20.4.2).
// Index order matches datalayer_extended.tesla.*_alertMatrixActive[] filled in TeslaBattery::update_values().
static const char* const BMS_ALERT_NAMES[100] = {
    "BMS_a001_Pack_Config_Mismatch",
    "BMS_a017_SW_Brick_OV",
    "BMS_a018_SW_Brick_UV",
    "BMS_a019_SW_Module_OT",
    "BMS_a021_SW_Dr_Limits_Regulation",
    "BMS_a022_SW_Over_Current",
    "BMS_a023_SW_Stack_OV",
    "BMS_a024_SW_Islanded_Brick",
    "BMS_a025_SW_PwrBalance_Anomaly",
    "BMS_a026_SW_HFCurrent_Anomaly",
    "BMS_a034_SW_Passive_Isolation",
    "BMS_a035_SW_Isolation",
    "BMS_a036_SW_HvpHvilFault",
    "BMS_a037_SW_Flood_Port_Open",
    "BMS_a039_SW_DC_Link_Over_Voltage",
    "BMS_a041_SW_Power_On_Reset",
    "BMS_a042_SW_MPU_Error",
    "BMS_a043_SW_Watch_Dog_Reset",
    "BMS_a044_SW_Assertion",
    "BMS_a045_SW_Exception",
    "BMS_a046_SW_Task_Stack_Usage",
    "BMS_a047_SW_Task_Stack_Overflow",
    "BMS_a048_SW_Log_Upload_Request",
    "BMS_a050_SW_Brick_Voltage_MIA",
    "BMS_a051_SW_HVC_Vref_Bad",
    "BMS_a052_SW_PCS_MIA",
    "BMS_a053_SW_ThermalModel_Sanity",
    "BMS_a054_SW_Ver_Supply_Fault",
    "BMS_a055_SW_HvChain_Model_Fault",
    "BMS_a059_SW_Pack_Voltage_Sensing",
    "BMS_a060_SW_Leakage_Test_Failure",
    "BMS_a061_robinBrickOverVoltage",
    "BMS_a062_SW_BrickV_Imbalance",
    "BMS_a063_SW_ChargePort_Fault",
    "BMS_a064_SW_SOC_Imbalance",
    "BMS_a069_SW_Low_Power",
    "BMS_a071_SW_SM_TransCon_Not_Met",
    "BMS_a075_SW_Chg_Disable_Failure",
    "BMS_a076_SW_Dch_While_Charging",
    "BMS_a077_SW_Charger_Regulation",
    "BMS_a081_SW_Ctr_Close_Blocked",
    "BMS_a082_SW_Ctr_Force_Open",
    "BMS_a083_SW_Ctr_Close_Failure",
    "BMS_a084_SW_Sleep_Wake_Aborted",
    "BMS_a087_SW_Feim_Test_Blocked",
    "BMS_a088_SW_VcFront_MIA_InDrive",
    "BMS_a089_SW_VcFront_MIA",
    "BMS_a090_SW_Gateway_MIA",
    "BMS_a091_SW_ChargePort_MIA",
    "BMS_a092_SW_ChargePort_Mia_On_Hvs",
    "BMS_a094_SW_Drive_Inverter_MIA",
    "BMS_a099_SW_BMB_Communication",
    "BMS_a105_SW_One_Module_Tsense",
    "BMS_a106_SW_All_Module_Tsense",
    "BMS_a107_SW_Stack_Voltage_MIA",
    "BMS_a121_SW_NVRAM_Config_Error",
    "BMS_a122_SW_BMS_Therm_Irrational",
    "BMS_a123_SW_Internal_Isolation",
    "BMS_a126_SW_Thermistor_Failure",
    "BMS_a127_SW_shunt_SNA",
    "BMS_a128_SW_shunt_MIA",
    "BMS_a129_SW_VSH_Failure",
    "BMS_a130_IO_CAN_Error",
    "BMS_a131_Bleed_FET_Failure",
    "BMS_a132_HW_BMB_OTP_Uncorrctbl",
    "BMS_a134_SW_Delayed_Ctr_Off",
    "BMS_a135_HW_BMB_Diagnostics_Failure",
    "BMS_a136_SW_Module_OT_Warning",
    "BMS_a137_SW_Brick_UV_Warning",
    "BMS_a138_SW_Brick_OV_Warning",
    "BMS_a139_SW_DC_Link_V_Irrational",
    "BMS_a141_SW_BMB_Status_Warning",
    "BMS_a143_SW_CAC_Change",
    "BMS_a144_Hvp_Config_Mismatch",
    "BMS_a145_SW_SOC_Change",
    "BMS_a146_SW_Brick_Overdischarged",
    "BMS_a149_SW_Missing_Config_Block",
    "BMS_a151_SW_external_isolation",
    "BMS_a155_SW_Weak_short_impedence",
    "BMS_a156_SW_BMB_Vref_bad",
    "BMS_a157_SW_HVP_HVS_Comms",
    "BMS_a158_SW_HVP_HVI_Comms",
    "BMS_a159_SW_HVP_ECU_Error",
    "BMS_a161_SW_DI_Open_Request",
    "BMS_a162_SW_No_Power_For_Support",
    "BMS_a163_SW_Contactor_Mismatch",
    "BMS_a164_SW_Uncontrolled_Regen",
    "BMS_a165_SW_Pack_Partial_Weld",
    "BMS_a166_SW_Pack_Full_Weld",
    "BMS_a167_SW_FC_Partial_Weld",
    "BMS_a168_SW_FC_Full_Weld",
    "BMS_a169_SW_FC_Pack_Weld",
    "BMS_a170_SW_Limp_Mode",
    "BMS_a171_SW_Stack_Voltage_Sense",
    "BMS_a173_SW_Charge_Component_Fault",
    "BMS_a174_SW_Charge_Failure",
    "BMS_a176_SW_GracefulPowerOff",
    "BMS_a178_SW_Uncontrolled_Regen_PwrB",
    "BMS_a179_SW_Hvp_12V_Fault",
    "BMS_a180_SW_ECU_reset_blocked",
};

static const char* const PCS_ALERT_NAMES[94] = {
    "PCS_a001_chgHwInputOc",
    "PCS_a002_chgHwOutputOc",
    "PCS_a003_chgHwInputOv",
    "PCS_a004_chgHwIntBusOv",
    "PCS_a005_chgOutputOv",
    "PCS_a006_chgPrechargeFailedScr",
    "PCS_a007_chgPhaseTempHot",
    "PCS_a008_chgPhaseOverTemp",
    "PCS_a009_chgPfcCurrentRegulation",
    "PCS_a010_chgIntBusVRegulation",
    "PCS_a011_chgLlcCurrentRegulation",
    "PCS_a012_chgPfcIBandTracerFault",
    "PCS_a013_chgPrechargeFailedBoost",
    "PCS_a014_chgTempRationality",
    "PCS_a015_chg12vUv",
    "PCS_a016_chgAllPhasesFaulted",
    "PCS_a017_chgWallPowerRemoval",
    "PCS_a018_chgUnknownGridConfig",
    "PCS_a019_acChargePowerLimited",
    "PCS_a020_chgEnableLineMismatch",
    "PCS_a021_hvpMia",
    "PCS_a022_bmsMia",
    "PCS_a023_cpMia",
    "PCS_a024_vcfrontMia",
    "PCS_a025_cpu2Malfunction",
    "PCS_a026_watchdogAlarmed",
    "PCS_a027_chgInsufficientCooling",
    "PCS_a028_chgOutputUv",
    "PCS_a029_chgPowerRationality",
    "PCS_a030_canRationality",
    "PCS_a031_uiMia",
    "PCS_a032_gtwMia",
    "PCS_a033_hvBusUv",
    "PCS_a034_hvBusOv",
    "PCS_a035_lvBusUv",
    "PCS_a036_lvBusOv",
    "PCS_a037_resonantTankOc",
    "PCS_a038_claFaulted",
    "PCS_a039_sdModuleClkFault",
    "PCS_a040_dcdcMaxPowerReached",
    "PCS_a041_dcdcOverTemp",
    "PCS_a042_dcdcEnableLineMismatch",
    "PCS_a043_hvBusPrechargeFailure",
    "PCS_a044_12vSupportRegulation",
    "PCS_a045_hvBusLowImpedance",
    "PCS_a046_hvBusHighImpedence",
    "PCS_a047_lvBusLowImpedance",
    "PCS_a048_lvBusHighImpedance",
    "PCS_a049_dcdcTempRationality",
    "PCS_a050_dcdc12VsupportFaulted",
    "PCS_a051_chgIntBusUv",
    "PCS_a052_acVoltageNotPresent",
    "PCS_a053_chgInputVDropHigh",
    "PCS_a054_chgInputVDropTooHigh",
    "PCS_a055_chgLineImedanceHigh",
    "PCS_a056_chgLineImedanceTooHigh",
    "PCS_a057_chgInputOverFreq",
    "PCS_a058_chgInputUnderFreq",
    "PCS_a059_chgInputOvRms",
    "PCS_a060_chgInputOvPeak",
    "PCS_a061_chgVLineRationality",
    "PCS_a062_chgILineRationality",
    "PCS_a063_chgVOutRationality",
    "PCS_a064_chgIOutRationality",
    "PCS_a065_chgPllNotLocked",
    "PCS_a066_dcdcHvRationality",
    "PCS_a067_dcdcLvRationality",
    "PCS_a068_dcdcTankvRationality",
    "PCS_a069_chgPfcLineDidt",
    "PCS_a070_chgPfcLineDvdt",
    "PCS_a071_chgPfcILoopRationality",
    "PCS_a072_cpu2ClaStopped",
    "PCS_a073_unexpectedAcInputVoltage",
    "PCS_a074_hvBusDischargeFailure",
    "PCS_a075_hvBusDischargeTimeout",
    "PCS_a076_dcdcEnDeassertedErr",
    "PCS_a077_microGridEnergyLow",
    "PCS_a078_chgStopDcdcTooHot",
    "PCS_a079_eepromOperationError",
    "PCS_a080_damagedPhaseDetected",
    "PCS_a081_dcdcPchgTimeout",
    "PCS_a082_dcdcPchgUnsafeDiVoltage",
    "PCS_a083_triggerOdin",
    "PCS_a084_dcdcPchgStartVoltages",
    "PCS_a085_dcdcFetsNotSwitching",
    "PCS_a086_dcdcInsufficientCooling",
    "PCS_a087_nvramRecordStatusError",
    "PCS_a088_pchgParameters",
    "PCS_a089_hvBusDischargeIrrational",
    "PCS_a090_expectedAcVoltageSourceMissing",
    "PCS_a091_chgIntBusRationality",
    "PCS_a092_chgPowerLimitedByBusRipple",
    "PCS_a093_powerRailRationality",
    "PCS_a094_pcsDcdcNeedService",
};

static const char* const CP_ALERT_NAMES[96] = {
    "CP_a001_canRx",
    "CP_a002_canTx",
    "CP_a003_canError",
    "CP_a004_proximityRationality",
    "CP_a005_gbdcLiveDisconnect",
    "CP_a006_lostCommsBMS",
    "CP_a007_watchdog",
    "CP_a008_memoryError",
    "CP_a009_coverOpen",
    "CP_a010_pilotRationality",
    "CP_a011_eeprom",
    "CP_a012_ledDriver",
    "CP_a013_lostCommsGTW",
    "CP_a014_lostCommsCHG",
    "CP_a015_apsVov",
    "CP_a016_apsVuv",
    "CP_a017_fiveVov",
    "CP_a018_fiveVuv",
    "CP_a019_threeVov",
    "CP_a020_threeVuv",
    "CP_a021_zeroVov",
    "CP_a022_zeroVuv",
    "CP_a023_gbdcSessionFailed",
    "CP_a024_ledsUC",
    "CP_a025_ledsOC",
    "CP_a026_networkManagement",
    "CP_a027_doorSensorOutOfSpec",
    "CP_a028_insertEnableMismatch",
    "CP_a029_doorClosedProxPilot",
    "CP_a030_busOff",
    "CP_a031_doorClosedCommandedOpen",
    "CP_a032_doorOpenExpectedClosed",
    "CP_a033_spiOpen",
    "CP_a034_calibrationIncomplete",
    "CP_a035_latchMovement_1",
    "CP_a036_latchNotDisengaged",
    "CP_a037_latchNotEngaged",
    "CP_a038_latchNotBlocking",
    "CP_a039_latchMovement_2",
    "CP_a040_doNotUse",
    "CP_a041_doorSensorUnplugged",
    "CP_a042_doorAssemblyBroken",
    "CP_a043_doorPotIrrational",
    "CP_a044_lostCommsHVP",
    "CP_a045_lostCommsVCSEC",
    "CP_a046_lostCommsEVSE",
    "CP_a047_lostCommsVCFRONT",
    "CP_a048_lostCommsUI",
    "CP_a049_multipleCablesDetected",
    "CP_a050_latchNotConnected",
    "CP_a051_doorInductiveSensorMIA",
    "CP_a052_evseNotSupported",
    "CP_a053_proxLatchedNoPilot",
    "CP_a054_cableNotSecured",
    "CP_a055_chargeStoppedNoPilot",
    "CP_a056_proxDisconnected",
    "CP_a057_evseFaulted",
    "CP_a058_acChargingBlocked",
    "CP_a059_swcanError",
    "CP_a060_lostCommsPCS",
    "CP_a061_uhfReceiverMIA",
    "CP_a062_scOutOfService",
    "CP_a063_scUpdateInProgress",
    "CP_a064_superchargingBlocked",
    "CP_a065_selfTestFailed",
    "CP_a066_proxLatchedIdlePilot",
    "CP_a067_gbdcConnFault",
    "CP_a068_doorSensorMismatch",
    "CP_a069_doorInductiveSensorError",
    "CP_a070_doorInductiveSensorReset",
    "CP_a071_exiDecodeFailure",
    "CP_a072_v2gEvccTimeout",
    "CP_a073_iecComboShutdown",
    "CP_a074_failedToEstablishV2gComm",
    "CP_a075_v2gCommsFailure",
    "CP_a076_LDC1612errorWatchdog",
    "CP_a077_invalidMacAddress",
    "CP_a078_latchNotDisengagedCold",
    "CP_a079_cableNotSecuredCold",
    "CP_a080_taskStackOverflow",
    "CP_a081_swException",
    "CP_a082_powerOnReset",
    "CP_a083_watchdogTraceData",
    "CP_a084_proximityPeDisconnected",
    "CP_a085_dcPinTempFaulted",
    "CP_a086_dcPinTempIrrational",
    "CP_a087_dcTempModelFault",
    "CP_a088_dcTempModelDeviation",
    "CP_a089_plcConfigMismatch",
    "CP_a090_ccsEvseLowIso",
    "CP_a091_wrongSuperchargerHandle",
    "CP_a092_modemAppLoadFailed",
    "CP_a093_modemLoadedWithReset",
    "CP_a094_inductiveResetSuccessful",
    "CP_a095_thermalDcLimitActive",
    "CP_a096_pilotWake",
};


static int count_active(const bool* active, int n) {
  int c = 0;
  for (int i = 0; i < n; i++) {
    if (active[i]) {
      c++;
    }
  }
  return c;
}

// Appends one ECU's alert list to content, styled like the BE events page (.event-log / .event rows).
// Rendered directly into the single pre-reserved content String to keep peak heap usage low.
static void render_alert_table(String& content, const char* title, const char* canId, const bool* active,
                               const char* const* names, int n) {
  int ecu_active = count_active(active, n);

  content += "<div style=\"background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px\">";
  content += "<h3 style='margin:6px 0'>";
  content += title;
  content += " <span style='color:#8fa5b0;font-weight:normal'>(";
  content += canId;
  content += ")</span> &mdash; ";
  if (ecu_active > 0) {
    content += "<span style='color:#ff5c5c'>";
    content += ecu_active;
    content += " active</span>";
  } else {
    content += "<span style='color:#04b34f'>no active faults</span>";
  }
  content += " / ";
  content += n;
  content += "</h3><div class='event-log'>";
  for (int i = 0; i < n; i++) {
    if (active[i]) {
      content += "<div class='event act' data-a=1>";
    } else {
      content += "<div class='event' data-a=0>";
    }
    content += names[i];
    content += "</div>";
  }
  content += "</div></div>";
}

String faults_processor(const String& var) {
  if (var == "X") {
    String content = "";
    content.reserve(26000);  // one allocation up front; full page is ~23 KB (kept small to fit ESP32 heap)

    // Styling mirrors the BE events page (dark log rows, rounded panels, standard button style).
    content += "<style>body{background-color:#000;color:#fff}"
               ".event-log{display:flex;flex-direction:column}"
               ".event{border:1px solid #fff;padding:8px;text-align:left;word-break:break-word}"
               ".event[data-a]:nth-child(even){background-color:#455a64}"
               ".event[data-a]:nth-child(odd){background-color:#394b52}"
               ".event[data-a].act{background-color:#a6192e;color:#fff}"
               "button{background-color:#505E67;color:white;border:none;padding:10px 20px;margin:6px 6px 20px 0;"
               "cursor:pointer;border-radius:10px}button:hover{background-color:#3A4A52}</style>";

    bool isTesla = (user_selected_battery_type == BatteryType::TeslaModel3Y ||
                    user_selected_battery_type == BatteryType::TeslaModelSX);

    if (!isTesla) {
      content += "<div style=\"background-color:#303e47;padding:14px;margin-bottom:10px;border-radius:25px\">";
      content += "<h4 style='color:#ff9900'>This page shows Tesla alert-matrix faults. The active battery is not a "
                 "Tesla Model 3/Y or S/X, so no fault data is available.</h4></div>";
      content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";
      return content;
    }

    // Pre-count so the summary banner can show the total before the (large) tables are appended.
    int total_active = count_active(datalayer_extended.tesla.BMS_alertMatrixActive, 100) +
                       count_active(datalayer_extended.tesla.PCS_alertMatrixActive, 94) +
                       count_active(datalayer_extended.tesla.CP_alertMatrixActive, 96);

    content += "<div style=\"background-color:#303e47;padding:12px;margin-bottom:10px;border-radius:25px\">";
    if (total_active > 0) {
      content += "<h3 style='margin:4px 0;color:#ff5c5c'>&#9888; ";
      content += total_active;
      content += " active fault(s) across BMS / PCS / CP</h3>";
    } else {
      content += "<h3 style='margin:4px 0;color:#04b34f'>&#10004; No active faults</h3>";
    }
    content += "<label style='cursor:pointer'><input type='checkbox' id='oa'> Show only active faults</label>";

    // Tables rendered directly into the single pre-reserved content String.
    render_alert_table(content, "BMS alert matrix", "0x320", datalayer_extended.tesla.BMS_alertMatrixActive,
                       BMS_ALERT_NAMES, 100);
    render_alert_table(content, "PCS alert matrix", "0x3A4", datalayer_extended.tesla.PCS_alertMatrixActive,
                       PCS_ALERT_NAMES, 94);
    render_alert_table(content, "CP alert matrix", "0x31E", datalayer_extended.tesla.CP_alertMatrixActive,
                       CP_ALERT_NAMES, 96);

    content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";
    content += "<button onclick='location.reload(true)'>Refresh</button>";

    // "Show only active" defaults to ON and is persisted in localStorage so it survives the 15 s auto-refresh
    // and manual reloads.
    content += "<script>(function(){var K='beTeslaFaultsActiveOnly';var cb=document.getElementById('oa');"
               "var s=localStorage.getItem(K);cb.checked=(s===null)?true:(s==='1');"
               "function ap(){var o=cb.checked;document.querySelectorAll('.event[data-a]').forEach(function(r){"
               "r.style.display=(o&&r.getAttribute('data-a')!=='1')?'none':'';});}"
               "cb.addEventListener('change',function(){try{localStorage.setItem(K,cb.checked?'1':'0');}catch(e){}ap();});"
               "ap();setTimeout(function(){location.reload(true);},15000);})();</script>";

    return content;
  }
  return String();
}
