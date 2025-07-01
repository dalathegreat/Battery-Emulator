#include "RJXZS-BMS.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

void RjxzsBms::update_values() {

  datalayer.battery.status.real_soc = battery_capacity_percentage * 100;
  if (battery_capacity_percentage == 0) {
    //SOC% not available. Raise warning event if we go too long without SOC
    timespent_without_soc++;
    if (timespent_without_soc > FIVE_MINUTES) {
      set_event(EVENT_SOC_UNAVAILABLE, 0);
    }
  } else {  //SOC is available, stop counting and clear error
    timespent_without_soc = 0;
    clear_event(EVENT_SOC_UNAVAILABLE);
  }

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt;  // This BMS does not have a SOH% formula

  datalayer.battery.status.voltage_dV = total_voltage;

  if (charging_active) {
    datalayer.battery.status.current_dA = total_current;
  } else if (discharging_active) {
    datalayer.battery.status.current_dA = -total_current;
  } else {  //No direction data. Should never occur, but send current as charging, better than nothing
    datalayer.battery.status.current_dA = total_current;
  }

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        MAX_CHARGE_POWER_ALLOWED_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_ALLOWED_W;
  }

  // Discharge power is also set in .h file
  datalayer.battery.status.max_discharge_power_W = MAX_DISCHARGE_POWER_ALLOWED_W;

  uint16_t temperatures[] = {
      module_1_temperature,  module_2_temperature,  module_3_temperature,  module_4_temperature,
      module_5_temperature,  module_6_temperature,  module_7_temperature,  module_8_temperature,
      module_9_temperature,  module_10_temperature, module_11_temperature, module_12_temperature,
      module_13_temperature, module_14_temperature, module_15_temperature, module_16_temperature};

  uint16_t min_temp = std::numeric_limits<uint16_t>::max();
  uint16_t max_temp = 0;

  // Loop through the array to find min and max temperatures, ignoring 0 values
  for (int i = 0; i < 16; i++) {
    if (temperatures[i] != 0) {  // Ignore unavailable temperatures
      if (temperatures[i] < min_temp) {
        min_temp = temperatures[i];
      }
      if (temperatures[i] > max_temp) {
        max_temp = temperatures[i];
      }
    }
  }

  datalayer.battery.status.temperature_min_dC = min_temp;

  datalayer.battery.status.temperature_max_dC = max_temp;

  // The cellvoltages[] array can contain 0s inside it
  populated_cellvoltages = 0;
  for (int i = 0; i < MAX_AMOUNT_CELLS; ++i) {
    if (cellvoltages[i] > 0) {  // We have a measurement available
      datalayer.battery.status.cell_voltages_mV[populated_cellvoltages] = cellvoltages[i];
      populated_cellvoltages++;
    }
  }

  datalayer.battery.info.number_of_cells = populated_cellvoltages;  // 1-192S

  datalayer.battery.info.max_design_voltage_dV;  // Set according to cells?

  datalayer.battery.info.min_design_voltage_dV;  // Set according to cells?

  datalayer.battery.status.cell_max_voltage_mV = maximum_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = minimum_cell_voltage;
}

