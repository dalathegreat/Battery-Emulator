#include "ENNOID-BMS.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void EnnoidBms::update_values() {

  datalayer.battery.status.real_soc = SOC;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = SOH;

  datalayer.battery.status.voltage_dV = packVoltage;

  datalayer.battery.status.current_dA = (int16_t)packCurrent1;

  // Charge power is manually set
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        datalayer.battery.status.override_charge_power_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  // Discharge power is manually set
  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  datalayer.battery.status.temperature_min_dC = tBattHi;

  datalayer.battery.status.temperature_max_dC = tBattHi - 1;

  datalayer.battery.info.number_of_cells = NoOfCells;  // 1-192S

  //datalayer.battery.info.max_design_voltage_dV;  // TODO: Set according to cells?

  //datalayer.battery.info.min_design_voltage_dV;  // TODO: Set according to cells?

  datalayer.battery.status.cell_max_voltage_mV = cellVoltageHigh;

  datalayer.battery.status.cell_min_voltage_mV = cellVoltageLow;
}

void EnnoidBms::handle_incoming_can_frame(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case 0x2b0a:
      NoOfCells = rx_frame.data.u8[1];
      break;
    case 0x260a:
      packVoltage =
          (rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    case 0x270a:
      packCurrent1 =
          (rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      packCurrent2 =
          (rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x280a:
      //Ah_counter = (rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      //Wh_counter = (rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x290a:
      //Most likely contains all cellvoltages, but DBC was not clear on how this is muxed. CAN log needed!
      break;
    case 0x2a0a:
      //Contains balancing shunt status, but DBC was not clear on how this is muxed. CAN log needed!
      break;
    case 0x2d0a:
      cellVoltageLow = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellVoltageHigh = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      SOC = rx_frame.data.u8[4] * 40;
      SOH = rx_frame.data.u8[5] * 40;
      tBattHi = rx_frame.data.u8[6];
      BitF = rx_frame.data.u8[7];
      break;
    default:
      break;
  }
}

void EnnoidBms::transmit_can(unsigned long currentMillis) {
  //No periodic sending for this protocol
}

void EnnoidBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
