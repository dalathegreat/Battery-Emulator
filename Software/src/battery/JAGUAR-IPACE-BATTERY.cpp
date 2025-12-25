#include "JAGUAR-IPACE-BATTERY.h"
#include <cstring>  //for unit tests
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void JaguarIpaceBattery::update_values() {

  if (user_selected_use_estimated_SOC) {
    datalayer.battery.status.real_soc =
        (1000 + (datalayer.battery.status.voltage_dV - 4500)) * 10;  //450.0 FULL, 350.0 EMPTY
  } else {
    datalayer.battery.status.real_soc = HVBattAvgSOC * 100;  //Add two decimals
  }

  datalayer.battery.status.soh_pptt = 9900;  //TODO: Map

  datalayer.battery.status.voltage_dV = HVBattVoltageExt * 10;

  datalayer.battery.status.current_dA = HVBattCurrentTR * 10;  //TODO: This value OK?

  datalayer.battery.info.total_capacity_Wh = HVBattEnergyUsableMax * 100;  // kWh+1 to Wh

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = HVBattCellVoltageMaxMv;

  datalayer.battery.status.cell_min_voltage_mV = HVBattCellVoltageMinMv;

  datalayer.battery.status.temperature_min_dC = HVBattCellTempColdest * 10;  // C to dC

  datalayer.battery.status.temperature_max_dC = HVBattCellTempHottest * 10;  // C to dC

  datalayer.battery.status.max_discharge_power_W =
      HVBattDischargeContiniousPowerLimit * 10;  // kWh+2 to W (TODO: Check that scaling is right way)

  datalayer.battery.status.max_charge_power_W =
      HVBattChargeContiniousPowerLimit * 10;  // kWh+2 to W (TODO: Check that scaling is right way)

  if (HVBattHVILError) {  // Alert user incase the high voltage interlock is not OK
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  if (HVILBattIsolationError) {  // Alert user incase battery reports isolation error
    set_event(EVENT_BATTERY_ISOLATION, 0);
  } else {
    clear_event(EVENT_BATTERY_ISOLATION);
  }
}

void JaguarIpaceBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  switch (rx_frame.ID) {  // These messages are periodically transmitted by the battery
    case 0x080:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBatteryContactorStatus = ((rx_frame.data.u8[0] & 0x80) >> 7);
      HVBattHVILError = ((rx_frame.data.u8[0] & 0x40) >> 6);
      HVILBattIsolationError = ((rx_frame.data.u8[0] & 0x20) >> 5);
      break;
    case 0x100:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattDischargeContiniousPowerLimit =
          ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);                             // 0x3269 = 12905 = 129.05kW
      HVBattDischargePowerLimitExt = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);  // 0x7BD5 = 31701 = 317.01kW
      HVBattDischargeVoltageLimit =
          ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);  // Lowest voltage the pack can go to
      break;
    case 0x102:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattPwrGpCounter = ((rx_frame.data.u8[1] & 0x3C) >> 2);  // Loops 0-F-0
      HVBattPwerGPCS = rx_frame.data.u8[0];                      // SAE J1850 CRC8 Checksum.
      //TODO: Add function that checks if CRC is correct. We can use this to detect corrupted CAN messages
      //HVBattCurrentExt = //Used only on 2018+
      HVBattVoltageExt = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[2]);
      break;
    case 0x104:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBatteryContactorStatusT = ((rx_frame.data.u8[2] & 0x80) >> 7);
      HVIsolationTestStatus = ((rx_frame.data.u8[2] & 0x10) >> 4);
      HVBatteryVoltageOC = (((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]);
      HVBatteryChgCurrentLimit = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x10A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattChargeContiniousPowerLimit = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      break;
    case 0x198:
      break;
    case 0x1C4:
      HVBattCurrentTR = rx_frame.data.u8[0];  //TODO: scaling?
      //HVBattPwrExtGPCounter = (rx_frame.data.u8[2] & 0xF0) >> 4; // not needed
      //HVBattPwrExtGPCS = rx_frame.data.u8[1]; // Checksum, not needed
      //HVBattVoltageBusTF Frame 3/4/5?
      //HVBattVoltageBusTR Frame 3/4/5?
      break;
    case 0x220:
      break;
    case 0x222:
      HVBattAvgSOC = rx_frame.data.u8[4];
      HVBattAverageTemperature = (rx_frame.data.u8[5] / 2) - 40;
      HVBattFastChgCounter = rx_frame.data.u8[7];
      //HVBattLogEvent
      HVBattTempColdCellID = rx_frame.data.u8[0];
      HVBatTempHotCellID = rx_frame.data.u8[1];
      HVBattVoltMaxCellID = rx_frame.data.u8[2];
      HVBattVoltMinCellID = rx_frame.data.u8[3];
      break;
    case 0x248:
      break;
    case 0x308:
      break;
    case 0x424:
      HVBattCellVoltageMaxMv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      HVBattCellVoltageMinMv = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      //HVBattHeatPowerGenChg // kW
      //HVBattHeatPowerGenDcg // kW
      //HVBattWarmupRateChg // degC/minute
      //HVBattWarmupRateDcg // degC/minute
      break;
    case 0x448:
      HVBattCellTempAverage = (rx_frame.data.u8[0] / 2) - 40;
      HVBattCellTempColdest = (rx_frame.data.u8[1] / 2) - 40;
      HVBattCellTempHottest = (rx_frame.data.u8[2] / 2) - 40;
      //HVBattCIntPumpDiagStat // Pump OK / NOK
      //HVBattCIntPumpDiagStat_UB // True/False
      //HVBattCoolantLevel // Coolant level OK / NOK
      //HVBattHeaterCtrlStat // Off / On
      HVBattInletCoolantTemp = (rx_frame.data.u8[5] / 2) - 40;
      //HVBattInlentCoolantTemp_UB // True/False
      //HVBattMILRequested // True/False
      //HVBattThermalOvercheck // OK / NOK
      break;
    case 0x449:
      break;
    case 0x464:
      HVBattEnergyAvailable =
          ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) / 2;  // 0x0198 = 408 / 2 = 204 = 20.4kWh
      HVBattEnergyUsableMax =
          ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) / 2;  // 0x06EA = 1770 / 2 = 885 = 88.5kWh
      HVBattTotalCapacityWhenNew = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);  //0x0395 = 917 = 91.7kWh
      break;
    case 0x522:
      break;
    default:
      break;
  }
}

void JaguarIpaceBattery::transmit_can(unsigned long currentMillis) {
  /* Send keep-alive every 200ms */
  if (currentMillis - previousMillisKeepAlive >= INTERVAL_200_MS) {
    previousMillisKeepAlive = currentMillis;
    transmit_can_frame(&ipace_keep_alive);
  }
}

void JaguarIpaceBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
