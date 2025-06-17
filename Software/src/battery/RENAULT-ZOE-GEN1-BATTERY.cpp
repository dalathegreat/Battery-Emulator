#include "RENAULT-ZOE-GEN1-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../include.h"

void transmit_can_frame(CAN_frame* tx_frame, int interface);

/* Information in this file is based of the OVMS V3 vehicle_renaultzoe.cpp component 
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe/src/vehicle_renaultzoe.cpp
The Zoe BMS apparently does not send total pack voltage, so we use the polled 96x cellvoltages summed up as total voltage
Still TODO:
- Automatically detect what vehicle and battery size we are on (Zoe 22/41 , Kangoo 33, Fluence ZE 22/36)
/*

/* Do not change code below unless you are sure what you are doing */

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

void RenaultZoeGen1Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer_battery->status.soh_pptt = (LB_SOH * 100);  // Increase range from 99% -> 99.00%

  datalayer_battery->status.real_soc = SOC_polled;
  //datalayer_battery->status.real_soc = LB_Display_SOC; //Alternative would be to use Dash SOC%

  datalayer_battery->status.current_dA = LB_Current * 10;  //Convert A to dA

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = LB_Discharge_allowed_W;

  datalayer_battery->status.max_charge_power_W = LB_Regen_allowed_W;

  int16_t temperatures[] = {cell_1_temperature_polled,  cell_2_temperature_polled,  cell_3_temperature_polled,
                            cell_4_temperature_polled,  cell_5_temperature_polled,  cell_6_temperature_polled,
                            cell_7_temperature_polled,  cell_8_temperature_polled,  cell_9_temperature_polled,
                            cell_10_temperature_polled, cell_11_temperature_polled, cell_12_temperature_polled};

  // Find the minimum and maximum temperatures
  int16_t min_temperature = *std::min_element(temperatures, temperatures + 12);
  int16_t max_temperature = *std::max_element(temperatures, temperatures + 12);

  datalayer_battery->status.temperature_min_dC = min_temperature * 10;

  datalayer_battery->status.temperature_max_dC = max_temperature * 10;

  // Calculate total pack voltage on packs that require this. Only calculate once all cellvotages have been read
  if (datalayer_battery->status.cell_voltages_mV[95] > 0) {
    calculated_total_pack_voltage_mV = datalayer_battery->status.cell_voltages_mV[0];
    for (uint8_t i = 0; i < datalayer_battery->info.number_of_cells; ++i) {
      calculated_total_pack_voltage_mV += datalayer_battery->status.cell_voltages_mV[i];
    }
  }

  datalayer_battery->status.cell_min_voltage_mV = LB_Cell_minimum_voltage;
  datalayer_battery->status.cell_max_voltage_mV = LB_Cell_maximum_voltage;
  datalayer_battery->status.voltage_dV = ((calculated_total_pack_voltage_mV / 100));  // mV to dV

  //Update extended datalayer
  if (datalayer_zoe) {
    datalayer_zoe->CUV = LB_CUV;
    datalayer_zoe->HVBIR = LB_HVBIR;
    datalayer_zoe->HVBUV = LB_HVBUV;
    datalayer_zoe->EOCR = LB_EOCR;
    datalayer_zoe->HVBOC = LB_HVBOC;
    datalayer_zoe->HVBOT = LB_HVBOT;
    datalayer_zoe->HVBOV = LB_HVBOV;
    datalayer_zoe->COV = LB_COV;
    datalayer_zoe->mileage_km = battery_mileage_in_km;
    datalayer_zoe->alltime_kWh = kWh_from_beginning_of_battery_life;
  }
}

