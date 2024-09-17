#include "../include.h"
#ifdef RENAULT_ZOE_GEN1_BATTERY
#include <algorithm>  // For std::min and std::max
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RENAULT-ZOE-GEN1-BATTERY.h"

/* Information in this file is based of the OVMS V3 vehicle_renaultzoe.cpp component 
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe/src/vehicle_renaultzoe.cpp
The Zoe BMS apparently does not send total pack voltage, so we use the polled 96x cellvoltages summed up as total voltage
Still TODO:
- Find max discharge and max charge values (for now hardcoded to 5kW)
- Find current sensor value (OVMS code reads this from inverter, which we dont have)
- Figure out why SOH% is not read (low prio)
/*

/* Do not change code below unless you are sure what you are doing */
static uint16_t LB_SOC = 50;
static uint16_t LB_SOH = 99;
static int16_t LB_Average_Temperature = 0;
static uint32_t LB_Charge_Power_W = 0;
static int32_t LB_Current = 0;
static uint16_t LB_kWh_Remaining = 0;
static uint16_t LB_Battery_Voltage = 3700;
static uint8_t frame0 = 0;
static uint8_t current_poll = 0;
static uint8_t requested_poll = 0;
static uint8_t group = 0;
static uint16_t cellvoltages[96];
static uint32_t calculated_total_pack_voltage_mV = 370000;
static uint8_t highbyte_cell_next_frame = 0;
static uint16_t SOC_polled = 50;
static int16_t cell_1_temperature_polled = 0;
static int16_t cell_2_temperature_polled = 0;
static int16_t cell_3_temperature_polled = 0;
static int16_t cell_4_temperature_polled = 0;
static int16_t cell_5_temperature_polled = 0;
static int16_t cell_6_temperature_polled = 0;
static int16_t cell_7_temperature_polled = 0;
static int16_t cell_8_temperature_polled = 0;
static int16_t cell_9_temperature_polled = 0;
static int16_t cell_10_temperature_polled = 0;
static int16_t cell_11_temperature_polled = 0;
static int16_t cell_12_temperature_polled = 0;
static uint16_t battery_mileage_in_km = 0;
static uint16_t kWh_from_beginning_of_battery_life = 0;
static bool looping_over_20 = false;

CAN_frame ZOE_423 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x423,
                     .data = {0x07, 0x1d, 0x00, 0x02, 0x5d, 0x80, 0x5d, 0xc8}};
