#include "RANGE-ROVER-PHEV-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

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

void RangeRoverPhevBattery::update_values() {

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

void RangeRoverPhevBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
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

void RangeRoverPhevBattery::transmit_can(unsigned long currentMillis) {
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {
    previousMillis50ms = currentMillis;

    transmit_can_frame(&RANGE_ROVER_18B, can_config.battery);
  }
}

void RangeRoverPhevBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