void RenaultZoeGen1Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x155:  //10ms - Charging power, current and SOC - Confirmed sent by: Fluence ZE40, Zoe 22/41kWh, Kangoo 33kWh
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_Charging_Power_W = rx_frame.data.u8[0] * 300;
      LB_Current = (((((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2]) * 0.25) - 500);
      LB_Display_SOC = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;

    case 0x42E:  //NOTE: Not present on 41kWh battery!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_Battery_Voltage = (((((rx_frame.data.u8[3] << 8) | (rx_frame.data.u8[4])) >> 5) & 0x3ff) * 0.5);  //0.5V/bit
      LB_Average_Temperature = (((((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6])) >> 5) & 0x7F) - 40);
      break;
    case 0x424:  //100ms - Charge limits, Temperatures, SOH - Confirmed sent by: Fluence ZE40, Zoe 22/41kWh, Kangoo 33kWh
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_CUV = (rx_frame.data.u8[0] & 0x03);
      LB_HVBIR = (rx_frame.data.u8[0] & 0x0C) >> 2;
      LB_HVBUV = (rx_frame.data.u8[0] & 0x30) >> 4;
      LB_EOCR = (rx_frame.data.u8[0] & 0xC0) >> 6;
      LB_HVBOC = (rx_frame.data.u8[1] & 0x03);
      LB_HVBOT = (rx_frame.data.u8[1] & 0x0C) >> 2;
      LB_HVBOV = (rx_frame.data.u8[1] & 0x30) >> 4;
      LB_COV = (rx_frame.data.u8[1] & 0xC0) >> 6;
      LB_Regen_allowed_W = rx_frame.data.u8[2] * 500;
      LB_Discharge_allowed_W = rx_frame.data.u8[3] * 500;
      LB_Cell_minimum_temperature = (rx_frame.data.u8[4] - 40);
      LB_SOH = rx_frame.data.u8[5];
      LB_Heartbeat = rx_frame.data.u8[6];  // Alternates between 0x55 and 0xAA every 500ms (Same as on Nissan LEAF)
      LB_Cell_maximum_temperature = (rx_frame.data.u8[7] - 40);
      break;
    case 0x425:  //100ms Cellvoltages and kWh remaining - Confirmed sent by: Fluence ZE40
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_Cell_maximum_voltage = (((((rx_frame.data.u8[4] & 0x03) << 7) | (rx_frame.data.u8[5] >> 1)) * 10) + 1000);
      LB_Cell_minimum_voltage = (((((rx_frame.data.u8[6] & 0x01) << 8) | rx_frame.data.u8[7]) * 10) + 1000);
      break;
    case 0x427:  // NOTE: Not present on 41kWh battery!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_kWh_Remaining = (((((rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7])) >> 6) & 0x3ff) * 0.1);
      break;
    case 0x445:  //100ms - Confirmed sent by: Fluence ZE40
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4AE:  //3000ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Sent only? by 41kWh battery (potential use for detecting which generation we are on)
      break;
    case 0x4AF:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Sent only? by 41kWh battery (potential use for detecting which generation we are on)
      break;
    case 0x654:  //SOC
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_SOC = rx_frame.data.u8[3];
      break;
    case 0x658:  //SOH - NOTE: Not present on 41kWh battery! (Is this message on 21kWh?)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //LB_SOH = (rx_frame.data.u8[4] & 0x7F);
      break;
    case 0x659:  //3000ms - Confirmed sent by: Fluence ZE40
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7BB:  //Reply from active polling
      frame0 = rx_frame.data.u8[0];

      switch (frame0) {
        case 0x10:  //PID HEADER, datarow 0
          requested_poll = rx_frame.data.u8[3];
          transmit_can_frame(&ZOE_ACK_79B, can_config.battery);

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
            cellvoltages[31] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
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
            //All cells read, map them to the global array
            memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages, 96 * sizeof(uint16_t));
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
            if (cellvoltages[47] < 100) {  //This cell measurement is inbetween pack halves. If low, fuse blown
              set_event(EVENT_BATTERY_FUSE, cellvoltages[47]);
            } else {
              clear_event(EVENT_BATTERY_FUSE);
            }
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

void RenaultZoeGen1Battery::transmit_can(unsigned long currentMillis) {

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    transmit_can_frame(&ZOE_423, can_config.battery);

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

    transmit_can_frame(&ZOE_POLL_79B, can_config.battery);
  }
}

void RenaultZoeGen1Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
