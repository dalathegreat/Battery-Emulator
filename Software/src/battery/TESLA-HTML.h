#ifndef _TESLA_HTML_H
#define _TESLA_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class TeslaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    float beginning_of_life = static_cast<float>(datalayer_extended.tesla.battery_beginning_of_life);
    float battTempPct = static_cast<float>(datalayer_extended.tesla.battery_battTempPct) * 0.4;
    float dcdcLvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 0.0390625;
    float dcdcHvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcHvBusVolt) * 0.146484;
    float dcdcLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent) * 0.1;
    float nominal_full_pack_energy =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy) * 0.1;
    float nominal_full_pack_energy_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy_m0) * 0.02;
    float nominal_energy_remaining =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining) * 0.1;
    float nominal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining_m0) * 0.02;
    float ideal_energy_remaining = static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining) * 0.1;
    float ideal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining_m0) * 0.02;
    float energy_to_charge_complete =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete) * 0.1;
    float energy_to_charge_complete_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete_m1) * 0.02;
    float energy_buffer = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer) * 0.1;
    float energy_buffer_m1 = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer_m1) * 0.01;
    float expected_energy_remaining_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_expected_energy_remaining_m1) * 0.02;
    float total_discharge = static_cast<float>(datalayer.battery.status.total_discharged_battery_Wh) * 0.001;
    float total_charge = static_cast<float>(datalayer.battery.status.total_charged_battery_Wh) * 0.001;
    float packMass = static_cast<float>(datalayer_extended.tesla.battery_packMass);
    float platformMaxBusVoltage =
        static_cast<float>(datalayer_extended.tesla.battery_platformMaxBusVoltage) * 0.1 + 375;
    float bms_min_voltage = static_cast<float>(datalayer_extended.tesla.battery_bms_min_voltage) * 0.01 * 2;
    float bms_max_voltage = static_cast<float>(datalayer_extended.tesla.battery_bms_max_voltage) * 0.01 * 2;
    float max_charge_current = static_cast<float>(datalayer_extended.tesla.battery_max_charge_current);
    float max_discharge_current = static_cast<float>(datalayer_extended.tesla.battery_max_discharge_current);
    float soc_ave = static_cast<float>(datalayer_extended.tesla.battery_soc_ave) * 0.1;
    float soc_max = static_cast<float>(datalayer_extended.tesla.battery_soc_max) * 0.1;
    float soc_min = static_cast<float>(datalayer_extended.tesla.battery_soc_min) * 0.1;
    float soc_ui = static_cast<float>(datalayer_extended.tesla.battery_soc_ui) * 0.1;
    float BrickVoltageMax = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMax) * 0.002;
    float BrickVoltageMin = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMin) * 0.002;
    float BrickModelTMax = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMax) * 0.5 - 40;
    float BrickModelTMin = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMin) * 0.5 - 40;
    float isolationResistance = static_cast<float>(datalayer_extended.tesla.battery_BMS_isolationResistance) * 10;
    float PCS_dcdcMaxOutputCurrentAllowed =
        static_cast<float>(datalayer_extended.tesla.battery_PCS_dcdcMaxOutputCurrentAllowed) * 0.1;
    float PCS_dcdcTemp = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTemp) * 0.1 + 40;
    float PCS_ambientTemp = static_cast<float>(datalayer_extended.tesla.PCS_ambientTemp) * 0.1 + 40;
    float PCS_chgPhATemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhATemp) * 0.1 + 40;
    float PCS_chgPhBTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhBTemp) * 0.1 + 40;
    float PCS_chgPhCTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhCTemp) * 0.1 + 40;
    float BMS_maxRegenPower = static_cast<float>(datalayer_extended.tesla.BMS_maxRegenPower) * 0.01;
    float BMS_maxDischargePower = static_cast<float>(datalayer_extended.tesla.BMS_maxDischargePower) * 0.013;
    float BMS_maxStationaryHeatPower = static_cast<float>(datalayer_extended.tesla.BMS_maxStationaryHeatPower) * 0.01;
    float BMS_hvacPowerBudget = static_cast<float>(datalayer_extended.tesla.BMS_hvacPowerBudget) * 0.02;
    float BMS_powerDissipation = static_cast<float>(datalayer_extended.tesla.BMS_powerDissipation) * 0.02;
    float BMS_flowRequest = static_cast<float>(datalayer_extended.tesla.BMS_flowRequest) * 0.3;
    float BMS_inletActiveCoolTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveCoolTargetT) * 0.25 - 25;
    float BMS_inletPassiveTargetT = static_cast<float>(datalayer_extended.tesla.BMS_inletPassiveTargetT) * 0.25 - 25;
    float BMS_inletActiveHeatTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveHeatTargetT) * 0.25 - 25;
    float BMS_packTMin = static_cast<float>(datalayer_extended.tesla.BMS_packTMin) * 0.25 - 25;
    float BMS_packTMax = static_cast<float>(datalayer_extended.tesla.BMS_packTMax) * 0.25 - 25;
    float PCS_dcdcMaxLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.PCS_dcdcMaxLvOutputCurrent) * 0.1;
    float PCS_dcdcCurrentLimit = static_cast<float>(datalayer_extended.tesla.PCS_dcdcCurrentLimit) * 0.1;
    float PCS_dcdcLvOutputCurrentTempLimit =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcLvOutputCurrentTempLimit) * 0.1;
    float PCS_dcdcUnifiedCommand = static_cast<float>(datalayer_extended.tesla.PCS_dcdcUnifiedCommand) * 0.001;
    float PCS_dcdcCLAControllerOutput =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcCLAControllerOutput * 0.001);
    float PCS_dcdcTankVoltage = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltage);
    float PCS_dcdcTankVoltageTarget = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltageTarget);
    float PCS_dcdcClaCurrentFreq = static_cast<float>(datalayer_extended.tesla.PCS_dcdcClaCurrentFreq) * 0.0976563;
    float PCS_dcdcTCommMeasured = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTCommMeasured) * 0.00195313;
    float PCS_dcdcShortTimeUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcShortTimeUs) * 0.000488281;
    float PCS_dcdcHalfPeriodUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcHalfPeriodUs) * 0.000488281;
    float PCS_dcdcIntervalMaxFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxFrequency);
    float PCS_dcdcIntervalMaxHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxHvBusVolt) * 0.1;
    float PCS_dcdcIntervalMaxLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvBusVolt) * 0.1;
    float PCS_dcdcIntervalMaxLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvOutputCurr);
    float PCS_dcdcIntervalMinFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinFrequency);
    float PCS_dcdcIntervalMinHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinHvBusVolt) * 0.1;
    float PCS_dcdcIntervalMinLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvBusVolt) * 0.1;
    float PCS_dcdcIntervalMinLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvOutputCurr);
    float PCS_dcdc12vSupportLifetimekWh =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdc12vSupportLifetimekWh) * 0.01;
    float HVP_hvp1v5Ref = static_cast<float>(datalayer_extended.tesla.HVP_hvp1v5Ref) * 0.1;
    float HVP_shuntCurrentDebug = static_cast<float>(datalayer_extended.tesla.HVP_shuntCurrentDebug) * 0.1;
    float HVP_dcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkVoltage) * 0.1;
    float HVP_packVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packVoltage) * 0.1;
    float HVP_fcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkVoltage) * 0.1;
    float HVP_packContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packContVoltage) * 0.1;
    float HVP_packNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_packNegativeV) * 0.1;
    float HVP_packPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_packPositiveV) * 0.1;
    float HVP_pyroAnalog = static_cast<float>(datalayer_extended.tesla.HVP_pyroAnalog) * 0.1;
    float HVP_dcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkNegativeV) * 0.1;
    float HVP_dcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkPositiveV) * 0.1;
    float HVP_fcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkNegativeV) * 0.1;
    float HVP_fcContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_fcContCoilCurrent) * 0.1;
    float HVP_fcContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcContVoltage) * 0.1;
    float HVP_hvilInVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilInVoltage) * 0.1;
    float HVP_hvilOutVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilOutVoltage) * 0.1;
    float HVP_fcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkPositiveV) * 0.1;
    float HVP_packContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_packContCoilCurrent) * 0.1;
    float HVP_battery12V = static_cast<float>(datalayer_extended.tesla.HVP_battery12V) * 0.1;
    float HVP_shuntRefVoltageDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntRefVoltageDbg) * 0.001;
    float HVP_shuntAuxCurrentDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAuxCurrentDbg) * 0.1;
    float HVP_shuntBarTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntBarTempDbg) * 0.01;
    float HVP_shuntAsicTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAsicTempDbg) * 0.01;

    static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                          "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                          "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
    static const char* hvilStatusState[] = {"NOT Ok",
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
    static const char* PCS_dcdcStatus[] = {"IDLE", "ACTIVE", "FAULTED"};
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
    static const char* HVP_status[] = {"INVALID", "NOT_AVAILABLE", "STALE", "VALID"};
    static const char* HVP_contactor[] = {"NOT_ACTIVE", "ACTIVE", "COMPLETED"};
    static const char* falseTrue[] = {"False", "True"};
    static const char* noYes[] = {"No", "Yes"};
    static const char* Fault[] = {"NOT_ACTIVE", "ACTIVE"};

    //0x20A 522 HVP_contatorState
    content += "<h4>Contactor Status: " + String(contactorText[datalayer_extended.tesla.status_contactor]) + "</h4>";
    content += "<h4>HVIL: " + String(hvilStatusState[datalayer_extended.tesla.hvil_status]) + "</h4>";
    content +=
        "<h4>Negative contactor: " + String(contactorState[datalayer_extended.tesla.packContNegativeState]) + "</h4>";
    content +=
        "<h4>Positive contactor: " + String(contactorState[datalayer_extended.tesla.packContPositiveState]) + "</h4>";
    content += "<h4>Closing allowed?: " + String(noYes[datalayer_extended.tesla.packCtrsClosingAllowed]) + "</h4>";
    content += "<h4>Pyrotest in Progress: " + String(noYes[datalayer_extended.tesla.pyroTestInProgress]) + "</h4>";
    content += "<h4>Contactors Open Now Requested: " +
               String(noYes[datalayer_extended.tesla.battery_packCtrsOpenNowRequested]) + "</h4>";
    content +=
        "<h4>Contactors Open Requested: " + String(noYes[datalayer_extended.tesla.battery_packCtrsOpenRequested]) +
        "</h4>";
    content += "<h4>Contactors Request Status: " +
               String(HVP_contactor[datalayer_extended.tesla.battery_packCtrsRequestStatus]) + "</h4>";
    content += "<h4>Contactors Reset Request Required: " +
               String(noYes[datalayer_extended.tesla.battery_packCtrsResetRequestRequired]) + "</h4>";
    content +=
        "<h4>DC Link Allowed to Energize: " + String(noYes[datalayer_extended.tesla.battery_dcLinkAllowedToEnergize]) +
        "</h4>";
    char readableSerialNumber[15];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.tesla.BMS_SerialNumber,
           sizeof(datalayer_extended.tesla.BMS_SerialNumber));
    readableSerialNumber[14] = '\0';  // Null terminate the string
    content += "<h4>BMS Serial number: " + String(readableSerialNumber) + "</h4>";
    // Comment what data you would like to display, order can be changed.
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == false) {
      content += "<h3>BMS 0x352 w/o mux</h3>";  //if using older BMS <2021 and comment 0x352 without MUX
      content += "<h4>Calculated SOH: " + String(nominal_full_pack_energy * 100 / beginning_of_life) + "</h4>";
      content += "<h4>Nominal Full Pack Energy: " + String(nominal_full_pack_energy) + " KWh</h4>";
      content += "<h4>Nominal Energy Remaining: " + String(nominal_energy_remaining) + " KWh</h4>";
      content += "<h4>Ideal Energy Remaining: " + String(ideal_energy_remaining) + " KWh</h4>";
      content += "<h4>Energy to Charge Complete: " + String(energy_to_charge_complete) + " KWh</h4>";
      content += "<h4>Energy Buffer: " + String(energy_buffer) + " KWh</h4>";
      content += "<h4>Full Charge Complete: " + String(noYes[datalayer_extended.tesla.battery_full_charge_complete]) +
                 "</h4>";  //bool
    }
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == true) {
      content += "<h3>BMS 0x352 w/ mux</h3>";  //if using newer BMS >2021 and comment 0x352 with MUX
      content += "<h4>Calculated SOH: " + String(nominal_full_pack_energy_m0 * 100 / beginning_of_life) + "</h4>";
      content += "<h4>Nominal Full Pack Energy: " + String(nominal_full_pack_energy_m0) + " KWh</h4>";
      content += "<h4>Nominal Energy Remaining: " + String(nominal_energy_remaining_m0) + " KWh</h4>";
      content += "<h4>Ideal Energy Remaining: " + String(ideal_energy_remaining_m0) + " KWh</h4>";
      content += "<h4>Energy to Charge Complete: " + String(energy_to_charge_complete_m1) + " KWh</h4>";
      content += "<h4>Energy Buffer: " + String(energy_buffer_m1) + " KWh</h4>";
      content += "<h4>Expected Energy Remaining: " + String(expected_energy_remaining_m1) + " KWh</h4>";
      content += "<h4>Fully Charged: " + String(noYes[datalayer_extended.tesla.battery_fully_charged]) + "</h4>";
    }
    //0x3D2 978 BMS_kwhCounter
    content += "<h4>Total Discharge: " + String(total_discharge) + " KWh</h4>";
    content += "<h4>Total Charge: " + String(total_charge) + " KWh</h4>";
    //0x292 658 BMS_socStates
    content += "<h4>Battery Beginning of Life: " + String(beginning_of_life) + " KWh</h4>";
    content += "<h4>Battery SOC UI: " + String(soc_ui) + " </h4>";
    content += "<h4>Battery SOC Ave: " + String(soc_ave) + " </h4>";
    content += "<h4>Battery SOC Max: " + String(soc_max) + " </h4>";
    content += "<h4>Battery SOC Min: " + String(soc_min) + " </h4>";
    content += "<h4>Battery Temp Percent: " + String(battTempPct) + " </h4>";
    //0x2B4 PCS_dcdcRailStatus
    content += "<h4>PCS Lv Output: " + String(dcdcLvOutputCurrent) + " A</h4>";
    content += "<h4>PCS Lv Bus: " + String(dcdcLvBusVolt) + " V</h4>";
    content += "<h4>PCS Hv Bus: " + String(dcdcHvBusVolt) + " V</h4>";
    //0x392 BMS_packConfig
    //content += "<h4>packConfigMultiplexer: " + String(datalayer_extended.tesla.battery_packConfigMultiplexer) + "</h4>"; // Not giving useable data
    //content += "<h4>moduleType: " + String(datalayer_extended.tesla.battery_moduleType) + "</h4>";  // Not giving useable data
    //content += "<h4>reserveConfig: " + String(datalayer_extended.tesla.battery_reservedConfig) + "</h4>";  // Not giving useable data
    content += "<h4>Battery Pack Mass: " + String(packMass) + " KG</h4>";
    content += "<h4>Platform Max Bus Voltage: " + String(platformMaxBusVoltage) + " V</h4>";
    //0x2D2 722 BMSVAlimits
    content += "<h4>BMS Min Voltage: " + String(bms_min_voltage) + " V</h4>";
    content += "<h4>BMS Max Voltage: " + String(bms_max_voltage) + " V</h4>";
    content += "<h4>Max Charge Current: " + String(max_charge_current) + " A</h4>";
    content += "<h4>Max Discharge Current: " + String(max_discharge_current) + " A</h4>";
    //0x332 818 BMS_bmbMinMax
    content += "<h4>Brick Voltage Max: " + String(BrickVoltageMax) + " V</h4>";
    content += "<h4>Brick Voltage Min: " + String(BrickVoltageMin) + " V</h4>";
    content += "<h4>Brick Temp Max Num: " + String(datalayer_extended.tesla.battery_BrickTempMaxNum) + " </h4>";
    content += "<h4>Brick Temp Min Num: " + String(datalayer_extended.tesla.battery_BrickTempMinNum) + " </h4>";
    //content += "<h4>Brick Model Temp Max: " + String(BrickModelTMax) + " C</h4>";// Not giving useable data
    //content += "<h4>Brick Model Temp Min: " + String(BrickModelTMin) + " C</h4>";// Not giving useable data
    //0x2A4 676 PCS_thermalStatus
    content += "<h4>PCS dcdc Temp: " + String(PCS_dcdcTemp) + " DegC</h4>";
    content += "<h4>PCS Ambient Temp: " + String(PCS_ambientTemp) + " DegC</h4>";
    content += "<h4>PCS Chg PhA Temp: " + String(PCS_chgPhATemp) + " DegC</h4>";
    content += "<h4>PCS Chg PhB Temp: " + String(PCS_chgPhBTemp) + " DegC</h4>";
    content += "<h4>PCS Chg PhC Temp: " + String(PCS_chgPhCTemp) + " DegC</h4>";
    //0x252 594 BMS_powerAvailable
    content += "<h4>Max Regen Power: " + String(BMS_maxRegenPower) + " KW</h4>";
    content += "<h4>Max Discharge Power: " + String(BMS_maxDischargePower) + " KW</h4>";
    //content += "<h4>Max Stationary Heat Power: " + String(BMS_maxStationaryHeatPower) + " KWh</h4>"; // Not giving useable data
    //content += "<h4>HVAC Power Budget: " + String(BMS_hvacPowerBudget) + " KW</h4>"; // Not giving useable data
    //content += "<h4>Not Enough Power For Heat Pump: " + String(noYes[datalayer_extended.tesla.BMS_notEnoughPowerForHeatPump]) + "</h4>"; // Not giving useable data
    content +=
        "<h4>Power Limit State: " + String(BMS_powerLimitState[datalayer_extended.tesla.BMS_powerLimitState]) + "</h4>";
    //content += "<h4>Inverter TQF: " + String(datalayer_extended.tesla.BMS_inverterTQF) + "</h4>"; // Not giving useable data
    //0x212 530 BMS_status
    content += "<h4>Isolation Resistance: " + String(isolationResistance) + " kOhms</h4>";
    content +=
        "<h4>BMS Contactor State: " + String(BMS_contactorState[datalayer_extended.tesla.battery_BMS_contactorState]) +
        "</h4>";
    content += "<h4>BMS State: " + String(BMS_state[datalayer_extended.tesla.battery_BMS_state]) + "</h4>";
    content += "<h4>BMS HV State: " + String(BMS_hvState[datalayer_extended.tesla.battery_BMS_hvState]) + "</h4>";
    content += "<h4>BMS UI Charge Status: " + String(BMS_uiChargeStatus[datalayer_extended.tesla.battery_BMS_hvState]) +
               "</h4>";
    content +=
        "<h4>BMS PCS PWM Enabled: " + String(Fault[datalayer_extended.tesla.battery_BMS_pcsPwmEnabled]) + "</h4>";
    //0x312 786 BMS_thermalStatus
    content += "<h4>Power Dissipation: " + String(BMS_powerDissipation) + " kW</h4>";
    content += "<h4>Flow Request: " + String(BMS_flowRequest) + " LPM</h4>";
    content += "<h4>Inlet Active Cool Target Temp: " + String(BMS_inletActiveCoolTargetT) + " DegC</h4>";
    content += "<h4>Inlet Passive Target Temp: " + String(BMS_inletPassiveTargetT) + " DegC</h4>";
    content += "<h4>Inlet Active Heat Target Temp: " + String(BMS_inletActiveHeatTargetT) + " DegC</h4>";
    content += "<h4>Pack Temp Min: " + String(BMS_packTMin) + " DegC</h4>";
    content += "<h4>Pack Temp Max: " + String(BMS_packTMax) + " DegC</h4>";
    content += "<h4>PCS No Flow Request: " + String(Fault[datalayer_extended.tesla.BMS_pcsNoFlowRequest]) + "</h4>";
    content += "<h4>BMS No Flow Request: " + String(Fault[datalayer_extended.tesla.BMS_noFlowRequest]) + "</h4>";
    //0x224 548 PCS_dcdcStatus
    content +=
        "<h4>Precharge Status: " + String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdcPrechargeStatus]) +
        "</h4>";
    content +=
        "<h4>12V Support Status: " + String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdc12VSupportStatus]) +
        "</h4>";
    content += "<h4>HV Bus Discharge Status: " +
               String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdcHvBusDischargeStatus]) + "</h4>";
    content +=
        "<h4>Main State: " + String(PCS_dcdcMainState[datalayer_extended.tesla.battery_PCS_dcdcMainState]) + "</h4>";
    content +=
        "<h4>Sub State: " + String(PCS_dcdcSubState[datalayer_extended.tesla.battery_PCS_dcdcSubState]) + "</h4>";
    content += "<h4>PCS Faulted: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcFaulted]) + "</h4>";
    content +=
        "<h4>Output Is Limited: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcOutputIsLimited]) + "</h4>";
    content += "<h4>Max Output Current Allowed: " + String(PCS_dcdcMaxOutputCurrentAllowed) + " A</h4>";
    content += "<h4>Precharge Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcPrechargeRtyCnt]) +
               "</h4>";
    content +=
        "<h4>12V Support Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdc12VSupportRtyCnt]) +
        "</h4>";
    content += "<h4>Discharge Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcDischargeRtyCnt]) +
               "</h4>";
    content +=
        "<h4>PWM Enable Line: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcPwmEnableLine]) + "</h4>";
    content += "<h4>Supporting Fixed LV Target: " +
               String(Fault[datalayer_extended.tesla.battery_PCS_dcdcSupportingFixedLvTarget]) + "</h4>";
    content += "<h4>Precharge Restart Cnt: " +
               String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcPrechargeRestartCnt]) + "</h4>";
    content += "<h4>Initial Precharge Substate: " +
               String(PCS_dcdcSubState[datalayer_extended.tesla.battery_PCS_dcdcInitialPrechargeSubState]) + "</h4>";
    //0x2C4 708 PCS_logging
    content += "<h4>PCS_dcdcMaxLvOutputCurrent: " + String(PCS_dcdcMaxLvOutputCurrent) + " A</h4>";
    content += "<h4>PCS_dcdcCurrentLimit: " + String(PCS_dcdcCurrentLimit) + " A</h4>";
    content += "<h4>PCS_dcdcLvOutputCurrentTempLimit: " + String(PCS_dcdcLvOutputCurrentTempLimit) + " A</h4>";
    content += "<h4>PCS_dcdcUnifiedCommand: " + String(PCS_dcdcUnifiedCommand) + "</h4>";
    content += "<h4>PCS_dcdcCLAControllerOutput: " + String(PCS_dcdcCLAControllerOutput) + "</h4>";
    content += "<h4>PCS_dcdcTankVoltage: " + String(PCS_dcdcTankVoltage) + " V</h4>";
    content += "<h4>PCS_dcdcTankVoltageTarget: " + String(PCS_dcdcTankVoltageTarget) + " V</h4>";
    content += "<h4>PCS_dcdcClaCurrentFreq: " + String(PCS_dcdcClaCurrentFreq) + " kHz</h4>";
    content += "<h4>PCS_dcdcTCommMeasured: " + String(PCS_dcdcTCommMeasured) + " us</h4>";
    content += "<h4>PCS_dcdcShortTimeUs: " + String(PCS_dcdcShortTimeUs) + " us</h4>";
    content += "<h4>PCS_dcdcHalfPeriodUs: " + String(PCS_dcdcHalfPeriodUs) + " us</h4>";
    content += "<h4>PCS_dcdcIntervalMaxFrequency: " + String(PCS_dcdcIntervalMaxFrequency) + " kHz</h4>";
    content += "<h4>PCS_dcdcIntervalMaxHvBusVolt: " + String(PCS_dcdcIntervalMaxHvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMaxLvBusVolt: " + String(PCS_dcdcIntervalMaxLvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMaxLvOutputCurr: " + String(PCS_dcdcIntervalMaxLvOutputCurr) + " A</h4>";
    content += "<h4>PCS_dcdcIntervalMinFrequency: " + String(PCS_dcdcIntervalMinFrequency) + " kHz</h4>";
    content += "<h4>PCS_dcdcIntervalMinHvBusVolt: " + String(PCS_dcdcIntervalMinHvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMinLvBusVolt: " + String(PCS_dcdcIntervalMinLvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMinLvOutputCurr: " + String(PCS_dcdcIntervalMinLvOutputCurr) + " A</h4>";
    content += "<h4>PCS_dcdc12vSupportLifetimekWh: " + String(PCS_dcdc12vSupportLifetimekWh) + " kWh</h4>";
    //0x7AA 1962 HVP_debugMessage
    content += "<h4>HVP_battery12V: " + String(HVP_battery12V) + " V</h4>";
    content += "<h4>HVP_dcLinkVoltage: " + String(HVP_dcLinkVoltage) + " V</h4>";
    content += "<h4>HVP_packVoltage: " + String(HVP_packVoltage) + " V</h4>";
    content += "<h4>HVP_packContVoltage: " + String(HVP_packContVoltage) + " V</h4>";
    content += "<h4>HVP_packContCoilCurrent: " + String(HVP_packContCoilCurrent) + " A</h4>";
    content += "<h4>HVP_pyroAnalog: " + String(HVP_pyroAnalog) + " V</h4>";
    content += "<h4>HVP_hvp1v5Ref: " + String(HVP_hvp1v5Ref) + " V</h4>";
    content += "<h4>HVP_hvilInVoltage: " + String(HVP_hvilInVoltage) + " V</h4>";
    content += "<h4>HVP_hvilOutVoltage: " + String(HVP_hvilOutVoltage) + " V</h4>";
    content +=
        "<h4>HVP_gpioPassivePyroDepl: " + String(Fault[datalayer_extended.tesla.HVP_gpioPassivePyroDepl]) + "</h4>";
    content += "<h4>HVP_gpioPyroIsoEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioPyroIsoEn]) + "</h4>";
    content += "<h4>HVP_gpioCpFaultIn: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpFaultIn]) + "</h4>";
    content +=
        "<h4>HVP_gpioPackContPowerEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioPackContPowerEn]) + "</h4>";
    content += "<h4>HVP_gpioHvCablesOk: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvCablesOk]) + "</h4>";
    content += "<h4>HVP_gpioHvpSelfEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvpSelfEnable]) + "</h4>";
    content += "<h4>HVP_gpioLed: " + String(Fault[datalayer_extended.tesla.HVP_gpioLed]) + "</h4>";
    content += "<h4>HVP_gpioCrashSignal: " + String(Fault[datalayer_extended.tesla.HVP_gpioCrashSignal]) + "</h4>";
    content +=
        "<h4>HVP_gpioShuntDataReady: " + String(Fault[datalayer_extended.tesla.HVP_gpioShuntDataReady]) + "</h4>";
    content += "<h4>HVP_gpioFcContPosAux: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContPosAux]) + "</h4>";
    content += "<h4>HVP_gpioFcContNegAux: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContNegAux]) + "</h4>";
    content += "<h4>HVP_gpioBmsEout: " + String(Fault[datalayer_extended.tesla.HVP_gpioBmsEout]) + "</h4>";
    content += "<h4>HVP_gpioCpFaultOut: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpFaultOut]) + "</h4>";
    content += "<h4>HVP_gpioPyroPor: " + String(Fault[datalayer_extended.tesla.HVP_gpioPyroPor]) + "</h4>";
    content += "<h4>HVP_gpioShuntEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioShuntEn]) + "</h4>";
    content += "<h4>HVP_gpioHvpVerEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvpVerEn]) + "</h4>";
    content +=
        "<h4>HVP_gpioPackCoontPosFlywheel: " + String(Fault[datalayer_extended.tesla.HVP_gpioPackCoontPosFlywheel]) +
        "</h4>";
    content += "<h4>HVP_gpioCpLatchEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpLatchEnable]) + "</h4>";
    content += "<h4>HVP_gpioPcsEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsEnable]) + "</h4>";
    content +=
        "<h4>HVP_gpioPcsDcdcPwmEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsDcdcPwmEnable]) + "</h4>";
    content += "<h4>HVP_gpioPcsChargePwmEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsChargePwmEnable]) +
               "</h4>";
    content +=
        "<h4>HVP_gpioFcContPowerEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContPowerEnable]) + "</h4>";
    content += "<h4>HVP_gpioHvilEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvilEnable]) + "</h4>";
    content += "<h4>HVP_gpioSecDrdy: " + String(Fault[datalayer_extended.tesla.HVP_gpioSecDrdy]) + "</h4>";
    content += "<h4>HVP_shuntCurrentDebug: " + String(HVP_shuntCurrentDebug) + " A</h4>";
    content += "<h4>HVP_packCurrentMia: " + String(noYes[datalayer_extended.tesla.HVP_packCurrentMia]) + "</h4>";
    content += "<h4>HVP_auxCurrentMia: " + String(noYes[datalayer_extended.tesla.HVP_auxCurrentMia]) + "</h4>";
    content += "<h4>HVP_currentSenseMia: " + String(noYes[datalayer_extended.tesla.HVP_currentSenseMia]) + "</h4>";
    content +=
        "<h4>HVP_shuntRefVoltageMismatch: " + String(noYes[datalayer_extended.tesla.HVP_shuntRefVoltageMismatch]) +
        "</h4>";
    content +=
        "<h4>HVP_shuntThermistorMia: " + String(noYes[datalayer_extended.tesla.HVP_shuntThermistorMia]) + "</h4>";
    content += "<h4>HVP_shuntHwMia: " + String(noYes[datalayer_extended.tesla.HVP_shuntHwMia]) + "</h4>";
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

    return content;
  }
};

#endif
