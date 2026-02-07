#include "LAND-ROVER-VELAR-PHEV-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/* TODO
*/

void LandRoverVelarPhevBattery::update_values() {

  datalayer.battery.status.real_soc = HVBattSOCAverage;

  datalayer.battery.status.soh_pptt = HVBattStateofHealth * 10;

  datalayer.battery.status.voltage_dV = HVBattVoltageExt * 10;

  //datalayer.battery.status.current_dA;

  //datalayer.battery.status.max_charge_power_W;

  //datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = HVBattCellVoltageMax;

  datalayer.battery.status.cell_min_voltage_mV = HVBattCellVoltageMin;

  datalayer.battery.status.temperature_min_dC = HVBattCellTempColdest * 10;

  datalayer.battery.status.temperature_max_dC = HVBattCellTempHottest * 10;

  if (HVBattHVILStatus) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  if (HVBattAuxiliaryFuse) {
    set_event(EVENT_BATTERY_FUSE, 0);
  } else {
    clear_event(EVENT_BATTERY_FUSE);
  }

  if (HVBattAuxiliaryFuse) {
    set_event(EVENT_BATTERY_FUSE, 1);
  } else {
    clear_event(EVENT_BATTERY_FUSE);
  }

  if (HVBattAuxiliaryFuse) {
    set_event(EVENT_BATTERY_FUSE, 2);
  } else {
    clear_event(EVENT_BATTERY_FUSE);
  }

  if (HVBattStatusCritical == 2) {
    set_event(EVENT_THERMAL_RUNAWAY, 0);
  }
}

void LandRoverVelarPhevBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x080:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x088:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattChgExtGpCS
      //HVBattChgExtGpCounter
      //HVBattChgVoltageLimit
      //HVBattChgPowerLimitExt
      //HVBattChgContPwrLmt
      break;
    case 0x08A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattTotalCapacity
      //HVBattChgCurrentLimit
      //HVBattFastChgCounter
      //HVBattPrechargeAllowed
      //HVBattEndOfCharge
      //HVBattDerateWarning
      //HVBattCCCVChargeMode
      break;
    case 0x08C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattDchExtGpCS
      //HVBattDchExtGpCounter
      //HVBattDchVoltageLimit
      //HVBattDchContPwrLmt
      //HVBattDchPowerLimitExt
      break;
    case 0x08E:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattEstEgyDischLoss
      //HVBattDchCurrentLimit
      //HVBattLossDchRouteTrac
      //HVBattLossDchRouteTot
      break;
    case 0x090:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattVoltageOC
      //HVBattCurrentWarning
      //HVBattMILRequest
      HVBattTractionFuseF = (rx_frame.data.u8[2] & 0x40) >> 6;
      HVBattTractionFuseR = (rx_frame.data.u8[2] & 0x80) >> 7;
      break;
    case 0x098:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattStatusGpCS
      //HVBattStatusGpCounter
      //HVBattOCMonitorStatus
      //HVBattContactorStatus
      HVBattHVILStatus = (rx_frame.data.u8[1] & 0x80) >> 7;
      //HVBattWeldCheckStatus
      //HVBattStatusCAT7NowBPO
      //HVBattStatusCAT6DlyBPO
      //HVBattStatusCAT5BPOChg
      //HVBattStatusCAT4Derate
      //HVBattStatusCAT3
      //HVBattIsolationStatus
      //HVBattCAT6Count
      HVBattAuxiliaryFuse = (rx_frame.data.u8[3] & 0x80) >> 7;
      HVBattStateofHealth = (((rx_frame.data.u8[4] & 0x03) << 8) | rx_frame.data.u8[5]);
      HVBattStatusCritical = (rx_frame.data.u8[5] & 0x0C) >> 2;
      //HVBattIsolationStatus2
      //HVBattClntPumpDiagStat
      //HVBattValveCtrlStat
      //HVIsolationTestStatus
      //HVBattIsoRes
      break;
    case 0x0C8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattPwrExtGpCS
      //HVBattPwrExtGpCounter
      //HVBattVoltageBusTF
      //HVBattVoltageBusTR
      //HVBattCurrentTR
      break;
    case 0x0CA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattPwrGpCS
      //HVBattVoltageBus
      HVBattVoltageExt = (((rx_frame.data.u8[3] & 0x03) << 8) | rx_frame.data.u8[4]);
      //HVBattPwrGpCounter
      //HVBattCurrentExt
      break;
    case 0x0EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattStatExtGpCS
      //HVBattStatExtGpCounter
      //HVBattOCMonitorStatusTR
      //HVBattOCMonitorStatusTF
      //HVBattWeldCheckStatusT
      //HVBattWeldCheckStatusC
      //HVBattContactorStatusT
      break;
    case 0x18B:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x146:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattEstimatedLossChg
      //HVBattEstimatedLossDch
      //HVBattEstLossDchTgtSoC
      //HVBattEstLossTracDch
      break;
    case 0x148:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattThermalMgtStatus
      //HVBattThermalOvercheck
      //HVBattTempWarning
      //HVBattAvTempAtEvent
      //HVBattTempUpLimit
      HVBattCellTempHottest = (rx_frame.data.u8[5] / 2) - 40;
      HVBattCellTempColdest = (rx_frame.data.u8[6] / 2) - 40;
      //HVBattCellTempAverage
      break;
    case 0x14C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattCellVoltUpLimit
      HVBattCellVoltageMin = (((rx_frame.data.u8[4] & 0x1F) << 8) | rx_frame.data.u8[5]);
      HVBattCellVoltageMax = (((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[7]);
      //HVBattCellVoltWarning
      break;
    case 0x14E:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattWarmupRateChg
      //HVBattWarmupRateDch
      //HVBattWarmupRateRtTot
      //HVBattWakeUpDchReq
      //HVBattWarmupRateRtTrac
      //HVBattWakeUpThermalReq
      //HVBattWakeUpTopUpReq
      break;
    case 0x171:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattHeaterCtrlStat
      //HVBattCoolingRequestExt
      //HVBattFanDutyRequest
      //HVBattInletCoolantTemp
      //HVBattHeatPowerGenChg
      //HVBattHeatPowerGenDch
      //HVBattHeatPwrGenRtTot
      //HVBattCoolantLevel
      //HVBattHeatPwrGenRtTrac
      //HVBattHeatingRequest
      break;
    case 0x204:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattChgExtGp2CS
      //HVBattChgExtGp2AC
      //HVBattChgContPwrLmtExt
      break;
    case 0x207:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattAvgSOCOAtEvent
      //HVBattSOCLowestCell
      //HVBattSOCHighestCell
      HVBattSOCAverage = (((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x27A:  //Cellvoltages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // Extract module ID (1-8) and sequence counter from first byte
      module_id = rx_frame.data.u8[0] >> 1;  // Bits 7-1: 1-8

      // Extract voltage group from second byte (1-4 for groups of 3 cells)
      voltage_group = rx_frame.data.u8[1] >> 2;  // Bits 7-2: 1-4

      // Calculate the starting index for these 3 cells
      // Each module has 12 cells (4 groups × 3 cells)
      // Module 1: cells 0-11, Module 2: cells 12-23, etc.
      // Voltage group 1: first 3 cells, group 2: next 3, etc.
      base_index = ((module_id - 1) * 12) + ((voltage_group - 1) * 3);

      // Decode the 3 cell voltages from the message
      // Format appears to be: high 4 bits in byte 2/4/6, low 8 bits in following byte
      datalayer.battery.status.cell_voltages_mV[base_index] = ((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[3];
      datalayer.battery.status.cell_voltages_mV[base_index + 1] =
          ((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5];
      datalayer.battery.status.cell_voltages_mV[base_index + 2] =
          ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[7];
      break;
    case 0x27E:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //celltemperatures same as above
      break;
    case 0x310:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HVBattEnergyAvailable
      //HVBattEnergyUsableBulk
      //HVBattEnergyUsableMax
      //HVBattEnergyUsableMin
      break;
    case 0x318:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //ID of min/max cells for temps and voltages
      break;
    default:
      break;
  }
}

void LandRoverVelarPhevBattery::transmit_can(unsigned long currentMillis) {
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {
    previousMillis50ms = currentMillis;

    //transmit_can_frame(&VELAR_18B);
  }
}

void LandRoverVelarPhevBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 72;
  datalayer.battery.info.total_capacity_Wh = 17000;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