void RjxzsBms::handle_incoming_can_frame(CAN_frame rx_frame) {

  /*
  // All CAN messages recieved will be logged via serial
  logging.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  logging.print("  ");
  logging.print(rx_frame.ID, HEX);
  logging.print("  ");
  logging.print(rx_frame.DLC);
  logging.print("  ");
  for (int i = 0; i < rx_frame.DLC; ++i) {
    logging.print(rx_frame.data.u8[i], HEX);
    logging.print(" ");
  }
  logging.println("");
  */
  switch (rx_frame.ID) {
    case 0xF5:                 // This is the only message is sent from BMS
      setup_completed = true;  // Let the function know we no longer need to send startup messages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];

      if (mux == 0x01) {  // discharge protection voltage, protective current, battery pack capacity
        discharge_protection_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        protective_current = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        battery_pack_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x02) {  // Number of strings, charge protection voltage, protection temperature
        number_of_battery_strings = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        charging_protection_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        protection_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x03) {  // Total voltage/current/power
        total_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        total_current = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        total_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x04) {  // Capacity, percentages
        battery_usage_capacity = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        battery_capacity_percentage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        charging_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x05) {  //Recovery / capacity
        charging_recovery_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        discharging_recovery_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        remaining_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x06) {  // temperature, equalization
        host_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        status_accounting = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        equalization_starting_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        if ((rx_frame.data.u8[4] & 0x40) >> 6) {
          charging_active = true;
          discharging_active = false;
        } else {
          charging_active = false;
          discharging_active = true;
        }
      } else if (mux == 0x07) {  // Cellvoltages 1-3
        cellvoltages[0] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[1] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[2] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x08) {  // Cellvoltages 4-6
        cellvoltages[3] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[4] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[5] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x09) {  // Cellvoltages 7-9
        cellvoltages[6] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[7] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[8] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0A) {  // Cellvoltages 10-12
        cellvoltages[9] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[10] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[11] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0B) {  // Cellvoltages 13-15
        cellvoltages[12] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[13] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[14] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0C) {  // Cellvoltages 16-18
        cellvoltages[15] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[16] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[17] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0D) {  // Cellvoltages 19-21
        cellvoltages[18] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[19] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[20] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0E) {  // Cellvoltages 22-24
        cellvoltages[21] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[22] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[23] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x0F) {  // Cellvoltages 25-27
        cellvoltages[24] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[25] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[26] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x10) {  // Cellvoltages 28-30
        cellvoltages[27] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[28] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[29] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x11) {  // Cellvoltages 31-33
        cellvoltages[30] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[31] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[32] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x12) {  // Cellvoltages 34-36
        cellvoltages[33] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[34] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[35] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x13) {  // Cellvoltages 37-39
        cellvoltages[36] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[37] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[38] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x14) {  // Cellvoltages 40-42
        cellvoltages[39] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[40] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[41] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x15) {  // Cellvoltages 43-45
        cellvoltages[42] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[43] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[44] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x16) {  // Cellvoltages 46-48
        cellvoltages[45] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[46] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[47] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x17) {  // Cellvoltages 49-51
        cellvoltages[48] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[49] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[50] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x18) {  // Cellvoltages 52-54
        cellvoltages[51] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[52] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[53] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x19) {  // Cellvoltages 55-57
        cellvoltages[54] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[55] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[56] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1A) {  // Cellvoltages 58-60
        cellvoltages[57] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[58] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[59] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1B) {  // Cellvoltages 61-63
        cellvoltages[60] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[61] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[62] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1C) {  // Cellvoltages 64-66
        cellvoltages[63] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[64] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[65] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1D) {  // Cellvoltages 67-69
        cellvoltages[66] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[67] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[68] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1E) {  // Cellvoltages 70-72
        cellvoltages[69] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[70] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[71] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x1F) {  // Cellvoltages 73-75
        cellvoltages[72] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[73] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[74] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x20) {  // Cellvoltages 76-78
        cellvoltages[75] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[76] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[77] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x21) {  // Cellvoltages 79-81
        cellvoltages[78] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[79] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[80] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x22) {  // Cellvoltages 82-84
        cellvoltages[81] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[82] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[83] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x23) {  // Cellvoltages 85-87
        cellvoltages[84] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[85] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[86] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x24) {  // Cellvoltages 88-90
        cellvoltages[87] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[88] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[89] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x25) {  // Cellvoltages 91-93
        cellvoltages[90] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[91] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[92] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x26) {  // Cellvoltages 94-96
        cellvoltages[93] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[94] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[95] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x27) {  // Cellvoltages 97-99
        cellvoltages[96] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[97] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[98] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x28) {  // Cellvoltages 100-102
        cellvoltages[99] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[100] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[101] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x29) {  // Cellvoltages 103-105
        cellvoltages[102] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[103] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[104] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2A) {  // Cellvoltages 106-108
        cellvoltages[105] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[106] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[107] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2B) {  // Cellvoltages 109-111
        cellvoltages[108] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[109] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[110] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2C) {  // Cellvoltages 112-114
        cellvoltages[111] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[112] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[113] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2D) {  // Cellvoltages 115-117
        cellvoltages[114] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[115] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[116] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2E) {  // Cellvoltages 118-120
        cellvoltages[117] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[118] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[119] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x2F) {  // Cellvoltages 121-123
        cellvoltages[120] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[121] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[122] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x30) {  // Cellvoltages 124-126
        cellvoltages[123] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[124] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[125] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x31) {  // Cellvoltages 127-129
        cellvoltages[126] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[127] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[128] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x32) {  // Cellvoltages 130-132
        cellvoltages[129] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[130] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[131] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x33) {  // Cellvoltages 133-135
        cellvoltages[132] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[133] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[134] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x34) {  // Cellvoltages 136-138
        cellvoltages[135] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[136] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[137] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x35) {  // Cellvoltages 139-141
        cellvoltages[138] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[139] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[140] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x36) {  // Cellvoltages 142-144
        cellvoltages[141] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[142] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[143] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x37) {  // Cellvoltages 145-147
        cellvoltages[144] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[145] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[146] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x38) {  // Cellvoltages 148-150
        cellvoltages[147] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[148] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[149] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x39) {  // Cellvoltages 151-153
        cellvoltages[150] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[151] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[152] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3A) {  // Cellvoltages 154-156
        cellvoltages[153] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[154] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[155] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3B) {  // Cellvoltages 157-159
        cellvoltages[156] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[157] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[158] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3C) {  // Cellvoltages 160-162
        cellvoltages[159] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[160] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[161] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3D) {  // Cellvoltages 163-165
        cellvoltages[162] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[163] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[164] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3E) {  // Cellvoltages 166-167
        cellvoltages[165] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[166] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[167] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x3F) {  // Cellvoltages 169-171
        cellvoltages[168] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[169] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[170] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x40) {  // Cellvoltages 172-174
        cellvoltages[171] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[172] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[173] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x41) {  // Cellvoltages 175-177
        cellvoltages[174] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[175] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[176] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x42) {  // Cellvoltages 178-180
        cellvoltages[177] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[178] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[179] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x43) {  // Cellvoltages 181-183
        cellvoltages[180] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[181] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[182] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x44) {  // Cellvoltages 184-186
        cellvoltages[183] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[184] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[185] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x45) {  // Cellvoltages 187-189
        cellvoltages[186] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[187] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[188] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x46) {  // Cellvoltages 190-192
        cellvoltages[189] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        cellvoltages[190] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        cellvoltages[191] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x47) {
        temperature_below_zero_mod1_4 = rx_frame.data.u8[2];
        module_1_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x48) {
        module_2_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_3_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x49) {
        module_4_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4A) {
        temperature_below_zero_mod5_8 = rx_frame.data.u8[2];
        module_5_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4B) {
        module_6_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_7_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4C) {
        module_8_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4D) {
        temperature_below_zero_mod9_12 = rx_frame.data.u8[2];
        module_9_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_10_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4E) {
        module_11_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_12_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_13_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4F) {
        module_14_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_15_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_16_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x50) {
        low_voltage_power_outage_protection = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        low_voltage_power_outage_delayed = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        num_of_triggering_protection_cells = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x51) {
        balanced_reference_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        minimum_cell_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        maximum_cell_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x52) {
        accumulated_total_capacity_high = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        accumulated_total_capacity_low = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        pre_charge_delay_time = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        LCD_status = rx_frame.data.u8[7];
      } else if (mux == 0x53) {
        differential_pressure_setting_value = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        use_capacity_to_automatically_reset = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        low_temperature_protection_setting_value = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        protecting_historical_logs = rx_frame.data.u8[7];

        if ((protecting_historical_logs & 0x0F) > 0) {
          set_event(EVENT_RJXZS_LOG, 0);
        } else {
          clear_event(EVENT_RJXZS_LOG);
        }

        if (protecting_historical_logs == 0x01) {
          // Overcurrent protection
          set_event(EVENT_DISCHARGE_LIMIT_EXCEEDED, 0);  // could also be EVENT_CHARGE_LIMIT_EXCEEDED
        } else if (protecting_historical_logs == 0x02) {
          // over discharge protection
          set_event(EVENT_BATTERY_UNDERVOLTAGE, 0);
        } else if (protecting_historical_logs == 0x03) {
          // overcharge protection
          set_event(EVENT_BATTERY_OVERVOLTAGE, 0);
        } else if (protecting_historical_logs == 0x04) {
          // Over temperature protection
          set_event(EVENT_BATTERY_OVERHEAT, 0);
        } else if (protecting_historical_logs == 0x05) {
          // Battery string error protection
          set_event(EVENT_BATTERY_CAUTION, 0);
        } else if (protecting_historical_logs == 0x06) {
          // Damaged charging relay
          set_event(EVENT_BATTERY_CHG_STOP_REQ, 0);
        } else if (protecting_historical_logs == 0x07) {
          // Damaged discharge relay
          set_event(EVENT_BATTERY_DISCHG_STOP_REQ, 0);
        } else if (protecting_historical_logs == 0x08) {
          // Low voltage power outage protection
          set_event(EVENT_12V_LOW, 0);
        } else if (protecting_historical_logs == 0x09) {
          // Voltage difference protection
          set_event(EVENT_VOLTAGE_DIFFERENCE, differential_pressure_setting_value);
        } else if (protecting_historical_logs == 0x0A) {
          // Low temperature protection
          set_event(EVENT_BATTERY_FROZEN, low_temperature_protection_setting_value);
        }
      } else if (mux == 0x54) {
        hall_sensor_type = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        fan_start_setting_value = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        ptc_heating_start_setting_value = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        default_channel_state = rx_frame.data.u8[7];
      }
      break;
    default:
      break;
  }
}

void RjxzsBms::transmit_can(unsigned long currentMillis) {
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    if (datalayer.battery.status.bms_status == FAULT) {
      // Incase we loose BMS comms, resend CAN start
      setup_completed = false;
    }

    if (!setup_completed) {
      transmit_can_frame(&RJXZS_10, can_config.battery);  // Communication connected flag
      transmit_can_frame(&RJXZS_1C, can_config.battery);  // CAN OK
    }
  }
}

void RjxzsBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
