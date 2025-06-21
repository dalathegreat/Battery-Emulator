#ifndef TESLA_BATTERY_H
#define TESLA_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../include.h"
#include "CanBattery.h"
#include "TESLA-HTML.h"

#ifdef TESLA_MODEL_3Y_BATTERY
#define SELECTED_BATTERY_CLASS TeslaModel3YBattery
#endif
#ifdef TESLA_MODEL_SX_BATTERY
#define SELECTED_BATTERY_CLASS TeslaModelSXBattery
#endif

//#define EXP_TESLA_BMS_DIGITAL_HVIL    // Experimental parameter. Forces the transmission of additional CAN frames for experimental purposes, to test potential HVIL issues in 3/Y packs with newer firmware.

class TeslaBattery : public CanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  TeslaBattery() { allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing; }

  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_clear_isolation() { return true; }
  void clear_isolation() { datalayer.battery.settings.user_requests_tesla_isolation_clear = true; }

  bool supports_reset_BMS() { return true; }
  void reset_BMS() { datalayer.battery.settings.user_requests_tesla_bms_reset = true; }

  bool supports_charged_energy() { return true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  TeslaHtmlRenderer renderer;

 protected:
  /* Modify these if needed */
  static const int MAXCHARGEPOWERALLOWED =
      15000;  // 15000W we use a define since the value supplied by Tesla is always 0
  static const int MAXDISCHARGEPOWERALLOWED =
      60000;  // 60000W we use a define since the value supplied by Tesla is always 0

  /* Do not change the defines below */
  static const int RAMPDOWN_SOC = 900;  // 90.0 SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int RAMPDOWNPOWERALLOWED = 10000;      // What power we ramp down from towards top balancing
  static const int FLOAT_MAX_POWER_W = 200;           // W, what power to allow for top balancing battery
  static const int FLOAT_START_MV = 20;               // mV, how many mV under overvoltage to start float charging
  static const int MAX_PACK_VOLTAGE_SX_NCMA = 4600;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_SX_NCMA = 3100;   // V+1, if pack voltage goes over this, charge stops
  static const int MAX_PACK_VOLTAGE_3Y_NCMA = 4030;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_NCMA = 3100;   // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_PACK_VOLTAGE_3Y_LFP = 3880;    // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_LFP = 2968;    // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_CELL_DEVIATION_NCA_NCM = 500;  //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_DEVIATION_LFP = 400;      //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_VOLTAGE_NCA_NCM =
      4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_NCA_NCM =
      2950;                                      //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_VOLTAGE_LFP = 3650;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_LFP = 2800;  //Battery is put into emergency stop if one cell goes below this value

  bool operate_contactors = false;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  void printFaultCodesIfActive();

  unsigned long previousMillis10 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis500 = 0;   // will store last time a 500ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  bool alternate243 = false;
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
  //0x241 VCFRONT_coolant 100ms
  CAN_frame TESLA_241 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 7,
                         .ID = 0x241,
                         .data = {0x3C, 0x78, 0x2C, 0x0F, 0x1E, 0x5B, 0x00}};
  //0x242 VCLEFT_LVPowerState 100ms
  CAN_frame TESLA_242 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x242, .data = {0x10, 0x95}};
  //0x243 VCRIGHT_hvacStatus 50ms
  CAN_frame TESLA_243_1 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x243,
                           .data = {0xC9, 0x00, 0xEB, 0xD4, 0x31, 0x32, 0x02, 0x00}};
  CAN_frame TESLA_243_2 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x243,
                           .data = {0x08, 0x81, 0x42, 0x60, 0x92, 0x2C, 0x0E, 0x09}};
  //0x129 SteeringAngle 10ms
  CAN_frame TESLA_129 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x129,
                         .data = {0x21, 0x24, 0x36, 0x5F, 0x00, 0x20, 0xFF, 0x3F}};
  //0x612 UDS diagnostic requests - on demand
  CAN_frame TESLA_602 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x602,
                         .data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint8_t stateMachineClearIsolationFault = 0xFF;
  uint8_t stateMachineBMSReset = 0xFF;
  uint16_t sendContactorClosingMessagesStill = 300;
  uint16_t battery_cell_max_v = 3300;
  uint16_t battery_cell_min_v = 3300;
  uint16_t battery_cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
  bool cellvoltagesRead = false;
  //0x3d2: 978 BMS_kwhCounter
  uint32_t battery_total_discharge = 0;
  uint32_t battery_total_charge = 0;
  //0x352: 850 BMS_energyStatus
  bool BMS352_mux = false;                            // variable to store when 0x352 mux is present
  uint16_t battery_energy_buffer = 0;                 // kWh
  uint16_t battery_energy_buffer_m1 = 0;              // kWh
  uint16_t battery_energy_to_charge_complete = 0;     // kWh
  uint16_t battery_energy_to_charge_complete_m1 = 0;  // kWh
  uint16_t battery_expected_energy_remaining = 0;     // kWh
  uint16_t battery_expected_energy_remaining_m1 = 0;  // kWh
  bool battery_full_charge_complete = false;          // Changed to bool
  bool battery_fully_charged = false;                 // Changed to bool
  uint16_t battery_ideal_energy_remaining = 0;        // kWh
  uint16_t battery_ideal_energy_remaining_m0 = 0;     // kWh
  uint16_t battery_nominal_energy_remaining = 0;      // kWh
  uint16_t battery_nominal_energy_remaining_m0 = 0;   // kWh
  uint16_t battery_nominal_full_pack_energy = 0;      // Kwh
  uint16_t battery_nominal_full_pack_energy_m0 = 0;   // Kwh
  //0x132 306 HVBattAmpVolt
  uint16_t battery_volts = 0;                  // V
  int16_t battery_amps = 0;                    // A
  int16_t battery_raw_amps = 0;                // A
  uint16_t battery_charge_time_remaining = 0;  // Minutes
  //0x252 594 BMS_powerAvailable
  uint16_t BMS_maxRegenPower = 0;           //rename from battery_regenerative_limit
  uint16_t BMS_maxDischargePower = 0;       // rename from battery_discharge_limit
  uint16_t BMS_maxStationaryHeatPower = 0;  //rename from battery_max_heat_park
  uint16_t BMS_hvacPowerBudget = 0;         //rename from battery_hvac_max_power
  uint8_t BMS_notEnoughPowerForHeatPump = 0;
  uint8_t BMS_powerLimitState = 0;
  uint8_t BMS_inverterTQF = 0;
  //0x2d2: 722 BMSVAlimits
  uint16_t battery_max_discharge_current = 0;
  uint16_t battery_max_charge_current = 0;
  uint16_t battery_bms_max_voltage = 0;
  uint16_t battery_bms_min_voltage = 0;
  //0x2b4: 692 PCS_dcdcRailStatus
  uint16_t battery_dcdcHvBusVolt = 0;        // Change name from battery_high_voltage to battery_dcdcHvBusVolt
  uint16_t battery_dcdcLvBusVolt = 0;        // Change name from battery_low_voltage to battery_dcdcLvBusVolt
  uint16_t battery_dcdcLvOutputCurrent = 0;  // Change name from battery_output_current to battery_dcdcLvOutputCurrent
  //0x292: 658 BMS_socStatus
  uint16_t battery_beginning_of_life = 0;  // kWh
  uint16_t battery_soc_min = 0;
  uint16_t battery_soc_max = 0;
  uint16_t battery_soc_ui = 0;  //Change name from battery_soc_vi to reflect DBC battery_soc_ui
  uint16_t battery_soc_ave = 0;
  uint8_t battery_battTempPct = 0;
  //0x392: BMS_packConfig
  uint32_t battery_packMass = 0;
  uint32_t battery_platformMaxBusVoltage = 0;
  uint32_t battery_packConfigMultiplexer = 0;
  uint32_t battery_moduleType = 0;
  uint32_t battery_reservedConfig = 0;
  //0x332: 818 BattBrickMinMax:BMS_bmbMinMax
  int16_t battery_max_temp = 0;  // C*
  int16_t battery_min_temp = 0;  // C*
  uint16_t battery_BrickVoltageMax = 0;
  uint16_t battery_BrickVoltageMin = 0;
  uint8_t battery_BrickTempMaxNum = 0;
  uint8_t battery_BrickTempMinNum = 0;
  uint8_t battery_BrickModelTMax = 0;
  uint8_t battery_BrickModelTMin = 0;
  uint8_t battery_BrickVoltageMaxNum = 0;  //rename from battery_max_vno
  uint8_t battery_BrickVoltageMinNum = 0;  //rename from battery_min_vno
  //0x20A: 522 HVP_contactorState
  uint8_t battery_contactor = 0;  //State of contactor
  uint8_t battery_hvil_status = 0;
  uint8_t battery_packContNegativeState = 0;
  uint8_t battery_packContPositiveState = 0;
  uint8_t battery_packContactorSetState = 0;
  bool battery_packCtrsClosingAllowed = false;    // Change to bool
  bool battery_pyroTestInProgress = false;        // Change to bool
  bool battery_packCtrsOpenNowRequested = false;  // Change to bool
  bool battery_packCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_packCtrsRequestStatus = 0;
  bool battery_packCtrsResetRequestRequired = false;  // Change to bool
  bool battery_dcLinkAllowedToEnergize = false;       // Change to bool
  bool battery_fcContNegativeAuxOpen = false;         // Change to bool
  uint8_t battery_fcContNegativeState = 0;
  bool battery_fcContPositiveAuxOpen = false;  // Change to bool
  uint8_t battery_fcContPositiveState = 0;
  uint8_t battery_fcContactorSetState = 0;
  bool battery_fcCtrsClosingAllowed = false;    // Change to bool
  bool battery_fcCtrsOpenNowRequested = false;  // Change to bool
  bool battery_fcCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_fcCtrsRequestStatus = 0;
  bool battery_fcCtrsResetRequestRequired = false;  // Change to bool
  bool battery_fcLinkAllowedToEnergize = false;     // Change to bool
  //0x72A: BMS_serialNumber
  uint8_t BMS_SerialNumber[14] = {0};  // Stores raw HEX values for ASCII chars
  //0x212: 530 BMS_status
  bool battery_BMS_hvacPowerRequest = false;          //Change to bool
  bool battery_BMS_notEnoughPowerForDrive = false;    //Change to bool
  bool battery_BMS_notEnoughPowerForSupport = false;  //Change to bool
  bool battery_BMS_preconditionAllowed = false;       //Change to bool
  bool battery_BMS_updateAllowed = false;             //Change to bool
  bool battery_BMS_activeHeatingWorthwhile = false;   //Change to bool
  bool battery_BMS_cpMiaOnHvs = false;                //Change to bool
  uint8_t battery_BMS_contactorState = 0;
  uint8_t battery_BMS_state = 0;
  uint8_t battery_BMS_hvState = 0;
  uint16_t battery_BMS_isolationResistance = 0;
  bool battery_BMS_chargeRequest = false;    //Change to bool
  bool battery_BMS_keepWarmRequest = false;  //Change to bool
  uint8_t battery_BMS_uiChargeStatus = 0;
  bool battery_BMS_diLimpRequest = false;   //Change to bool
  bool battery_BMS_okToShipByAir = false;   //Change to bool
  bool battery_BMS_okToShipByLand = false;  //Change to bool
  uint32_t battery_BMS_chgPowerAvailable = 0;
  uint8_t battery_BMS_chargeRetryCount = 0;
  bool battery_BMS_pcsPwmEnabled = false;        //Change to bool
  bool battery_BMS_ecuLogUploadRequest = false;  //Change to bool
  uint8_t battery_BMS_minPackTemperature = 0;
  // 0x224:548 PCS_dcdcStatus
  uint8_t battery_PCS_dcdcPrechargeStatus = 0;
  uint8_t battery_PCS_dcdc12VSupportStatus = 0;
  uint8_t battery_PCS_dcdcHvBusDischargeStatus = 0;
  uint16_t battery_PCS_dcdcMainState = 0;
  uint8_t battery_PCS_dcdcSubState = 0;
  bool battery_PCS_dcdcFaulted = false;          //Change to bool
  bool battery_PCS_dcdcOutputIsLimited = false;  //Change to bool
  uint32_t battery_PCS_dcdcMaxOutputCurrentAllowed = 0;
  uint8_t battery_PCS_dcdcPrechargeRtyCnt = 0;
  uint8_t battery_PCS_dcdc12VSupportRtyCnt = 0;
  uint8_t battery_PCS_dcdcDischargeRtyCnt = 0;
  uint8_t battery_PCS_dcdcPwmEnableLine = 0;
  uint8_t battery_PCS_dcdcSupportingFixedLvTarget = 0;
  uint8_t battery_PCS_ecuLogUploadRequest = 0;
  uint8_t battery_PCS_dcdcPrechargeRestartCnt = 0;
  uint8_t battery_PCS_dcdcInitialPrechargeSubState = 0;
  //0x312: 786 BMS_thermalStatus
  uint16_t BMS_powerDissipation = 0;
  uint16_t BMS_flowRequest = 0;
  uint16_t BMS_inletActiveCoolTargetT = 0;
  uint16_t BMS_inletPassiveTargetT = 0;
  uint16_t BMS_inletActiveHeatTargetT = 0;
  uint16_t BMS_packTMin = 0;
  uint16_t BMS_packTMax = 0;
  bool BMS_pcsNoFlowRequest = false;
  bool BMS_noFlowRequest = false;
  //0x2A4; 676 PCS_thermalStatus
  int16_t PCS_chgPhATemp = 0;
  int16_t PCS_chgPhBTemp = 0;
  int16_t PCS_chgPhCTemp = 0;
  int16_t PCS_dcdcTemp = 0;
  int16_t PCS_ambientTemp = 0;
  //0x2C4; 708 PCS_logging
  uint16_t PCS_logMessageSelect = 0;
  uint16_t PCS_dcdcMaxLvOutputCurrent = 0;
  uint16_t PCS_dcdcCurrentLimit = 0;
  uint16_t PCS_dcdcLvOutputCurrentTempLimit = 0;
  uint16_t PCS_dcdcUnifiedCommand = 0;
  uint16_t PCS_dcdcCLAControllerOutput = 0;
  int16_t PCS_dcdcTankVoltage = 0;
  uint16_t PCS_dcdcTankVoltageTarget = 0;
  uint16_t PCS_dcdcClaCurrentFreq = 0;
  int16_t PCS_dcdcTCommMeasured = 0;
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
  //0x7AA: //1962 HVP_debugMessage:
  uint8_t HVP_debugMessageMultiplexer = 0;
  bool HVP_gpioPassivePyroDepl = false;       //Change to bool
  bool HVP_gpioPyroIsoEn = false;             //Change to bool
  bool HVP_gpioCpFaultIn = false;             //Change to bool
  bool HVP_gpioPackContPowerEn = false;       //Change to bool
  bool HVP_gpioHvCablesOk = false;            //Change to bool
  bool HVP_gpioHvpSelfEnable = false;         //Change to bool
  bool HVP_gpioLed = false;                   //Change to bool
  bool HVP_gpioCrashSignal = false;           //Change to bool
  bool HVP_gpioShuntDataReady = false;        //Change to bool
  bool HVP_gpioFcContPosAux = false;          //Change to bool
  bool HVP_gpioFcContNegAux = false;          //Change to bool
  bool HVP_gpioBmsEout = false;               //Change to bool
  bool HVP_gpioCpFaultOut = false;            //Change to bool
  bool HVP_gpioPyroPor = false;               //Change to bool
  bool HVP_gpioShuntEn = false;               //Change to bool
  bool HVP_gpioHvpVerEn = false;              //Change to bool
  bool HVP_gpioPackCoontPosFlywheel = false;  //Change to bool
  bool HVP_gpioCpLatchEnable = false;         //Change to bool
  bool HVP_gpioPcsEnable = false;             //Change to bool
  bool HVP_gpioPcsDcdcPwmEnable = false;      //Change to bool
  bool HVP_gpioPcsChargePwmEnable = false;    //Change to bool
  bool HVP_gpioFcContPowerEnable = false;     //Change to bool
  bool HVP_gpioHvilEnable = false;            //Change to bool
  bool HVP_gpioSecDrdy = false;               //Change to bool
  uint16_t HVP_hvp1v5Ref = 0;
  int16_t HVP_shuntCurrentDebug = 0;
  bool HVP_packCurrentMia = false;           //Change to bool
  bool HVP_auxCurrentMia = false;            //Change to bool
  bool HVP_currentSenseMia = false;          //Change to bool
  bool HVP_shuntRefVoltageMismatch = false;  //Change to bool
  bool HVP_shuntThermistorMia = false;       //Change to bool
  bool HVP_shuntHwMia = false;               //Change to bool
  int16_t HVP_dcLinkVoltage = 0;
  int16_t HVP_packVoltage = 0;
  int16_t HVP_fcLinkVoltage = 0;
  uint16_t HVP_packContVoltage = 0;
  int16_t HVP_packNegativeV = 0;
  int16_t HVP_packPositiveV = 0;
  uint16_t HVP_pyroAnalog = 0;
  int16_t HVP_dcLinkNegativeV = 0;
  int16_t HVP_dcLinkPositiveV = 0;
  int16_t HVP_fcLinkNegativeV = 0;
  uint16_t HVP_fcContCoilCurrent = 0;
  uint16_t HVP_fcContVoltage = 0;
  uint16_t HVP_hvilInVoltage = 0;
  uint16_t HVP_hvilOutVoltage = 0;
  int16_t HVP_fcLinkPositiveV = 0;
  uint16_t HVP_packContCoilCurrent = 0;
  uint16_t HVP_battery12V = 0;
  int16_t HVP_shuntRefVoltageDbg = 0;
  int16_t HVP_shuntAuxCurrentDbg = 0;
  int16_t HVP_shuntBarTempDbg = 0;
  int16_t HVP_shuntAsicTempDbg = 0;
  uint8_t HVP_shuntAuxCurrentStatus = 0;
  uint8_t HVP_shuntBarTempStatus = 0;
  uint8_t HVP_shuntAsicTempStatus = 0;
  //0x3aa: HVP_alertMatrix1 Fault codes   // Change to bool
  bool battery_WatchdogReset = false;           //Warns if the processor has experienced a reset due to watchdog reset.
  bool battery_PowerLossReset = false;          //Warns if the processor has experienced a reset due to power loss.
  bool battery_SwAssertion = false;             //An internal software assertion has failed.
  bool battery_CrashEvent = false;              //Warns if the crash signal is detected by HVP
  bool battery_OverDchgCurrentFault = false;    //Warns if the pack discharge is above max discharge current limit
  bool battery_OverChargeCurrentFault = false;  //Warns if the pack discharge current is above max charge current limit
  bool battery_OverCurrentFault = false;  //Warns if the pack current (discharge or charge) is above max current limit.
  bool battery_OverTemperatureFault = false;  //A pack module temperature is above maximum temperature limit
  bool battery_OverVoltageFault = false;      //A brick voltage is above maximum voltage limit
  bool battery_UnderVoltageFault = false;     //A brick voltage is below minimum voltage limit
  bool battery_PrimaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from primary BMB chain are mia
  bool battery_SecondaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
  bool battery_BmbMismatchFault =
      false;  //Warns if the primary and secondary BMB chain readings don't match with each other
  bool battery_BmsHviMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_CpMiaFault = false;       //Warns if the CP node is mia on HVS CAN
  bool battery_PcsMiaFault = false;      //The PCS node is mia on HVS CAN
  bool battery_BmsFault = false;         //Warns if the BMS ECU has faulted
  bool battery_PcsFault = false;         //Warns if the PCS ECU has faulted
  bool battery_CpFault = false;          //Warns if the CP ECU has faulted
  bool battery_ShuntHwMiaFault = false;  //Warns if the shunt current reading is not available
  bool battery_PyroMiaFault = false;     //Warns if the pyro squib is not connected
  bool battery_hvsMiaFault = false;      //Warns if the pack contactor hw fault
  bool battery_hviMiaFault = false;      //Warns if the FC contactor hw fault
  bool battery_Supply12vFault = false;   //Warns if the low voltage (12V) battery is below minimum voltage threshold
  bool battery_VerSupplyFault = false;   //Warns if the Energy reserve voltage supply is below minimum voltage threshold
  bool battery_HvilFault = false;        //Warn if a High Voltage Inter Lock fault is detected
  bool battery_BmsHvsMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_PackVoltMismatchFault =
      false;                         //Warns if the pack voltage doesn't match approximately with sum of brick voltages
  bool battery_EnsMiaFault = false;  //Warns if the ENS line is not connected to HVC
  bool battery_PackPosCtrArcFault = false;  //Warns if the HVP detectes series arc at pack contactor
  bool battery_packNegCtrArcFault = false;  //Warns if the HVP detectes series arc at FC contactor
  bool battery_ShuntHwAndBmsMiaFault = false;
  bool battery_fcContHwFault = false;
  bool battery_robinOverVoltageFault = false;
  bool battery_packContHwFault = false;
  bool battery_pyroFuseBlown = false;
  bool battery_pyroFuseFailedToBlow = false;
  bool battery_CpilFault = false;
  bool battery_PackContactorFellOpen = false;
  bool battery_FcContactorFellOpen = false;
  bool battery_packCtrCloseBlocked = false;
  bool battery_fcCtrCloseBlocked = false;
  bool battery_packContactorForceOpen = false;
  bool battery_fcContactorForceOpen = false;
  bool battery_dcLinkOverVoltage = false;
  bool battery_shuntOverTemperature = false;
  bool battery_passivePyroDeploy = false;
  bool battery_logUploadRequest = false;
  bool battery_packCtrCloseFailed = false;
  bool battery_fcCtrCloseFailed = false;
  bool battery_shuntThermistorMia = false;
  //0x320: 800 BMS_alertMatrix
  uint8_t battery_BMS_matrixIndex = 0;  // Changed to bool
  bool battery_BMS_a061_robinBrickOverVoltage = false;
  bool battery_BMS_a062_SW_BrickV_Imbalance = false;
  bool battery_BMS_a063_SW_ChargePort_Fault = false;
  bool battery_BMS_a064_SW_SOC_Imbalance = false;
  bool battery_BMS_a127_SW_shunt_SNA = false;
  bool battery_BMS_a128_SW_shunt_MIA = false;
  bool battery_BMS_a069_SW_Low_Power = false;
  bool battery_BMS_a130_IO_CAN_Error = false;
  bool battery_BMS_a071_SW_SM_TransCon_Not_Met = false;
  bool battery_BMS_a132_HW_BMB_OTP_Uncorrctbl = false;
  bool battery_BMS_a134_SW_Delayed_Ctr_Off = false;
  bool battery_BMS_a075_SW_Chg_Disable_Failure = false;
  bool battery_BMS_a076_SW_Dch_While_Charging = false;
  bool battery_BMS_a017_SW_Brick_OV = false;
  bool battery_BMS_a018_SW_Brick_UV = false;
  bool battery_BMS_a019_SW_Module_OT = false;
  bool battery_BMS_a021_SW_Dr_Limits_Regulation = false;
  bool battery_BMS_a022_SW_Over_Current = false;
  bool battery_BMS_a023_SW_Stack_OV = false;
  bool battery_BMS_a024_SW_Islanded_Brick = false;
  bool battery_BMS_a025_SW_PwrBalance_Anomaly = false;
  bool battery_BMS_a026_SW_HFCurrent_Anomaly = false;
  bool battery_BMS_a087_SW_Feim_Test_Blocked = false;
  bool battery_BMS_a088_SW_VcFront_MIA_InDrive = false;
  bool battery_BMS_a089_SW_VcFront_MIA = false;
  bool battery_BMS_a090_SW_Gateway_MIA = false;
  bool battery_BMS_a091_SW_ChargePort_MIA = false;
  bool battery_BMS_a092_SW_ChargePort_Mia_On_Hv = false;
  bool battery_BMS_a034_SW_Passive_Isolation = false;
  bool battery_BMS_a035_SW_Isolation = false;
  bool battery_BMS_a036_SW_HvpHvilFault = false;
  bool battery_BMS_a037_SW_Flood_Port_Open = false;
  bool battery_BMS_a158_SW_HVP_HVI_Comms = false;
  bool battery_BMS_a039_SW_DC_Link_Over_Voltage = false;
  bool battery_BMS_a041_SW_Power_On_Reset = false;
  bool battery_BMS_a042_SW_MPU_Error = false;
  bool battery_BMS_a043_SW_Watch_Dog_Reset = false;
  bool battery_BMS_a044_SW_Assertion = false;
  bool battery_BMS_a045_SW_Exception = false;
  bool battery_BMS_a046_SW_Task_Stack_Usage = false;
  bool battery_BMS_a047_SW_Task_Stack_Overflow = false;
  bool battery_BMS_a048_SW_Log_Upload_Request = false;
  bool battery_BMS_a169_SW_FC_Pack_Weld = false;
  bool battery_BMS_a050_SW_Brick_Voltage_MIA = false;
  bool battery_BMS_a051_SW_HVC_Vref_Bad = false;
  bool battery_BMS_a052_SW_PCS_MIA = false;
  bool battery_BMS_a053_SW_ThermalModel_Sanity = false;
  bool battery_BMS_a054_SW_Ver_Supply_Fault = false;
  bool battery_BMS_a176_SW_GracefulPowerOff = false;
  bool battery_BMS_a059_SW_Pack_Voltage_Sensing = false;
  bool battery_BMS_a060_SW_Leakage_Test_Failure = false;
  bool battery_BMS_a077_SW_Charger_Regulation = false;
  bool battery_BMS_a081_SW_Ctr_Close_Blocked = false;
  bool battery_BMS_a082_SW_Ctr_Force_Open = false;
  bool battery_BMS_a083_SW_Ctr_Close_Failure = false;
  bool battery_BMS_a084_SW_Sleep_Wake_Aborted = false;
  bool battery_BMS_a094_SW_Drive_Inverter_MIA = false;
  bool battery_BMS_a099_SW_BMB_Communication = false;
  bool battery_BMS_a105_SW_One_Module_Tsense = false;
  bool battery_BMS_a106_SW_All_Module_Tsense = false;
  bool battery_BMS_a107_SW_Stack_Voltage_MIA = false;
  bool battery_BMS_a121_SW_NVRAM_Config_Error = false;
  bool battery_BMS_a122_SW_BMS_Therm_Irrational = false;
  bool battery_BMS_a123_SW_Internal_Isolation = false;
  bool battery_BMS_a129_SW_VSH_Failure = false;
  bool battery_BMS_a131_Bleed_FET_Failure = false;
  bool battery_BMS_a136_SW_Module_OT_Warning = false;
  bool battery_BMS_a137_SW_Brick_UV_Warning = false;
  bool battery_BMS_a138_SW_Brick_OV_Warning = false;
  bool battery_BMS_a139_SW_DC_Link_V_Irrational = false;
  bool battery_BMS_a141_SW_BMB_Status_Warning = false;
  bool battery_BMS_a144_Hvp_Config_Mismatch = false;
  bool battery_BMS_a145_SW_SOC_Change = false;
  bool battery_BMS_a146_SW_Brick_Overdischarged = false;
  bool battery_BMS_a149_SW_Missing_Config_Block = false;
  bool battery_BMS_a151_SW_external_isolation = false;
  bool battery_BMS_a156_SW_BMB_Vref_bad = false;
  bool battery_BMS_a157_SW_HVP_HVS_Comms = false;
  bool battery_BMS_a159_SW_HVP_ECU_Error = false;
  bool battery_BMS_a161_SW_DI_Open_Request = false;
  bool battery_BMS_a162_SW_No_Power_For_Support = false;
  bool battery_BMS_a163_SW_Contactor_Mismatch = false;
  bool battery_BMS_a164_SW_Uncontrolled_Regen = false;
  bool battery_BMS_a165_SW_Pack_Partial_Weld = false;
  bool battery_BMS_a166_SW_Pack_Full_Weld = false;
  bool battery_BMS_a167_SW_FC_Partial_Weld = false;
  bool battery_BMS_a168_SW_FC_Full_Weld = false;
  bool battery_BMS_a170_SW_Limp_Mode = false;
  bool battery_BMS_a171_SW_Stack_Voltage_Sense = false;
  bool battery_BMS_a174_SW_Charge_Failure = false;
  bool battery_BMS_a179_SW_Hvp_12V_Fault = false;
  bool battery_BMS_a180_SW_ECU_reset_blocked = false;
};

class TeslaModel3YBattery : public TeslaBattery {
 public:
  TeslaModel3YBattery() {
#ifdef EXP_TESLA_BMS_DIGITAL_HVIL
    operate_contactors = true;
#endif
  }
  static constexpr char* Name = "Tesla Model 3/Y";
  virtual void setup(void);
};

class TeslaModelSXBattery : public TeslaBattery {
 public:
  TeslaModelSXBattery() { operate_contactors = true; }
  static constexpr char* Name = "Tesla Model S/X";
  virtual void setup(void);
};

#endif