CAN_frame ZOE_POLL_79B = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x79B,
                          .data = {0x02, 0x21, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame ZOE_ACK_79B = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x79B,
                         .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

#define GROUP1_CELLVOLTAGES_1_POLL 0x41
#define GROUP2_CELLVOLTAGES_2_POLL 0x42
#define GROUP3_METRICS 0x61
#define GROUP4_SOC 0x03
#define GROUP5_TEMPERATURE_POLL 0x04

static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis250 = 0;  // will store last time a 250ms CAN Message was sent
static uint8_t counter_423 = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt = (LB_SOH * 100);  // Increase range from 99% -> 99.00%

  datalayer.battery.status.real_soc = SOC_polled;

  datalayer.battery.status.current_dA = LB_Current;  //TODO: Take from CAN

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = 5000;  //TODO: Take from CAN

  datalayer.battery.status.max_charge_power_W = 5000;  //TODO: Take from CAN

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  int16_t temperatures[] = {cell_1_temperature_polled,  cell_2_temperature_polled,  cell_3_temperature_polled,
                            cell_4_temperature_polled,  cell_5_temperature_polled,  cell_6_temperature_polled,
                            cell_7_temperature_polled,  cell_8_temperature_polled,  cell_9_temperature_polled,
                            cell_10_temperature_polled, cell_11_temperature_polled, cell_12_temperature_polled};

  // Find the minimum and maximum temperatures
  int16_t min_temperature = *std::min_element(temperatures, temperatures + 12);
  int16_t max_temperature = *std::max_element(temperatures, temperatures + 12);

  datalayer.battery.status.temperature_min_dC = min_temperature * 10;

  datalayer.battery.status.temperature_max_dC = max_temperature * 10;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, 96 * sizeof(uint16_t));

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  calculated_total_pack_voltage_mV = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < datalayer.battery.info.number_of_cells; ++i) {
    uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
    calculated_total_pack_voltage_mV += voltage_mV;
    if (voltage_mV != 0) {  // Skip unread values (0)
      min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
      max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
    }
  }
  // If all array values are 0, reset min/max to 3700
  if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
    min_cell_mv_value = 3700;
    max_cell_mv_value = 3700;
    calculated_total_pack_voltage_mV = 370000;
  }

  datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
  datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;
  datalayer.battery.status.voltage_dV =
      static_cast<uint32_t>((calculated_total_pack_voltage_mV / 100));  // Convert from mV to dV

  if (datalayer.battery.status.cell_max_voltage_mV >= ABSOLUTE_CELL_MAX_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (datalayer.battery.status.cell_min_voltage_mV <= ABSOLUTE_CELL_MIN_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x427:
      LB_Charge_Power_W = rx_frame.data.u8[5] * 300;
      LB_kWh_Remaining = (((((rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7])) >> 6) & 0x3ff) * 0.1);
      break;
    case 0x42E:  //HV SOC & Battery Temp & Charging Power
      LB_Battery_Voltage = (((((rx_frame.data.u8[3] << 8) | (rx_frame.data.u8[4])) >> 5) & 0x3ff) * 0.5);  //0.5V/bit
      LB_Average_Temperature = (((((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6])) >> 5) & 0x7F) - 40);
      break;
    case 0x654:  //SOC
      LB_SOC = rx_frame.data.u8[3];
      break;
    case 0x658:  //SOH
      LB_SOH = (rx_frame.data.u8[4] & 0x7F);
      break;
    case 0x7BB:  //Reply from active polling
      frame0 = rx_frame.data.u8[0];

      switch (frame0) {
        case 0x10:  //PID HEADER, datarow 0
          requested_poll = rx_frame.data.u8[3];
          transmit_can(&ZOE_ACK_79B, can_config.battery);

          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[0] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[1] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[62] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[63] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP3_METRICS) {
            //10,14,61,61,00,0A,8C,00,
          }
          if (requested_poll == GROUP4_SOC) {
            //10,1D,61,03,01,94,1F,85, (1F85 = 8069 real SOC?)
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //10,4D,61,04,09,12,3A,09,
            cell_1_temperature_polled = (rx_frame.data.u8[6] - 40);
          }
          break;
        case 0x21:  //First datarow in PID group
          if ((requested_poll == GROUP1_CELLVOLTAGES_1_POLL) && (looping_over_20 == false)) {
            cellvoltages[2] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[3] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[4] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if ((requested_poll == GROUP1_CELLVOLTAGES_1_POLL) && (looping_over_20 == true)) {
            cellvoltages[58] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[59] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[60] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[64] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[65] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[66] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP3_METRICS) {
            //21,C8,C8,C8,C0,C0,00,00,
          }
          if (requested_poll == GROUP4_SOC) {
            //21,21,32,00,00,00,00,01, (2132 = 8498 dash SOC?)
            SOC_polled = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            cell_2_temperature_polled = (rx_frame.data.u8[2] - 40);
            cell_3_temperature_polled = (rx_frame.data.u8[5] - 40);
            //21,11,3A,09,14,3A,09,0D,
          }
          break;
        case 0x22:  //Second datarow in PID group
          if ((requested_poll == GROUP1_CELLVOLTAGES_1_POLL) && (looping_over_20 == false)) {
            cellvoltages[5] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[6] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[7] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[8] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if ((requested_poll == GROUP1_CELLVOLTAGES_1_POLL) && (looping_over_20 == true)) {
            cellvoltages[61] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            looping_over_20 = false;
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[67] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[68] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[69] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[70] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP3_METRICS) {
            //22,BB,7C,00,00,23,E4,FF, (BB7C = 47996km) (23E4 = 9188kWh)
            battery_mileage_in_km = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            kWh_from_beginning_of_battery_life = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          }
          if (requested_poll == GROUP4_SOC) {
            //22,95,01,93,00,00,00,FF,
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            cell_4_temperature_polled = (rx_frame.data.u8[1] - 40);
            cell_5_temperature_polled = (rx_frame.data.u8[4] - 40);
            cell_6_temperature_polled = (rx_frame.data.u8[7] - 40);
            //22,3A,08,F6,3B,08,EE,3B,
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[9] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[10] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[11] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[71] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[72] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[73] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP4_SOC) {
            //23,FF,FF,FF,01,20,7B,00,
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //23,08,AC,3D,08,C0,3C,09,
            cell_7_temperature_polled = (rx_frame.data.u8[3] - 40);
            cell_8_temperature_polled = (rx_frame.data.u8[6] - 40);
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[12] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[13] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[14] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[15] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[74] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[75] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[76] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[77] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP4_SOC) {
            //24,00,00,00,00,00,00,00,
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //24,03,3A,09,11,3A,09,19,
            cell_9_temperature_polled = (rx_frame.data.u8[2] - 40);
            cell_10_temperature_polled = (rx_frame.data.u8[5] - 40);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[16] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[17] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[18] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[78] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[79] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[80] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //25,3A,09,14,3A,FF,FF,FF,
            cell_11_temperature_polled = (rx_frame.data.u8[1] - 40);
            cell_12_temperature_polled = (rx_frame.data.u8[4] - 40);
          }
          break;
        case 0x26:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[19] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[20] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[21] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[22] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[81] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[82] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[83] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[84] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //26,FF,FF,FF,FF,FF,FF,FF,G
          }
          break;
        case 0x27:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[23] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[24] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[25] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[85] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[86] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[87] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //27,FF,FF,FF,FF,FF,FF,FF,
          }
          break;
        case 0x28:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[26] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[27] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[28] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[29] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[88] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[89] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[90] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[91] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //28,FF,FF,FF,FF,FF,FF,FF,
          }
          break;
        case 0x29:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[30] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[21] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[32] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[92] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[93] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[94] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //29,FF,FF,FF,FF,FF,FF,FF,
          }
          break;
        case 0x2A:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[33] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[34] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[35] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[36] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP2_CELLVOLTAGES_2_POLL) {
            cellvoltages[95] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //2A,FF,FF,FF,FF,FF,3A,3A,
          }
          break;
        case 0x2B:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[37] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[38] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[39] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          if (requested_poll == GROUP5_TEMPERATURE_POLL) {
            //2B,3D,00,00,00,00,00,00,
          }
          break;
        case 0x2C:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[40] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[41] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[42] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[43] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          break;
        case 0x2D:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[44] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[45] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[46] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          break;
        case 0x2E:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[47] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[48] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[49] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[50] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          }
          break;
        case 0x2F:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[51] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
            cellvoltages[52] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
            cellvoltages[53] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
            highbyte_cell_next_frame = rx_frame.data.u8[7];
          }
          break;
        case 0x20:
          if (requested_poll == GROUP1_CELLVOLTAGES_1_POLL) {
            cellvoltages[54] = (highbyte_cell_next_frame << 8) | rx_frame.data.u8[1];
            cellvoltages[55] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
            cellvoltages[56] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            cellvoltages[57] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
            looping_over_20 = true;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis100 >= INTERVAL_100_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis100));
    }
    previousMillis100 = currentMillis;
    transmit_can(&ZOE_423, can_config.battery);

    if ((counter_423 / 5) % 2 == 0) {  // Alternate every 5 messages between these two
      ZOE_423.data.u8[4] = 0xB2;
      ZOE_423.data.u8[6] = 0xB2;
    } else {
      ZOE_423.data.u8[4] = 0x5D;
      ZOE_423.data.u8[6] = 0x5D;
    }
    counter_423 = (counter_423 + 1) % 10;
  }
  // 250ms CAN handling
  if (currentMillis - previousMillis250 >= INTERVAL_250_MS) {
    previousMillis250 = currentMillis;

    switch (group) {
      case 0:
        current_poll = GROUP1_CELLVOLTAGES_1_POLL;
        break;
      case 1:
        current_poll = GROUP2_CELLVOLTAGES_2_POLL;
        break;
      case 2:
        current_poll = GROUP3_METRICS;
        break;
      case 3:
        current_poll = GROUP4_SOC;
        break;
      case 4:
        current_poll = GROUP5_TEMPERATURE_POLL;
        break;
      default:
        break;
    }

    group = (group + 1) % 5;  // Cycle 0-1-2-3-4-0-1...

    ZOE_POLL_79B.data.u8[2] = current_poll;

    transmit_can(&ZOE_POLL_79B, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Renault Zoe 22/40kWh battery selected");
#endif
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = 4040;
  datalayer.battery.info.min_design_voltage_dV = 2700;
}

#endif
