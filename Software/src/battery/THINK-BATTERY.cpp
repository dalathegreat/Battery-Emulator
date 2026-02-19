#include "THINK-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void ThinkBattery::update_values() {

  datalayer.battery.status.real_soc = (1000 - sys_dod) * 10;

  datalayer.battery.status.soh_pptt = 9900;

  datalayer.battery.status.voltage_dV = sys_voltage;

  datalayer.battery.status.current_dA = sys_current;

  datalayer.battery.status.max_charge_power_W = (sys_voltage * sys_currentMaxCharge) / 100;

  datalayer.battery.status.max_discharge_power_W = (sys_voltage * sys_currentMaxDischarge) / 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.temperature_min_dC = (int16_t)(min_pack_temperature * 10);

  datalayer.battery.status.temperature_max_dC = (int16_t)(max_pack_temperature * 10);

  if (max_cellvoltage > 2800) {  //Value is low when unavailable
    datalayer.battery.status.cell_max_voltage_mV = max_cellvoltage;
  }

  if (min_cellvoltage > 2800) {  //Value is low when unavailable
    datalayer.battery.status.cell_min_voltage_mV = min_cellvoltage;
  }
}

void ThinkBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x300:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //BMSSoftwareTag = rx_frame.data.u8[0] << 16 | rx_frame.data.u8[1] << 8 | rx_frame.data.u8[2];
      //BMSHardwareTag = rx_frame.data.u8[5];
      //BMI_Init_Successful = rx_frame.data.u8[6];
      break;
    case 0x301:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sys_current = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      sys_voltage = rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3];
      sys_dod = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
      sys_tempMean = rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7];
      break;
    case 0x302:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sys_errGeneral = (rx_frame.data.u8[0] & 0x01);
      sys_isolationError = (rx_frame.data.u8[2] & 0x01);
      sys_voltageMinDischarge = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
      sys_currentMaxDischarge = rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7];
      break;
    case 0x303:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sys_voltageMaxCharge = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      sys_currentMaxCharge = rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3];
      //Lots of error flags here, TODO, read them all!
      break;
    case 0x304:  //Status message
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x305:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      Battery_Type = rx_frame.data.u8[3] >> 5;
      sys_ZebraTempError = (rx_frame.data.u8[6] & 0x18) >> 3;
      break;
    case 0x306:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //EquivalentInternalResistance = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      //RelativeEnergyContent = rx_frame.data.u8[2];
      //BatteryLinearAge = rx_frame.data.u8[3] << 8 | rx_frame.data.u8[4];
      break;
    case 0x307:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sys_numberFailedCells = rx_frame.data.u8[7];
      break;
    case 0x308:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //sys_ratedCycles = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      //sys_cumOperatingTime = rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]; //Days
      //sys_numberOfEOC = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
      //sys_numberOfRegenBreaking = rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7];
      break;
    case 0x309:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //sys_numberSOC_20 = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      //sys_numberSOC_10 = rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3];
      //sys_numberSOC_3 = rx_frame.data.u8[4] << 8 | rx_frame.data.u8[5];
      //sys_minimumISOResistance = rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7];
      break;
    case 0x30A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //sys_cumCapacity270_285 = rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1];
      //sys_cumCapacity285_310 = rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3];
      //sys_ignition = rx_frame.data.u8[4] & 0x01;
      BatterySOC = rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7];
      break;
    case 0x30E:  //Battery serial number 1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x30F:  //Battery serial number 2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x610:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_cellvoltage = ((rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1]) * 2.44141);
      min_cellvoltage = ((rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]) * 2.44141);
      max_pack_temperature = (int8_t)rx_frame.data.u8[4];
      min_pack_temperature = (int8_t)rx_frame.data.u8[5];
      break;
    case 0x642:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x644:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x646:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void ThinkBattery::transmit_can(unsigned long currentMillis) {
  //Send 200ms messages
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    if (datalayer.battery.status.bms_status != FAULT) {
      transmit_can_frame(&PCU_310);
      transmit_can_frame(&PCU_311);
    }
  }
}

void ThinkBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
