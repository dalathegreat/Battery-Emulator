#include "HYUNDAI-IONIQ-28-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"

void HyundaiIoniq28Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer_battery->status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer_battery->status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer_battery->status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0) , invert the sign

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_charge_power_W = allowedChargePower * 10;

  datalayer_battery->status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer_battery->status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer_battery->status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer_battery->status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer_battery->status.cell_min_voltage_mV = CellVoltMin_mV;

  //Map all cell voltages to the global array
  memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages_mv, 96 * sizeof(uint16_t));

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }
}

void HyundaiIoniq28Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4DE:
      startedUp = true;
      break;
    case 0x542:  //BMS SOC
      startedUp = true;
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      startedUp = true;
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      startedUp = true;
      batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (counter_200 > 3) {
        IONIQ_524.data.u8[0] = (uint8_t)(batteryVoltage / 10);
        IONIQ_524.data.u8[1] = (uint8_t)((batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms
      break;
    case 0x596:
      startedUp = true;
      leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      startedUp = true;
      break;
    case 0x5D5:
      startedUp = true;
      powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      startedUp = true;
      break;
    case 0x7EC:
      //We only poll multiframe messages
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          transmit_can_frame(&IONIQ_7E4_ACK);
          incoming_poll_group = rx_frame.data.u8[3];
          break;
        case 0x21:                         //First frame in PID group
          if (incoming_poll_group == 1) {  //21 14 13 24 13 24 04 00
            //SOC = rx_frame.data.u8[1];
            //available_charge_power = 2 & 3
            //available_discharge_power = 4 & 5
            //status_bits? = 6
            //battery_current_highbyte = rx_frame.data.u8[7];
          } else if (incoming_poll_group == 2) {  //21 AD AD AD AD AD AD AC
            cellvoltages_mv[0] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[6] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 3) {  //21 AD AD AD AD AD AD AD
            cellvoltages_mv[32] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[37] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[38] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 4) {  //21 AD AD AD AD AD AD AD
            cellvoltages_mv[64] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[65] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[66] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[67] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[68] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[69] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[70] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 5) {  //21	0	0	0	0	0	0f0f
            //battery_module_6_temperature = rx_frame.data.u8[6];
            //battery_module_7_temperature = rx_frame.data.u8[7];
          }
          break;
        case 0x22:                         //Second datarow in PID group
          if (incoming_poll_group == 1) {  //22 00 0C FF 17 16 17 17
            //battery_current_lowbyte = rx_frame.data.u8[1];
            //battery_DC_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            //battery_max_temperature = rx_frame.data.u8[4];
            //battery_min_temperature = rx_frame.data.u8[5];
            //battery_module_1_temperature = rx_frame.data.u8[6];
            //battery_module_2_temperature = rx_frame.data.u8[7];
          } else if (incoming_poll_group == 2) {  //22 AD AC AC AD AD AD AD
            cellvoltages_mv[7] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[13] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 3) {  //22 AD AD AD AD AD AD AD
            cellvoltages_mv[39] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[43] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[45] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 4) {  //22 AD AD AD AD AD AD AD
            cellvoltages_mv[71] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[72] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[73] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[74] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[75] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[76] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[77] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 5) {  //22 10 0d 0c 0e 0d 26 48
            //battery_module_8_temperature = rx_frame.data.u8[1];
            //battery_module_9_temperature = rx_frame.data.u8[2];
            //battery_module_10_temperature = rx_frame.data.u8[3];
            //battery_module_11_temperature = rx_frame.data.u8[4];
            //battery_module_12_temperature = rx_frame.data.u8[5];
            //available_charge_power = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          } else if (incoming_poll_group == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:                         //Third datarow in PID group
          if (incoming_poll_group == 1) {  //23 17 17 17 00 17 AD 25
            //battery_module_3_temperature = rx_frame.data.u8[1];
            //battery_module_4_temperature = rx_frame.data.u8[2];
            //battery_module_5_temperature = rx_frame.data.u8[3];
            //battery_inlet_temperature = rx_frame.data.u8[5];
            CellVoltMax_mV = (rx_frame.data.u8[6] * 20);  //(volts *50) *20 =mV
            //cellmaxvoltage_number = rx_frame.data.u8[7];
          } else if (incoming_poll_group == 2) {  //23 AD AD AD AD AB AD AD
            cellvoltages_mv[14] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[20] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 3) {  //23 AD AD AC AD AD AD AD
            cellvoltages_mv[46] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[49] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[52] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 4) {  //23 AA AD AD AD AD AD AC
            cellvoltages_mv[78] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[79] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[80] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[81] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[82] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[83] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[84] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 5) {
            //available_discharge_power = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
            //battery_cell_mV_deviation = rx_frame.data.u8[3];
            //airbag_h/wire_duty = rx_frame.data.u8[5];
            heatertemperature_1 = rx_frame.data.u8[6];
            heatertemperature_2 = rx_frame.data.u8[7];
          }
          break;
        case 0x24:                                        //Fourth datarow in PID group
          if (incoming_poll_group == 1) {                 //24 AA 4F 00 00 77 00 14
            CellVoltMin_mV = (rx_frame.data.u8[1] * 20);  //(volts *50) *20 =mV
            //mincellvoltage_number = rx_frame.data.u8[2];
            //fan_status = rx_frame.data.u8[3];
            //fan_feedback_signal = rx_frame.data.u8[4];
            //aux_battery_voltage = rx_frame.data.u8[5];
            //cumulative_charge_current_highbyte = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          } else if (incoming_poll_group == 2) {  //24 AD AD AD AD AD AD AB
            cellvoltages_mv[21] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[27] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 3) {  //24 AD AD AD AD AC AD AD
            cellvoltages_mv[53] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[55] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[59] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 4) {  //24 AD AC AC AD AC AD AD
            cellvoltages_mv[85] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[86] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[87] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[88] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[89] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[90] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[91] = (rx_frame.data.u8[7] * 20);
          } else if (incoming_poll_group == 5) {  //24	3	e8 5	3	e8 m34 6e
            batterySOH = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
            //max_deterioration_cell_number = rx_frame.data.u8[3]
            //min_deterioration = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
            //min_deterioration_cell_number = rx_frame.data.u8[6]
            //SOC_display = rx_frame.data.u8[7]
          }
          break;
        case 0x25:                         //Fifth datarow in PID group
          if (incoming_poll_group == 1) {  //25 5C A9 00 14 5F D3 00
            //cumulative_charge_current_lowbyte = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
            //cumulative_discharge_current = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
            //cumulative_charge_energy_highbyte = rx_frame.data.u8[7];
          } else if (incoming_poll_group == 2) {  //25 AD AD AD AD 00 00 00
            cellvoltages_mv[28] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[31] = (rx_frame.data.u8[4] * 20);
          } else if (incoming_poll_group == 3) {  //25 AD AD AD AD 00 00 00
            cellvoltages_mv[60] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[61] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[62] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[63] = (rx_frame.data.u8[4] * 20);
          } else if (incoming_poll_group == 4) {  //25 AD AD AD AD 00 00 00
            cellvoltages_mv[92] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[93] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[94] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[95] = (rx_frame.data.u8[4] * 20);
          } else if (incoming_poll_group == 5) {
          }
          break;
        case 0x26:                         //Sixth datarow in PID group
          if (incoming_poll_group == 1) {  //26 07 84 F9 00 07 42 8F
            //cumulative_charge_energy_lowbyte = (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
            //cumulative_discharge_energy = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);

          } else if (incoming_poll_group == 5) {
          }
          break;
        case 0x27:                         //Seventh datarow in PID group
          if (incoming_poll_group == 1) {  //27 03 3F A1 EB 00 19 99
            //cumulative_operating_time = ((rx_frame.data.u8[1] << 24) | (rx_frame.data.u8[2] << 16) | (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
            //bitfield (41 off, 45 car on) = rx_frame.data.u8[5];
            inverterVoltage = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          }
          break;
        case 0x28:                         //Eighth datarow in PID group
          if (incoming_poll_group == 1) {  //28 7F FF 7F FF 03 E8 00
            isolation_resistance = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
          }
          break;
      }
      break;
    default:
      break;
  }
}

void HyundaiIoniq28Battery::transmit_can(unsigned long currentMillis) {

  if (!startedUp) {
    return;  // Don't send any CAN messages towards battery until it has started up
  }

  //Send 250ms message
  if (currentMillis - previousMillis250 >= INTERVAL_250_MS) {
    previousMillis250 = currentMillis;

    switch (poll_group)  //Swap between the 5 groups every 250ms
    {
      case 0:
        IONIQ_7E4_POLL.data.u8[2] = 0x01;
        poll_group = 1;
        break;
      case 1:
        IONIQ_7E4_POLL.data.u8[2] = 0x02;
        poll_group = 2;
        break;
      case 2:
        IONIQ_7E4_POLL.data.u8[2] = 0x03;
        poll_group = 3;
        break;
      case 3:
        IONIQ_7E4_POLL.data.u8[2] = 0x04;
        poll_group = 4;
        break;
      case 4:
        IONIQ_7E4_POLL.data.u8[2] = 0x05;
        poll_group = 0;
        break;
      default:
        //Unhandlex exception
        poll_group = 0;
        break;
    }

    transmit_can_frame(&IONIQ_7E4_POLL);
  }

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&IONIQ_553);
    transmit_can_frame(&IONIQ_57F);
    transmit_can_frame(&IONIQ_2A1);
  }

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    switch (counter_200) {
      case 0:
        IONIQ_200.data.u8[5] = 0x17;
        ++counter_200;
        break;
      case 1:
        IONIQ_200.data.u8[5] = 0x57;
        ++counter_200;
        break;
      case 2:
        IONIQ_200.data.u8[5] = 0x97;
        ++counter_200;
        break;
      case 3:
        IONIQ_200.data.u8[5] = 0xD7;
        ++counter_200;
        break;
      case 4:
        IONIQ_200.data.u8[3] = 0x10;
        IONIQ_200.data.u8[5] = 0xFF;
        ++counter_200;
        break;
      case 5:
        IONIQ_200.data.u8[5] = 0x3B;
        ++counter_200;
        break;
      case 6:
        IONIQ_200.data.u8[5] = 0x7B;
        ++counter_200;
        break;
      case 7:
        IONIQ_200.data.u8[5] = 0xBB;
        ++counter_200;
        break;
      case 8:
        IONIQ_200.data.u8[5] = 0xFB;
        counter_200 = 5;
        break;
    }

    transmit_can_frame(&IONIQ_200);
    transmit_can_frame(&IONIQ_523);
    transmit_can_frame(&IONIQ_524);
  }
}

void HyundaiIoniq28Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.total_capacity_Wh = 28000;
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

uint16_t HyundaiIoniq28Battery::get_lead_acid_voltage() const {
  return leadAcidBatteryVoltage;
}

int16_t HyundaiIoniq28Battery::get_power_relay_temperature() const {
  return powerRelayTemperature * 2;
}

uint8_t HyundaiIoniq28Battery::get_battery_management_mode() const {
  return batteryManagementMode;
}

uint16_t HyundaiIoniq28Battery::get_isolation_resistance() const {
  return isolation_resistance;
}
