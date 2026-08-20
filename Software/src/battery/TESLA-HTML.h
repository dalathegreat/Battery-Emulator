#ifndef _TESLA_HTML_H
#define _TESLA_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

static void appendFault(String& string, const String& name, bool faultActive) {
  if (!faultActive) {
    return;
  }
  string += "<h4>";
  string += name;
  string += ": " + TR(TrKey::DRV_ACTIVE) + "</h4>";
}

class TeslaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    float beginning_of_life = static_cast<float>(datalayer_extended.tesla.battery_beginning_of_life);
    float battTempPct = static_cast<float>(datalayer_extended.tesla.battery_battTempPct) * 0.4f;
    float dcdcLvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 0.0390625f;
    float dcdcHvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcHvBusVolt) * 0.146484f;
    float dcdcLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent) * 0.1f;
    float nominal_full_pack_energy =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy) * 0.1f;
    float nominal_full_pack_energy_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy_m0) * 0.02f;
    float nominal_energy_remaining =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining) * 0.1f;
    float nominal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining_m0) * 0.02f;
    float ideal_energy_remaining = static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining) * 0.1f;
    float ideal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining_m0) * 0.02f;
    float energy_to_charge_complete =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete) * 0.1f;
    float energy_to_charge_complete_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete_m1) * 0.02f;
    float energy_buffer = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer) * 0.1f;
    float energy_buffer_m1 = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer_m1) * 0.01f;
    float expected_energy_remaining_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_expected_energy_remaining_m1) * 0.02f;
    float total_discharge = static_cast<float>(datalayer.battery.status.total_discharged_battery_Wh) * 0.001f;
    float total_charge = static_cast<float>(datalayer.battery.status.total_charged_battery_Wh) * 0.001f;
    float packMass = static_cast<float>(datalayer_extended.tesla.battery_packMass);
    float platformMaxBusVoltage =
        static_cast<float>(datalayer_extended.tesla.battery_platformMaxBusVoltage) * 0.1f + 375;
    float bms_min_voltage = static_cast<float>(datalayer_extended.tesla.BMS_min_voltage) * 0.01f * 2;
    float bms_max_voltage = static_cast<float>(datalayer_extended.tesla.BMS_max_voltage) * 0.01f * 2;
    float max_charge_current = static_cast<float>(datalayer_extended.tesla.battery_max_charge_current);
    float max_discharge_current = static_cast<float>(datalayer_extended.tesla.battery_max_discharge_current);
    float soc_ave = static_cast<float>(datalayer_extended.tesla.battery_soc_ave) * 0.1f;
    float soc_max = static_cast<float>(datalayer_extended.tesla.battery_soc_max) * 0.1f;
    float soc_min = static_cast<float>(datalayer_extended.tesla.battery_soc_min) * 0.1f;
    float soc_ui = static_cast<float>(datalayer_extended.tesla.battery_soc_ui) * 0.1f;
    float BrickVoltageMax = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMax) * 0.002f;
    float BrickVoltageMin = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMin) * 0.002f;
    //float BrickModelTMax = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMax) * 0.5 - 40;
    //float BrickModelTMin = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMin) * 0.5 - 40;
    float isolationResistance = static_cast<float>(datalayer_extended.tesla.BMS_isolationResistance) * 10;
    float PCS_dcdcMaxOutputCurrentAllowed =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcMaxOutputCurrentAllowed) * 0.1f;
    float PCS_dcdcTemp = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTemp) * 0.1f + 40.0f;
    float PCS_ambientTemp = static_cast<float>(datalayer_extended.tesla.PCS_ambientTemp) * 0.1f + 40.0f;
    float PCS_chgPhATemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhATemp) * 0.1f + 40.0f;
    float PCS_chgPhBTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhBTemp) * 0.1f + 40.0f;
    float PCS_chgPhCTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhCTemp) * 0.1f + 40.0f;
    float BMS_maxRegenPower = static_cast<float>(datalayer_extended.tesla.BMS_maxRegenPower) * 0.01f;
    float BMS_maxDischargePower = static_cast<float>(datalayer_extended.tesla.BMS_maxDischargePower) * 0.013f;
    //float BMS_maxStationaryHeatPower = static_cast<float>(datalayer_extended.tesla.BMS_maxStationaryHeatPower) * 0.01f;
    //float BMS_hvacPowerBudget = static_cast<float>(datalayer_extended.tesla.BMS_hvacPowerBudget) * 0.02;
    float BMS_powerDissipation = static_cast<float>(datalayer_extended.tesla.BMS_powerDissipation) * 0.02f;
    float BMS_flowRequest = static_cast<float>(datalayer_extended.tesla.BMS_flowRequest) * 0.3f;
    float BMS_inletActiveCoolTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveCoolTargetT) * 0.25f - 25;
    float BMS_inletPassiveTargetT = static_cast<float>(datalayer_extended.tesla.BMS_inletPassiveTargetT) * 0.25f - 25;
    float BMS_inletActiveHeatTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveHeatTargetT) * 0.25f - 25;
    float BMS_packTMin = static_cast<float>(datalayer_extended.tesla.BMS_packTMin) * 0.25f - 25;
    float BMS_packTMax = static_cast<float>(datalayer_extended.tesla.BMS_packTMax) * 0.25f - 25;
    float PCS_dcdcMaxLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.PCS_dcdcMaxLvOutputCurrent) * 0.1f;
    float PCS_dcdcCurrentLimit = static_cast<float>(datalayer_extended.tesla.PCS_dcdcCurrentLimit) * 0.1f;
    float PCS_dcdcLvOutputCurrentTempLimit =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcLvOutputCurrentTempLimit) * 0.1f;
    float PCS_dcdcUnifiedCommand = static_cast<float>(datalayer_extended.tesla.PCS_dcdcUnifiedCommand) * 0.001f;
    float PCS_dcdcCLAControllerOutput =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcCLAControllerOutput * 0.001f);
    float PCS_dcdcTankVoltage = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltage);
    float PCS_dcdcTankVoltageTarget = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltageTarget);
    float PCS_dcdcClaCurrentFreq = static_cast<float>(datalayer_extended.tesla.PCS_dcdcClaCurrentFreq) * 0.0976563f;
    float PCS_dcdcTCommMeasured = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTCommMeasured) * 0.00195313f;
    float PCS_dcdcShortTimeUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcShortTimeUs) * 0.000488281f;
    float PCS_dcdcHalfPeriodUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcHalfPeriodUs) * 0.000488281f;
    float PCS_dcdcIntervalMaxFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxFrequency);
    float PCS_dcdcIntervalMaxHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxHvBusVolt) * 0.1f;
    float PCS_dcdcIntervalMaxLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvBusVolt) * 0.1f;
    float PCS_dcdcIntervalMaxLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvOutputCurr);
    float PCS_dcdcIntervalMinFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinFrequency);
    float PCS_dcdcIntervalMinHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinHvBusVolt) * 0.1f;
    float PCS_dcdcIntervalMinLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvBusVolt) * 0.1f;
    float PCS_dcdcIntervalMinLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvOutputCurr);
    float PCS_dcdc12vSupportLifetimekWh =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdc12vSupportLifetimekWh) * 0.01f;
    float HVP_hvp1v5Ref = static_cast<float>(datalayer_extended.tesla.HVP_hvp1v5Ref) * 0.1f;
    float HVP_shuntCurrentDebug = static_cast<float>(datalayer_extended.tesla.HVP_shuntCurrentDebug) * 0.1f;
    float HVP_dcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkVoltage) * 0.1f;
    float HVP_packVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packVoltage) * 0.1f;
    //float HVP_fcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkVoltage) * 0.1f;
    float HVP_packContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packContVoltage) * 0.1f;
    //float HVP_packNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_packNegativeV) * 0.1f;
    //float HVP_packPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_packPositiveV) * 0.1f;
    float HVP_pyroAnalog = static_cast<float>(datalayer_extended.tesla.HVP_pyroAnalog) * 0.1f;
    //float HVP_dcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkNegativeV) * 0.1f;
    //float HVP_dcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkPositiveV) * 0.1f;
    //float HVP_fcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkNegativeV) * 0.1f;
    //float HVP_fcContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_fcContCoilCurrent) * 0.1f;
    //float HVP_fcContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcContVoltage) * 0.1f;
    float HVP_hvilInVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilInVoltage) * 0.1f;
    float HVP_hvilOutVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilOutVoltage) * 0.1f;
    //float HVP_fcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkPositiveV) * 0.1f;
    float HVP_packContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_packContCoilCurrent) * 0.1f;
    float HVP_battery12V = static_cast<float>(datalayer_extended.tesla.HVP_battery12V) * 0.1f;
    //float HVP_shuntRefVoltageDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntRefVoltageDbg) * 0.001f;
    //float HVP_shuntAuxCurrentDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAuxCurrentDbg) * 0.1f;
    //float HVP_shuntBarTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntBarTempDbg) * 0.01f;
    //float HVP_shuntAsicTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAsicTempDbg) * 0.01f;

    static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                          "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                          "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
    const String hvilStatusState[] = {TR(TrKey::DRV_UNKNOWN_CONTACTORS_OPEN),
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
    static const char* contactorState[] = {"SNA",        "OPEN",       "PRECHARGE",   "BLOCKED",
                                           "PULLED_IN",  "OPENING",    "ECONOMIZED",  "WELDED",
                                           "UNKNOWN(8)", "UNKNOWN(9)", "UNKNOWN(10)", "UNKNOWN(11)"};
    static const char* BMS_state[] = {"STANDBY",     "DRIVE", "SUPPORT", "CHARGE", "FEIM",
                                      "CLEAR_FAULT", "FAULT", "WELD",    "TEST",   "SNA"};
    static const char* BMS_contactorState[] = {"SNA", "OPEN", "OPENING", "CLOSING", "CLOSED", "WELDED", "BLOCKED"};
    static const char* BMS_hvState[] = {"DOWN",          "COMING_UP",        "GOING_DOWN", "UP_FOR_DRIVE",
                                        "UP_FOR_CHARGE", "UP_FOR_DC_CHARGE", "UP"};
    static const char* BMS_uiChargeStatus[] = {"DISCONNECTED", "NO_POWER",        "ABOUT_TO_CHARGE",
                                               "CHARGING",     "CHARGE_COMPLETE", "CHARGE_STOPPED"};
    const String PCS_dcdcStatus[] = {"IDLE", TR(TrKey::DRV_ACTIVE), "FAULTED"};
    static const char* PCS_dcdcMainState[] = {"STANDBY",          "12V_SUPPORT_ACTIVE", "PRECHARGE_STARTUP",
                                              "PRECHARGE_ACTIVE", "DIS_HVBUS_ACTIVE",   "SHUTDOWN",
                                              "FAULTED"};
    static const char* PCS_dcdcSubState[] = {"PWR_UP_INIT",
                                             "STANDBY",
                                             "12V_SUPPORT_ACTIVE",
                                             "DIS_HVBUS",
                                             "PCHG_FAST_DIS_HVBUS",
                                             "PCHG_SLOW_DIS_HVBUS",
                                             "PCHG_DWELL_CHARGE",
                                             "PCHG_DWELL_WAIT",
                                             "PCHG_DI_RECOVERY_WAIT",
                                             "PCHG_ACTIVE",
                                             "PCHG_FLT_FAST_DIS_HVBUS",
                                             "SHUTDOWN",
                                             "12V_SUPPORT_FAULTED",
                                             "DIS_HVBUS_FAULTED",
                                             "PCHG_FAULTED",
                                             "CLEAR_FAULTS",
                                             "FAULTED",
                                             "NUM"};
    static const char* BMS_powerLimitState[] = {"NOT_CALCULATED_FOR_DRIVE", "CALCULATED_FOR_DRIVE"};
    //static const char* HVP_status[] = {"INVALID", "NOT_AVAILABLE", "STALE", "VALID"};
    const String HVP_contactor[] = {"NOT_ACTIVE", TR(TrKey::DRV_ACTIVE), "COMPLETED"};
    const String falseTrue[] = {TR(TrKey::DRV_FALSE), TR(TrKey::DRV_TRUE)};
    const String noYes[] = {"No", TR(TrKey::DRV_YES)};

    //Main battery info
    char readableBatterySerialNumber[15];  // One extra space for null terminator
    memcpy(readableBatterySerialNumber, datalayer_extended.tesla.battery_serialNumber,
           sizeof(datalayer_extended.tesla.battery_serialNumber));
    readableBatterySerialNumber[14] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_BATTERY_SERIAL_NUMBER, String(readableBatterySerialNumber));
    char readableBatteryPartNumber[13];  // One extra space for null terminator
    memcpy(readableBatteryPartNumber, datalayer_extended.tesla.battery_partNumber,
           sizeof(datalayer_extended.tesla.battery_partNumber));
    readableBatteryPartNumber[12] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_BATTERY_PART_NUMBER, String(readableBatteryPartNumber));
    //0x3C4 PCS_info
    char readablePCSPartNumber[13];  // One extra space for null terminator
    memcpy(readablePCSPartNumber, datalayer_extended.tesla.PCS_partNumber,
           sizeof(datalayer_extended.tesla.PCS_partNumber));
    readablePCSPartNumber[12] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_PCS_PART_NUMBER, String(readablePCSPartNumber));
    tr_h4(content, TrKey::DRV_BATTERY_MANUFACTURE_DATE, String(datalayer_extended.tesla.battery_manufactureDate));
    tr_h4(content, TrKey::DRV_BATTERY_PACK_MASS, String(packMass), " KG");
    //0x3D2 978 BMS_kwhCounter
    tr_h4(content, TrKey::DRV_BATTERY_TOTAL_DISCHARGE, String(total_discharge), " kWh");
    tr_h4(content, TrKey::DRV_BATTERY_TOTAL_CHARGE, String(total_charge), " kWh");
    //0x20A 522 HVP_contactorState + HVIL
    //content += "<h4>HVIL Fault: " + String(noYes[datalayer_extended.tesla.BMS_hvilFault]) + "</h4>";
    tr_h4(content, TrKey::DRV_HVIL_STATUS, String(hvilStatusState[datalayer_extended.tesla.hvil_status]));
    tr_h4(content, TrKey::DRV_HVP_CONTACTOR_STATE,
          String(contactorText[datalayer_extended.tesla.packContactorSetState]));
    tr_h4(content, TrKey::DRV_BMS_CONTACTOR_STATE,
          String(BMS_contactorState[datalayer_extended.tesla.BMS_contactorState]));
    tr_h4(content, TrKey::DRV_NEGATIVE_CONTACTOR,
          String(contactorState[datalayer_extended.tesla.packContNegativeState]));
    tr_h4(content, TrKey::DRV_POSITIVE_CONTACTOR,
          String(contactorState[datalayer_extended.tesla.packContPositiveState]));
    if (datalayer_extended.tesla.packContactorSetState == 5) {  //Closed
      tr_h4(content, TrKey::DRV_CLOSING_BLOCKED,
            String(noYes[datalayer_extended.tesla.packCtrsClosingBlocked]) + " " + TR(TrKey::DRV_ALREADY_CLOSED));
    } else {
      tr_h4(content, TrKey::DRV_CLOSING_BLOCKED, String(noYes[datalayer_extended.tesla.packCtrsClosingBlocked]));
    }
    tr_h4(content, TrKey::DRV_PYROTEST_PROGRESS, String(noYes[datalayer_extended.tesla.pyroTestInProgress]));
    tr_h4(content, TrKey::DRV_CONTACTORS_OPEN_NOW_REQUESTED,
          String(noYes[datalayer_extended.tesla.battery_packCtrsOpenNowRequested]));
    tr_h4(content, TrKey::DRV_CONTACTORS_OPEN_REQUESTED,
          String(noYes[datalayer_extended.tesla.battery_packCtrsOpenRequested]));
    tr_h4(content, TrKey::DRV_CONTACTORS_REQUEST_STATUS,
          String(HVP_contactor[datalayer_extended.tesla.battery_packCtrsRequestStatus]));
    tr_h4(content, TrKey::DRV_CONTACTORS_RESET_REQUEST_REQUIRED,
          String(noYes[datalayer_extended.tesla.battery_packCtrsResetRequestRequired]));
    tr_h4(content, TrKey::DRV_DC_LINK_ALLOWED_ENERGIZE,
          String(noYes[datalayer_extended.tesla.battery_dcLinkAllowedToEnergize]));
    // Comment what data you would like to display, order can be changed.
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == false) {
      content += "<h3>BMS 0x352 w/o mux</h3>";  //if using older BMS <2021 and comment 0x352 without MUX
      tr_h4(content, TrKey::DRV_CALCULATED_SOH, String(nominal_full_pack_energy * 100 / beginning_of_life));
      tr_h4(content, TrKey::DRV_NOMINAL_FULL_PACK_ENERGY, String(nominal_full_pack_energy), " kWh");
      tr_h4(content, TrKey::DRV_NOMINAL_ENERGY_REMAINING, String(nominal_energy_remaining), " kWh");
      tr_h4(content, TrKey::DRV_IDEAL_ENERGY_REMAINING, String(ideal_energy_remaining), " kWh");
      tr_h4(content, TrKey::DRV_ENERGY_CHARGE_COMPLETE, String(energy_to_charge_complete), " kWh");
      tr_h4(content, TrKey::DRV_ENERGY_BUFFER, String(energy_buffer), " kWh");
      tr_h4(content, TrKey::DRV_FULL_CHARGE_COMPLETE,
            String(noYes[datalayer_extended.tesla.battery_full_charge_complete]));  //bool
    }
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == true) {
      content += "<h3>BMS 0x352 w/ mux</h3>";  //if using newer BMS >2021 and comment 0x352 with MUX
      tr_h4(content, TrKey::DRV_CALCULATED_SOH, String(nominal_full_pack_energy_m0 * 100 / beginning_of_life));
      tr_h4(content, TrKey::DRV_NOMINAL_FULL_PACK_ENERGY, String(nominal_full_pack_energy_m0), " kWh");
      tr_h4(content, TrKey::DRV_NOMINAL_ENERGY_REMAINING, String(nominal_energy_remaining_m0), " kWh");
      tr_h4(content, TrKey::DRV_IDEAL_ENERGY_REMAINING, String(ideal_energy_remaining_m0), " kWh");
      tr_h4(content, TrKey::DRV_ENERGY_CHARGE_COMPLETE, String(energy_to_charge_complete_m1), " kWh");
      tr_h4(content, TrKey::DRV_ENERGY_BUFFER, String(energy_buffer_m1), " kWh");
      tr_h4(content, TrKey::DRV_EXPECTED_ENERGY_REMAINING, String(expected_energy_remaining_m1), " kWh");
      tr_h4(content, TrKey::DRV_FULLY_CHARGED, String(noYes[datalayer_extended.tesla.battery_fully_charged]));
    }
    //0x212 530 BMS_status
    tr_h4(content, TrKey::DRV_ISOLATION_RESISTANCE, String(isolationResistance), " kOhms");
    tr_h4(content, TrKey::DRV_BMS_STATE, String(BMS_state[datalayer_extended.tesla.BMS_state]));
    tr_h4(content, TrKey::DRV_BMS_HV_STATE, String(BMS_hvState[datalayer_extended.tesla.BMS_hvState]));
    tr_h4(content, TrKey::DRV_BMS_UI_CHARGE_STATUS,
          String(BMS_uiChargeStatus[datalayer_extended.tesla.BMS_uiChargeStatus]));
    tr_h4(content, TrKey::DRV_BMS_BUILDCONFIGID, String(datalayer_extended.tesla.BMS_info_buildConfigId));
    tr_h4(content, TrKey::DRV_BMS_HARDWAREID, String(datalayer_extended.tesla.BMS_info_hardwareId));
    tr_h4(content, TrKey::DRV_BMS_COMPONENTID, String(datalayer_extended.tesla.BMS_info_componentId));
    appendFault(content, TR(TrKey::DRV_BMS_PCS_PWM_ENABLED), datalayer_extended.tesla.BMS_pcsPwmEnabled);
    //0x292 658 BMS_socStates
    tr_h4(content, TrKey::DRV_BATTERY_BEGINNING_LIFE, String(beginning_of_life), " kWh");
    tr_h4(content, TrKey::DRV_BATTERY_SOC_UI, String(soc_ui), " ");
    tr_h4(content, TrKey::DRV_BATTERY_SOC_AVE, String(soc_ave), " ");
    tr_h4(content, TrKey::DRV_BATTERY_SOC_MAX, String(soc_max), " ");
    tr_h4(content, TrKey::DRV_BATTERY_SOC_MIN, String(soc_min), " ");
    tr_h4(content, TrKey::DRV_BATTERY_TEMP_PERCENT, String(battTempPct), " ");
    //0x2B4 PCS_dcdcRailStatus
    tr_h4(content, TrKey::DRV_PCS_LV_OUTPUT, String(dcdcLvOutputCurrent), " A");
    tr_h4(content, TrKey::DRV_PCS_LV_BUS, String(dcdcLvBusVolt), " V");
    tr_h4(content, TrKey::DRV_PCS_HV_BUS, String(dcdcHvBusVolt), " V");
    //0x392 BMS_packConfig
    //content += "<h4>packConfigMultiplexer: " + String(datalayer_extended.tesla.battery_packConfigMultiplexer) + "</h4>"; // Not giving useable data
    //content += "<h4>moduleType: " + String(datalayer_extended.tesla.battery_moduleType) + "</h4>";  // Not giving useable data
    //content += "<h4>reserveConfig: " + String(datalayer_extended.tesla.battery_reservedConfig) + "</h4>";  // Not giving useable data
    //tr_h4(content, TrKey::DRV_BATTERY_PACK_MASS, String(packMass), " KG");
    tr_h4(content, TrKey::DRV_PLATFORM_MAX_BUS_VOLTAGE, String(platformMaxBusVoltage), " V");
    //0x2D2 722 BMSVAlimits
    tr_h4(content, TrKey::DRV_BMS_MIN_VOLTAGE, String(bms_min_voltage), " V");
    tr_h4(content, TrKey::DRV_BMS_MAX_VOLTAGE, String(bms_max_voltage), " V");
    tr_h4(content, TrKey::DRV_MAX_CHARGE_CURRENT, String(max_charge_current), " A");
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_CURRENT, String(max_discharge_current), " A");
    //0x332 818 BMS_bmbMinMax
    tr_h4(content, TrKey::DRV_BRICK_VOLTAGE_MAX, String(BrickVoltageMax), " V");
    tr_h4(content, TrKey::DRV_BRICK_VOLTAGE_MIN, String(BrickVoltageMin), " V");
    tr_h4(content, TrKey::DRV_BRICK_TEMP_MAX_NUM, String(datalayer_extended.tesla.battery_BrickTempMaxNum), " ");
    tr_h4(content, TrKey::DRV_BRICK_TEMP_MIN_NUM, String(datalayer_extended.tesla.battery_BrickTempMinNum), " ");
    //content += "<h4>Brick Model Temp Max: " + String(BrickModelTMax) + " C</h4>";// Not giving useable data
    //content += "<h4>Brick Model Temp Min: " + String(BrickModelTMin) + " C</h4>";// Not giving useable data
    //0x2A4 676 PCS_thermalStatus
    tr_h4(content, TrKey::DRV_PCS_DCDC_TEMP, String(PCS_dcdcTemp), " DegC");
    tr_h4(content, TrKey::DRV_PCS_AMBIENT_TEMP, String(PCS_ambientTemp), " DegC");
    tr_h4(content, TrKey::DRV_PCS_CHG_PHA_TEMP, String(PCS_chgPhATemp), " DegC");
    tr_h4(content, TrKey::DRV_PCS_CHG_PHB_TEMP, String(PCS_chgPhBTemp), " DegC");
    tr_h4(content, TrKey::DRV_PCS_CHG_PHC_TEMP, String(PCS_chgPhCTemp), " DegC");
    //0x252 594 BMS_powerAvailable
    tr_h4(content, TrKey::DRV_MAX_REGEN_POWER, String(BMS_maxRegenPower), " kW");
    tr_h4(content, TrKey::DRV_MAX_DISCHARGE_POWER, String(BMS_maxDischargePower), " kW");
    //content += "<h4>Max Stationary Heat Power: " + String(BMS_maxStationaryHeatPower) + " kWh</h4>"; // Not giving useable data
    //content += "<h4>HVAC Power Budget: " + String(BMS_hvacPowerBudget) + " kW</h4>"; // Not giving useable data
    //content += "<h4>Not Enough Power For Heat Pump: " + String(noYes[datalayer_extended.tesla.BMS_notEnoughPowerForHeatPump]) + "</h4>"; // Not giving useable data
    tr_h4(content, TrKey::DRV_POWER_LIMIT_STATE,
          String(BMS_powerLimitState[datalayer_extended.tesla.BMS_powerLimitState]));
    //content += "<h4>Inverter TQF: " + String(datalayer_extended.tesla.BMS_inverterTQF) + "</h4>"; // Not giving useable data
    //0x312 786 BMS_thermalStatus
    tr_h4(content, TrKey::DRV_POWER_DISSIPATION, String(BMS_powerDissipation), " kW");
    tr_h4(content, TrKey::DRV_FLOW_REQUEST, String(BMS_flowRequest), " LPM");
    tr_h4(content, TrKey::DRV_INLET_ACTIVE_COOL_TARGET_TEMP, String(BMS_inletActiveCoolTargetT), " DegC");
    tr_h4(content, TrKey::DRV_INLET_PASSIVE_TARGET_TEMP, String(BMS_inletPassiveTargetT), " DegC");
    tr_h4(content, TrKey::DRV_INLET_ACTIVE_HEAT_TARGET_TEMP, String(BMS_inletActiveHeatTargetT), " DegC");
    tr_h4(content, TrKey::DRV_PACK_TEMP_MIN, String(BMS_packTMin), " DegC");
    tr_h4(content, TrKey::DRV_PACK_TEMP_MAX, String(BMS_packTMax), " DegC");
    appendFault(content, TR(TrKey::DRV_PCS_NO_FLOW_REQUEST), datalayer_extended.tesla.BMS_pcsNoFlowRequest);
    appendFault(content, TR(TrKey::DRV_BMS_NO_FLOW_REQUEST), datalayer_extended.tesla.BMS_noFlowRequest);
    //0x224 548 PCS_dcdcStatus
    tr_h4(content, TrKey::DRV_PRECHARGE_STATUS,
          String(PCS_dcdcStatus[datalayer_extended.tesla.PCS_dcdcPrechargeStatus]));
    tr_h4(content, TrKey::DRV_12V_SUPPORT_STATUS,
          String(PCS_dcdcStatus[datalayer_extended.tesla.PCS_dcdc12VSupportStatus]));
    tr_h4(content, TrKey::DRV_HV_BUS_DISCHARGE_STATUS,
          String(PCS_dcdcStatus[datalayer_extended.tesla.PCS_dcdcHvBusDischargeStatus]));
    tr_h4(content, TrKey::DRV_MAIN_STATE, String(PCS_dcdcMainState[datalayer_extended.tesla.PCS_dcdcMainState]));
    tr_h4(content, TrKey::DRV_SUB_STATE, String(PCS_dcdcSubState[datalayer_extended.tesla.PCS_dcdcSubState]));
    appendFault(content, TR(TrKey::DRV_PCS_FAULTED), datalayer_extended.tesla.PCS_dcdcFaulted);
    appendFault(content, TR(TrKey::DRV_OUTPUT_LIMITED), datalayer_extended.tesla.PCS_dcdcOutputIsLimited);
    tr_h4(content, TrKey::DRV_MAX_OUTPUT_CURRENT_ALLOWED, String(PCS_dcdcMaxOutputCurrentAllowed), " A");
    tr_h4(content, TrKey::DRV_PRECHARGE_RTY_CNT, String(falseTrue[datalayer_extended.tesla.PCS_dcdcPrechargeRtyCnt]));
    tr_h4(content, TrKey::DRV_12V_SUPPORT_RTY_CNT,
          String(falseTrue[datalayer_extended.tesla.PCS_dcdc12VSupportRtyCnt]));
    tr_h4(content, TrKey::DRV_DISCHARGE_RTY_CNT, String(falseTrue[datalayer_extended.tesla.PCS_dcdcDischargeRtyCnt]));
    appendFault(content, TR(TrKey::DRV_PWM_ENABLE_LINE), datalayer_extended.tesla.PCS_dcdcPwmEnableLine);
    appendFault(content, TR(TrKey::DRV_SUPPORTING_FIXED_LV_TARGET),
                datalayer_extended.tesla.PCS_dcdcSupportingFixedLvTarget);
    tr_h4(content, TrKey::DRV_PRECHARGE_RESTART_CNT,
          String(falseTrue[datalayer_extended.tesla.PCS_dcdcPrechargeRestartCnt]));
    tr_h4(content, TrKey::DRV_INITIAL_PRECHARGE_SUBSTATE,
          String(PCS_dcdcSubState[datalayer_extended.tesla.PCS_dcdcInitialPrechargeSubState]));
    //0x3C4 PCS_info
    tr_h4(content, TrKey::DRV_PCS_BUILDCONFIGID, String(datalayer_extended.tesla.PCS_info_buildConfigId));
    tr_h4(content, TrKey::DRV_PCS_HARDWAREID, String(datalayer_extended.tesla.PCS_info_hardwareId));
    tr_h4(content, TrKey::DRV_PCS_COMPONENTID, String(datalayer_extended.tesla.PCS_info_componentId));
    //0x2C4 708 PCS_logging
    tr_h4(content, TrKey::DRV_PCS_DCDCMAXLVOUTPUTCURRENT, String(PCS_dcdcMaxLvOutputCurrent), " A");
    tr_h4(content, TrKey::DRV_PCS_DCDCCURRENTLIMIT, String(PCS_dcdcCurrentLimit), " A");
    tr_h4(content, TrKey::DRV_PCS_DCDCLVOUTPUTCURRENTTEMPLIMIT, String(PCS_dcdcLvOutputCurrentTempLimit), " A");
    tr_h4(content, TrKey::DRV_PCS_DCDCUNIFIEDCOMMAND, String(PCS_dcdcUnifiedCommand));
    tr_h4(content, TrKey::DRV_PCS_DCDCCLACONTROLLEROUTPUT, String(PCS_dcdcCLAControllerOutput));
    tr_h4(content, TrKey::DRV_PCS_DCDCTANKVOLTAGE, String(PCS_dcdcTankVoltage), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCTANKVOLTAGETARGET, String(PCS_dcdcTankVoltageTarget), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCCLACURRENTFREQ, String(PCS_dcdcClaCurrentFreq), " kHz");
    tr_h4(content, TrKey::DRV_PCS_DCDCTCOMMMEASURED, String(PCS_dcdcTCommMeasured), " us");
    tr_h4(content, TrKey::DRV_PCS_DCDCSHORTTIMEUS, String(PCS_dcdcShortTimeUs), " us");
    tr_h4(content, TrKey::DRV_PCS_DCDCHALFPERIODUS, String(PCS_dcdcHalfPeriodUs), " us");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMAXFREQUENCY, String(PCS_dcdcIntervalMaxFrequency), " kHz");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMAXHVBUSVOLT, String(PCS_dcdcIntervalMaxHvBusVolt), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMAXLVBUSVOLT, String(PCS_dcdcIntervalMaxLvBusVolt), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMAXLVOUTPUTCURR, String(PCS_dcdcIntervalMaxLvOutputCurr), " A");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMINFREQUENCY, String(PCS_dcdcIntervalMinFrequency), " kHz");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMINHVBUSVOLT, String(PCS_dcdcIntervalMinHvBusVolt), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMINLVBUSVOLT, String(PCS_dcdcIntervalMinLvBusVolt), " V");
    tr_h4(content, TrKey::DRV_PCS_DCDCINTERVALMINLVOUTPUTCURR, String(PCS_dcdcIntervalMinLvOutputCurr), " A");
    tr_h4(content, TrKey::DRV_PCS_DCDC12VSUPPORTLIFETIMEKWH, String(PCS_dcdc12vSupportLifetimekWh), " kWh");
    //0x310 HVP_info
    tr_h4(content, TrKey::DRV_HVP_BUILDCONFIGID, String(datalayer_extended.tesla.HVP_info_buildConfigId));
    tr_h4(content, TrKey::DRV_HVP_HARDWAREID, String(datalayer_extended.tesla.HVP_info_hardwareId));
    tr_h4(content, TrKey::DRV_HVP_COMPONENTID, String(datalayer_extended.tesla.HVP_info_componentId));
    //0x7AA 1962 HVP_debugMessage
    tr_h4(content, TrKey::DRV_HVP_BATTERY12V, String(HVP_battery12V), " V");
    tr_h4(content, TrKey::DRV_HVP_DCLINKVOLTAGE, String(HVP_dcLinkVoltage), " V");
    tr_h4(content, TrKey::DRV_HVP_PACKVOLTAGE, String(HVP_packVoltage), " V");
    tr_h4(content, TrKey::DRV_HVP_PACKCONTVOLTAGE, String(HVP_packContVoltage), " V");
    tr_h4(content, TrKey::DRV_HVP_PACKCONTCOILCURRENT, String(HVP_packContCoilCurrent), " A");
    tr_h4(content, TrKey::DRV_HVP_PYROANALOG, String(HVP_pyroAnalog), " V");
    tr_h4(content, TrKey::DRV_HVP_HVP1V5REF, String(HVP_hvp1v5Ref), " V");
    tr_h4(content, TrKey::DRV_HVP_HVILINVOLTAGE, String(HVP_hvilInVoltage), " V");
    tr_h4(content, TrKey::DRV_HVP_HVILOUTVOLTAGE, String(HVP_hvilOutVoltage), " V");
    appendFault(content, "HVP_gpioPassivePyroDepl", datalayer_extended.tesla.HVP_gpioPassivePyroDepl);
    appendFault(content, "HVP_gpioPyroIsoEn", datalayer_extended.tesla.HVP_gpioPyroIsoEn);
    appendFault(content, "HVP_gpioCpFaultIn", datalayer_extended.tesla.HVP_gpioCpFaultIn);
    appendFault(content, "HVP_gpioPackContPowerEn", datalayer_extended.tesla.HVP_gpioPackContPowerEn);
    appendFault(content, "HVP_gpioHvCablesOk", datalayer_extended.tesla.HVP_gpioHvCablesOk);
    appendFault(content, "HVP_gpioHvpSelfEnable", datalayer_extended.tesla.HVP_gpioHvpSelfEnable);
    appendFault(content, "HVP_gpioLed", datalayer_extended.tesla.HVP_gpioLed);
    appendFault(content, "HVP_gpioCrashSignal", datalayer_extended.tesla.HVP_gpioCrashSignal);
    appendFault(content, "HVP_gpioShuntDataReady", datalayer_extended.tesla.HVP_gpioShuntDataReady);
    appendFault(content, "HVP_gpioFcContPosAux", datalayer_extended.tesla.HVP_gpioFcContPosAux);
    appendFault(content, "HVP_gpioFcContNegAux", datalayer_extended.tesla.HVP_gpioFcContNegAux);
    appendFault(content, "HVP_gpioBmsEout", datalayer_extended.tesla.HVP_gpioBmsEout);
    appendFault(content, "HVP_gpioCpFaultOut", datalayer_extended.tesla.HVP_gpioCpFaultOut);
    appendFault(content, "HVP_gpioPyroPor", datalayer_extended.tesla.HVP_gpioPyroPor);
    appendFault(content, "HVP_gpioShuntEn", datalayer_extended.tesla.HVP_gpioShuntEn);
    appendFault(content, "HVP_gpioHvpVerEn", datalayer_extended.tesla.HVP_gpioHvpVerEn);
    appendFault(content, "HVP_gpioPackCoontPosFlywheel", datalayer_extended.tesla.HVP_gpioPackCoontPosFlywheel);
    appendFault(content, "HVP_gpioCpLatchEnable", datalayer_extended.tesla.HVP_gpioCpLatchEnable);
    appendFault(content, "HVP_gpioPcsEnable", datalayer_extended.tesla.HVP_gpioPcsEnable);
    appendFault(content, "HVP_gpioPcsDcdcPwmEnable", datalayer_extended.tesla.HVP_gpioPcsDcdcPwmEnable);
    appendFault(content, "HVP_gpioPcsChargePwmEnable", datalayer_extended.tesla.HVP_gpioPcsChargePwmEnable);
    appendFault(content, "HVP_gpioFcContPowerEnable", datalayer_extended.tesla.HVP_gpioFcContPowerEnable);
    appendFault(content, "HVP_gpioHvilEnable", datalayer_extended.tesla.HVP_gpioHvilEnable);
    appendFault(content, "HVP_gpioSecDrdy", datalayer_extended.tesla.HVP_gpioSecDrdy);
    tr_h4(content, TrKey::DRV_HVP_SHUNTCURRENTDEBUG, String(HVP_shuntCurrentDebug), " A");
    tr_h4(content, TrKey::DRV_HVP_PACKCURRENTMIA, String(noYes[datalayer_extended.tesla.HVP_packCurrentMia]));
    tr_h4(content, TrKey::DRV_HVP_AUXCURRENTMIA, String(noYes[datalayer_extended.tesla.HVP_auxCurrentMia]));
    tr_h4(content, TrKey::DRV_HVP_CURRENTSENSEMIA, String(noYes[datalayer_extended.tesla.HVP_currentSenseMia]));
    tr_h4(content, TrKey::DRV_HVP_SHUNTREFVOLTAGEMISMATCH,
          String(noYes[datalayer_extended.tesla.HVP_shuntRefVoltageMismatch]));
    tr_h4(content, TrKey::DRV_HVP_SHUNTTHERMISTORMIA, String(noYes[datalayer_extended.tesla.HVP_shuntThermistorMia]));
    tr_h4(content, TrKey::DRV_HVP_SHUNTHWMIA, String(noYes[datalayer_extended.tesla.HVP_shuntHwMia]));
    //content += "<h4>HVP_fcLinkVoltage: " + String(HVP_fcLinkVoltage) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_packNegativeV: " + String(HVP_packNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_packPositiveV: " + String(HVP_packPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_dcLinkNegativeV: " + String(HVP_dcLinkNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_dcLinkPositiveV: " + String(HVP_dcLinkPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcLinkNegativeV: " + String(HVP_fcLinkNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcContCoilCurrent: " + String(HVP_fcContCoilCurrent) + " A</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcContVoltage: " + String(HVP_fcContVoltage) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcLinkPositiveV: " + String(HVP_fcLinkPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntRefVoltageDbg: " + String(HVP_shuntRefVoltageDbg) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAuxCurrentDbg: " + String(HVP_shuntAuxCurrentDbg) + " A</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntBarTempDbg: " + String(HVP_shuntBarTempDbg) + " DegC</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAsicTempDbg: " + String(HVP_shuntAsicTempDbg) + " DegC</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAuxCurrentStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntAuxCurrentStatus]) + "</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntBarTempStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntBarTempStatus]) + "</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAsicTempStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntAsicTempStatus]) + "</h4>"; // Not giving useable data

    // ---- Active alert-matrix faults (0x320 BMS / 0x3A4 PCS / 0x31E CP) ----
    // Only ACTIVE faults are listed, to keep the page small. Nothing about the fault names/codes
    // is stored on the ESP32: each row emits only an integer match key (ECU base + the index into
    // the corresponding *_alertMatrixActive[] array). get_dtc_json_loader_html() then loads the
    // DTC JSON from GitHub (or a local copy) and fills the description cell with the readable text
    // (l_dsc) plus the full Tesla token (s_dsc, which carries the real code e.g. BMS_a036). See
    // web_data/dtc/README.md; the JSON "code" field must match "base + index" below.
    {
      struct AlertGroup {
        const char* label;
        int base;  // integer "code" base for this ECU (BMS 1xx, PCS 2xx, CP 3xx)
        const bool* active;
        int count;
      };
      const AlertGroup groups[] = {
          {"BMS 0x320", 100, datalayer_extended.tesla.BMS_alertMatrixActive, 100},
          {"PCS 0x3A4", 200, datalayer_extended.tesla.PCS_alertMatrixActive, 94},
          {"CP 0x31E", 300, datalayer_extended.tesla.CP_alertMatrixActive, 96},
      };

      int total_active = 0;
      for (auto& g : groups) {
        for (int i = 0; i < g.count; i++) {
          if (g.active[i]) {
            total_active++;
          }
        }
      }

      tr_h3(content, TrKey::DRV_ACTIVE_FAULTS, String(total_active));
      // Only render the table (and fetch the DTC JSON) when something is actually active, so a
      // healthy device does no network request on every page load.
      if (total_active > 0) {
        content +=
            "<table style='border-collapse: collapse; margin: 0 auto;'>"
            "<tr><th style='text-align:left;padding:2px 20px 2px 0'>ECU</th>"
            "<th style='text-align:left;padding:2px 0'>" +
            TR(TrKey::DRV_DESCRIPTION) + "</th></tr>";
        for (auto& g : groups) {
          for (int i = 0; i < g.count; i++) {
            if (!g.active[i]) {
              continue;
            }
            String code = String(g.base + i);  // integer match key; JSON maps it to code + description
            content += "<tr><td style='text-align:left;padding:2px 20px 2px 0'>";
            content += g.label;
            // Description cell: the shared loader replaces the placeholder integer with l_dsc + s_dsc
            // (s_dsc carries the real Tesla code, e.g. BMS_a036_SW_HvpHvilFault).
            content += "</td><td style='text-align:left;padding:2px 0' data-dtc-code='";
            content += code;
            content += "'>";
            content += code;
            content += "</td></tr>";
          }
        }
        content += "</table>";
        // Fetch descriptions from GitHub (falls back to a local-file picker when offline).
        content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "tesla_model3y_dtc.json");
      }
    }

    return content;
  }
};

#endif
