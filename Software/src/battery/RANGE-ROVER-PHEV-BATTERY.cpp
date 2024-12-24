#include "../include.h"
#ifdef RANGE_ROVER_PHEV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RANGE-ROVER-PHEV-BATTERY.h"

/* TODO
- LOG files from vehicle needed to determine CAN content needed to send towards battery!
  - BCCM_PMZ_A (0x18B 50ms)
  - BCCMB_PMZ_A (0x224 90ms)
  - BCM_CCP_RX_PMZCAN (0x601 non cyclic)
  - EPIC_PMZ_B (0x009 non cyclic)
  - GWM_FuelPumpEnableDataControl_PMZ (0x1F8 non cyclic)
  - GWM_IgnitionAuthDataTarget_PMZ (0x004 non cyclic)
  - GWM_PMZ_A (0x008 10ms cyclic)
  - GWM_PMZ_B -F, G-I, Immo, K-P 
    - 0x010 10ms
    - 0x090 10ms
    - 0x108 20ms
    - 0x110 20ms
    - 0x1d0 80ms
    - 0x490 900ms
    - 0x1B0 80ms
    - 0x460 720ms
    - 0x006 non cyclic immo
    - 0x450 600ms
    - 0x2b8 180ms
    - 0x388 200ms
    - 0x2b0 180ms
    - 0x380 80ms
  - GWM_PMZ_V_HYBRID (0x18d 60ms)
  - HVAC_PMZ_A-E
    - 0x1a8 70ms
    - 0x210 100ms
    - 0x300 200ms
    - 0x440 180ms
    - 0x0c0 10ms 
  - PCM_PMZ_C_Hybrid C, D, H, M
    - 0x030 15ms
    - 0x304 180ms
    - 0x1C0 80ms
    - 0x434 350ms
  - TCU_PMZ_A
    - 0x014 non cyclic, command from TCU, most likely not needed
- Determine CRC calculation
- Figure out contactor closing requirements
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis50ms = 0;  // will store last time a 50ms CAN Message was sent

//CAN content from battery
static bool StatusCAT5BPOChg = false;
static bool StatusCAT4Derate = false;
static uint8_t OCMonitorStatus = 0;
static bool StatusCAT3 = false;
static bool IsolationStatus = false;
static bool HVILStatus = false;
static bool ContactorStatus = false;
static uint8_t StatusGpCounter = 0;
static bool WeldCheckStatus = false;
static bool StatusCAT7NowBPO = false;
static bool StatusCAT6DlyBPO = false;
static uint8_t StatusGpCS = 0;
static uint8_t CAT6Count = 0;
static bool EndOfCharge = false;
static bool DerateWarning = false;
static bool PrechargeAllowed = false;
static uint8_t DischargeExtGpCounter = 0;    // Counter 0-15
static uint8_t DischargeExtGpCS = 0;         // CRC
static uint16_t DischargeVoltageLimit = 0;   //Min voltage battery allows discharging to
static uint16_t DischargePowerLimitExt = 0;  //Momentary Discharge power limit kW*0.01 (0-655)
static uint16_t DischargeContPwrLmt = 0;     //Longterm Discharge power limit kW*0.01 (0-655)
static uint8_t PwrGpCS = 0;                  // CRC
static uint8_t PwrGpCounter = 0;             // Counter 0-15
static uint16_t VoltageExt = 370;            // Voltage of the HV Battery
static uint16_t VoltageBus = 0;              // Voltage on the high-voltage DC bus
static int32_t CurrentExt =
    209715;  //Positive - discharge, Negative Charge (0 - 16777215) Scaling: 0.025 Offset: -209715.175 Units: Amps
static bool HVIsolationTestRunning = false;
static uint16_t VoltageOC =
    0;  //The instantaneous equivalent open-circuit voltage of the high voltage battery. This is used by the high-voltage inverter in power prediction and derating calculations.
static uint16_t DchCurrentLimit =
    0;  // A, 'Maximum current that can be delivered by the HV Battery during motoring mode  i.e during discharging.
static uint16_t ChgCurrentLimit =
    0;  // - 1023 A, Maximum current that can be transferred into the HV Battery during generating mode i.e during charging. Charging is neagtive and discharging is positive.
static uint16_t ChargeContPwrLmt = 0;      //Longterm charge power limit kW*0.01 (0-655)
static uint16_t ChargePowerLimitExt = 0;   //Momentary Charge power limit kW*0.01 (0-655)
static uint8_t ChgExtGpCS = 0;             // CRC
static uint8_t ChgExtGpCounter = 0;        //counter 0-15
static uint16_t ChargeVoltageLimit = 500;  //Max voltage limit during charging of the HV Battery.
static uint8_t CurrentWarning = 0;         // 0 normal, 1 cell overcurrent, 2 cell undercurrent
static uint8_t TempWarning = 0;            // 0 normal, 1 cell overtemp, 2 cell undertemp
static int8_t TempUpLimit = 0;             //Upper temperature limit.
static uint8_t CellVoltWarning = 0;        // 0 normal, 1 cell overvoltage, 2 cell undervoltage
static bool CCCVChargeMode = false;        //0 CC, 1 = CV
static uint16_t CellVoltUpLimit = 0;       //mV, Upper cell voltage limit
static uint16_t SOCHighestCell = 0;        //0.01, %
static uint16_t SOCLowestCell = 0;         //0.01, %
static uint16_t SOCAverage = 0;            //0.01, %
static bool WakeUpTopUpReq =
    false;  //The HV Battery can trigger a vehicle wake-up to request its State of Charge to be increased.
static bool WakeUpThermalReq =
    false;  //The HV Battery can trigger a vehicle wake-up in order to be thermally managed (ie. cooled down OR warmed up).
static bool WakeUpDchReq =
    false;  //The HV Battery can trigger a vehicle wake-up to request its State of Charge to be reduced.
static uint16_t StateofHealth = 0;
static uint16_t EstimatedLossChg =
    0;  //fact0.001, kWh Expected energy which will be lost during charging (at the rate given by VSCEstChargePower) due to resistance within the HV Battery.
static bool CoolingRequest =
    false;  //HV Battery cooling request to be cooled by the eAC/chiller as its cooling needs exceed the LTR cooling loop capability.
static uint16_t EstimatedLossDch =
    0;  //fact0.001, kWh Expected energy which will be lost during discharging (at the rate given by VSCEstDischargePower) due to resistance within the HV Battery.
static uint8_t FanDutyRequest =
    0;  //Request from the HV Battery cooling system to demand a change of duty for the electrical engine cooling fan speed (whilst using its LTR cooling loop).
static bool ValveCtrlStat = false;  //0 Chiller/Heater cooling loop requested , 1 LTR cooling loop requested
static uint16_t EstLossDchTgtSoC =
    0;  //fact0.001, kWh Expected energy which will be lost during discharging (at the rate given by VSCEstimatedDchPower) from the target charging SoC (PHEV: HVBattEnergyUsableMax, BEV: HVBattEnergyUsableBulk) down to HVBattEnergyUsableMin, due to resistance within the Traction Battery.
static uint8_t HeatPowerGenChg =
    0;  //fact0.1, kW, Estimated average heat generated by battery if charged at the rate given by VSCEstimatedChgPower.
static uint8_t HeatPowerGenDch =
    0;  //fact0.1, kW, Estimated average heat generated by battery if discharged at the rate given by VSCEstimatedDchPower.
static uint8_t WarmupRateChg =
    0;  //fact0.1, C/min , Expected average rate at which the battery will self-heat if charged at the rate given by VSCEstimatedChgPower.
static uint8_t WarmupRateDch =
    0;  //fact0.1, C/min , Expected average rate at which the battery will self-heat if discharged at the rate given by VSCEstimatedDchPower.
static uint16_t CellVoltageMax = 3700;
static uint16_t CellVoltageMin = 3700;
static int8_t CellTempAverage = 0;     //factor0.5, -40 offset
static int8_t CellTempColdest = 0;     //factor0.5, -40 offset
static int8_t CellTempHottest = 0;     //factor0.5, -40 offset
static uint8_t HeaterCtrlStat = 0;     //factor1, 0 offset
static bool ThermalOvercheck = false;  // 0 OK, 1 NOT OK
static int8_t InletCoolantTemp = 0;    //factor0.5, -40 offset
static bool ClntPumpDiagStat_UB = false;
static bool InletCoolantTemp_UB = false;
static bool CoolantLevel = false;      // Coolant level OK , 1 NOT OK
static bool ClntPumpDiagStat = false;  // 0 Pump OK, 1 NOT OK
static uint8_t MILRequest = 0;         //No req, 1 ON, 2 FLASHING, 3 unused
static uint16_t EnergyAvailable = 0;   //fac0.05 , The total energy available from the HV Battery
static uint16_t EnergyUsableMax = 0;   //fac0.05 , The total energy available from the HV Battery at its maximum SOC
static uint16_t EnergyUsableMin = 0;   //fac0.05 , The total energy available from the HV Battery at its minimum SOC
static uint16_t TotalCapacity =
    0;  //fac0.1 , Total Battery capacity in Kwh. This will reduce over the lifetime of the HV Battery.

//CAN messages needed by battery (LOG needed!)
CAN_frame RANGE_ROVER_18B = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x18B,  //CONTENT??? TODO
                             .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void update_values_battery() {

  datalayer.battery.status.real_soc = SOCAverage;

  datalayer.battery.status.soh_pptt = StateofHealth * 10;

  datalayer.battery.status.voltage_dV = VoltageExt * 10;

  datalayer.battery.status.current_dA = (CurrentExt * 0.025) - 209715;

  datalayer.battery.status.max_charge_power_W = (ChargeContPwrLmt * 10) - 6550;

  datalayer.battery.status.max_discharge_power_W = (DischargeContPwrLmt * 10) - 6550;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = CellVoltageMax;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltageMin;

  datalayer.battery.status.temperature_min_dC = CellTempColdest * 10;

  datalayer.battery.status.temperature_max_dC = CellTempHottest * 10;

  datalayer.battery.info.max_design_voltage_dV = ChargeVoltageLimit * 10;

  datalayer.battery.info.min_design_voltage_dV = DischargeVoltageLimit * 10;
}

void map_can_frame_to_variable_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x080:  // 15ms
      StatusCAT5BPOChg = (rx_frame.data.u8[0] & 0x01);
      StatusCAT4Derate = (rx_frame.data.u8[0] & 0x02) >> 1;
      OCMonitorStatus = (rx_frame.data.u8[0] & 0x0C) >> 2;
      StatusCAT3 = (rx_frame.data.u8[0] & 0x10) >> 4;
      IsolationStatus = (rx_frame.data.u8[0] & 0x20) >> 5;
      HVILStatus = (rx_frame.data.u8[0] & 0x40) >> 6;
      ContactorStatus = (rx_frame.data.u8[0] & 0x80) >> 7;
      StatusGpCounter = (rx_frame.data.u8[1] & 0x0F);
      WeldCheckStatus = (rx_frame.data.u8[1] & 0x20) >> 5;
      StatusCAT7NowBPO = (rx_frame.data.u8[1] & 0x40) >> 6;
      StatusCAT6DlyBPO = (rx_frame.data.u8[1] & 0x80) >> 7;
      StatusGpCS = rx_frame.data.u8[2];
      CAT6Count = rx_frame.data.u8[3] & 0x7F;
      EndOfCharge = (rx_frame.data.u8[6] & 0x04) >> 2;
      DerateWarning = (rx_frame.data.u8[6] & 0x08) >> 3;
      PrechargeAllowed = (rx_frame.data.u8[6] & 0x10) >> 4;
      break;
    case 0x100:  // 20ms
      DischargeExtGpCounter = (rx_frame.data.u8[0] & 0x0F);
      DischargeExtGpCS = rx_frame.data.u8[1];
      DischargeVoltageLimit = (((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]);
      DischargePowerLimitExt = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      DischargeContPwrLmt = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      break;
    case 0x102:  // 20ms
      PwrGpCS = rx_frame.data.u8[0];
      PwrGpCounter = (rx_frame.data.u8[1] & 0x3C) >> 2;
      VoltageExt = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[2]);
      VoltageBus = (((rx_frame.data.u8[3] & 0x03) << 8) | rx_frame.data.u8[4]);
      CurrentExt = ((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      break;
    case 0x104:  // 20ms
      HVIsolationTestRunning = (rx_frame.data.u8[2] & 0x10) >> 4;
      VoltageOC = (((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]);
      DchCurrentLimit = (((rx_frame.data.u8[4] & 0x03) << 8) | rx_frame.data.u8[5]);
      ChgCurrentLimit = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x10A:  // 20ms
      ChargeContPwrLmt = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      ChargePowerLimitExt = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      ChgExtGpCS = rx_frame.data.u8[4];
      ChgExtGpCounter = (rx_frame.data.u8[5] >> 4);
      ChargeVoltageLimit = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x198:  // 60ms
      CurrentWarning = (rx_frame.data.u8[4] & 0x03);
      TempWarning = ((rx_frame.data.u8[4] & 0x0C) >> 2);
      TempUpLimit = (rx_frame.data.u8[5] / 2) - 40;
      CellVoltWarning = ((rx_frame.data.u8[6] & 0x60) >> 5);
      CCCVChargeMode = ((rx_frame.data.u8[6] & 0x80) >> 7);
      CellVoltUpLimit = (((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x220:  // 100ms
      SOCHighestCell = (((rx_frame.data.u8[0] & 0x3F) << 8) | rx_frame.data.u8[1]);
      SOCLowestCell = (((rx_frame.data.u8[2] & 0x3F) << 8) | rx_frame.data.u8[3]);
      SOCAverage = (((rx_frame.data.u8[4] & 0x3F) << 8) | rx_frame.data.u8[5]);
      WakeUpTopUpReq = ((rx_frame.data.u8[6] & 0x04) >> 2);
      WakeUpThermalReq = ((rx_frame.data.u8[6] & 0x08) >> 3);
      WakeUpDchReq = ((rx_frame.data.u8[6] & 0x10) >> 4);
      StateofHealth = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x308:  // 190ms
      EstimatedLossChg = (((rx_frame.data.u8[0] & 0x03) << 8) | rx_frame.data.u8[1]);
      CoolingRequest = ((rx_frame.data.u8[6] & 0x04) >> 2);
      EstimatedLossDch = (((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]);
      FanDutyRequest = (rx_frame.data.u8[4] & 0x7F);
      ValveCtrlStat = ((rx_frame.data.u8[4] & 0x80) >> 7);
      EstLossDchTgtSoC = (((rx_frame.data.u8[5] & 0x03) << 8) | rx_frame.data.u8[6]);
      break;
    case 0x424:  // 280ms
      HeatPowerGenChg = (rx_frame.data.u8[0] & 0x7F);
      HeatPowerGenDch = (rx_frame.data.u8[1] & 0x7F);
      WarmupRateChg = (rx_frame.data.u8[2] & 0x3F);
      WarmupRateDch = (rx_frame.data.u8[3] & 0x3F);
      CellVoltageMax = (((rx_frame.data.u8[4] & 0x1F) << 8) | rx_frame.data.u8[5]);
      CellVoltageMin = (((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x448:  // 600ms
      CellTempAverage = (rx_frame.data.u8[0] / 2) - 40;
      CellTempColdest = (rx_frame.data.u8[1] / 2) - 40;
      CellTempHottest = (rx_frame.data.u8[2] / 2) - 40;
      HeaterCtrlStat = (rx_frame.data.u8[3] & 0x7F);
      ThermalOvercheck = ((rx_frame.data.u8[3] & 0x80) >> 7);
      InletCoolantTemp = rx_frame.data.u8[5];
      ClntPumpDiagStat_UB = ((rx_frame.data.u8[6] & 0x04) >> 2);
      InletCoolantTemp_UB = ((rx_frame.data.u8[6] & 0x08) >> 3);
      CoolantLevel = ((rx_frame.data.u8[6] & 0x10) >> 4);
      ClntPumpDiagStat = ((rx_frame.data.u8[6] & 0x20) >> 5);
      MILRequest = ((rx_frame.data.u8[6] & 0xC0) >> 6);
      break;
    case 0x464:  // 800ms
      EnergyAvailable = (((rx_frame.data.u8[0] & 0x07) << 8) | rx_frame.data.u8[1]);
      EnergyUsableMax = (((rx_frame.data.u8[2] & 0x07) << 8) | rx_frame.data.u8[3]);
      EnergyUsableMin = (((rx_frame.data.u8[4] & 0x07) << 8) | rx_frame.data.u8[5]);
      TotalCapacity = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x5A2:  //Not periodically transferred
      break;
    case 0x656:  //Not periodically transferred
      break;
    case 0x657:  //Not periodically transferred
      break;
    case 0x6C8:  //Not periodically transferred
      break;
    case 0x6C9:  //Not periodically transferred
      break;
    case 0x6CA:  //Not periodically transferred
      break;
    case 0x6CB:  //Not periodically transferred
      break;
    case 0x7EC:  //Not periodically transferred
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {

    previousMillis50ms = currentMillis;

    transmit_can_frame(&RANGE_ROVER_18B, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Range Rover 13kWh PHEV battery (L494/L405)", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}

#endif  //RANGE_ROVER_PHEV_BATTERY
