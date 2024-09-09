#include "../include.h"
#ifdef RJXZS_BMS
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RJXZS-BMS.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10s = 0;  // will store last time a 10s CAN Message was sent

//Actual content messages
CAN_frame RJXZS_1C = {.FD = false, .ext_ID = true, .DLC = 3, .ID = 0xF4, .data = {0x1C, 0x00, 0x02}};
CAN_frame RJXZS_10 = {.FD = false, .ext_ID = true, .DLC = 3, .ID = 0xF4, .data = {0x10, 0x00, 0x02}};

static uint8_t mux = 0;
static bool setup_completed = false;
static uint16_t total_voltage = 0;
static int16_t total_current = 0;
static uint16_t total_power = 0;
static uint16_t battery_usage_capacity = 0;
static uint16_t battery_capacity_percentage = 0;
static uint16_t charging_capacity = 0;
static uint16_t charging_recovery_voltage = 0;
static uint16_t discharging_recovery_voltage = 0;
static uint16_t remaining_capacity = 0;
static int16_t host_temperature = 0;
static uint16_t status_accounting = 0;
static uint16_t equalization_starting_voltage = 0;
static uint16_t discharge_protection_voltage = 0;
static uint16_t protective_current = 0;
static uint16_t battery_pack_capacity = 0;
static uint16_t number_of_battery_strings = 0;
static uint16_t charging_protection_voltage = 0;
static int16_t protection_temperature = 0;
static bool temperature_below_zero_mod1_4 = false;
static bool temperature_below_zero_mod5_8 = false;
static uint16_t module_1_temperature = 0;
static uint16_t module_2_temperature = 0;
static uint16_t module_3_temperature = 0;
static uint16_t module_4_temperature = 0;
static uint16_t module_5_temperature = 0;
static uint16_t module_6_temperature = 0;
static uint16_t module_7_temperature = 0;
static uint16_t module_8_temperature = 0;
static uint16_t module_9_temperature = 0;
static uint16_t module_10_temperature = 0;
static uint16_t module_11_temperature = 0;
static uint16_t module_12_temperature = 0;
static uint16_t module_13_temperature = 0;
static uint16_t module_14_temperature = 0;
static uint16_t module_15_temperature = 0;
static uint16_t module_16_temperature = 0;
static uint16_t balanced_reference_voltage = 0;
static uint16_t minimum_cell_voltage = 0;
static uint16_t maximum_cell_voltage = 0;
static uint16_t cellvoltages[MAX_AMOUNT_CELLS];
static uint8_t populated_cellvoltages = 0;

void update_values_battery() {

  datalayer.battery.status.real_soc = battery_capacity_percentage;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = total_voltage;

  datalayer.battery.status.current_dA = total_current;

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.max_charge_power_W = protective_current * total_voltage;

  datalayer.battery.status.max_discharge_power_W = protective_current * total_voltage;

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

void receive_can_battery(CAN_frame rx_frame) {

  // All CAN messages recieved will be logged via serial
  Serial.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  Serial.print("  ");
  Serial.print(rx_frame.ID, HEX);
  Serial.print("  ");
  Serial.print(rx_frame.DLC);
  Serial.print("  ");
  for (int i = 0; i < rx_frame.DLC; ++i) {
    Serial.print(rx_frame.data.u8[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

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
      } else if (mux == 0x51) {
        balanced_reference_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        minimum_cell_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        maximum_cell_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {

    previousMillis10s = currentMillis;

    if (!setup_completed) {
      transmit_can(&RJXZS_10, can_config.battery);  // Communication connected flag
      transmit_can(&RJXZS_1C, can_config.battery);  // CAN OK
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("RJXZS BMS selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 5000;
  datalayer.battery.info.min_design_voltage_dV = 1000;
}

#endif  // RJXZS_BMS
