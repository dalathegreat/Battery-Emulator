#include "../include.h"
#ifdef SANTA_FE_PHEV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "SANTA-FE-PHEV-BATTERY.h"

/* Credits go to maciek16c for these findings!
https://github.com/maciek16c/hyundai-santa-fe-phev-battery
https://openinverter.org/forum/viewtopic.php?p=62256

TODO: Find cellvoltages in CAN data (alternatively poll for them)
TODO: Tweak temperature values once more data is known about them
TODO: Check if CRC function works like it should. This enables checking for corrupt messages
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
static uint8_t poll_data_pid = 0;
static uint8_t counter_200 = 0;
static uint8_t checksum_200 = 0;

static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 100;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint8_t CellVmaxNo = 0;
static uint8_t CellVminNo = 0;
static uint16_t allowedDischargePower = 0;
static uint16_t allowedChargePower = 0;
static uint16_t batteryVoltage = 0;
static int16_t leadAcidBatteryVoltage = 120;
static int8_t temperatureMax = 0;
static int8_t temperatureMin = 0;
static int16_t batteryAmps = 0;
static uint8_t StatusBattery = 0;
static uint16_t cellvoltages_mv[96];

#ifdef DOUBLE_BATTERY
static uint16_t battery2_SOC_Display = 0;
static uint16_t battery2_SOH = 100;
static uint16_t battery2_CellVoltMax_mV = 3700;
static uint16_t battery2_CellVoltMin_mV = 3700;
static uint8_t battery2_CellVmaxNo = 0;
static uint8_t battery2_CellVminNo = 0;
static uint16_t battery2_allowedDischargePower = 0;
static uint16_t battery2_allowedChargePower = 0;
static uint16_t battery2_batteryVoltage = 0;
static int16_t battery2_leadAcidBatteryVoltage = 120;
static int8_t battery2_temperatureMax = 0;
static int8_t battery2_temperatureMin = 0;
static int16_t battery2_batteryAmps = 0;
static uint8_t battery2_StatusBattery = 0;
static uint16_t battery2_cellvoltages_mv[96];
#endif  //DOUBLE_BATTERY

CAN_frame SANTAFE_200 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x200,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};
CAN_frame SANTAFE_2A1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A1,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x02}};
CAN_frame SANTAFE_2F0 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2F0,
                         .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00}};
CAN_frame SANTAFE_523 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x523,
                         .data = {0x60, 0x00, 0x60, 0, 0, 0, 0, 0}};
CAN_frame SANTAFE_7E4_poll = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E4,  //Polling frame, 0x22 01 0X
                              .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SANTAFE_7E4_ack = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,  //Ack frame, correct PID is returned. Flow control message
                             .data = {0x30, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;

  datalayer.battery.status.current_dA = -batteryAmps;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer.battery.status.max_charge_power_W = allowedChargePower * 10;

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  datalayer.battery.status.temperature_min_dC = temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1FF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      StatusBattery = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x4D5:
      break;
    case 0x4DD:
      break;
    case 0x4DE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4E0:
      break;
    case 0x542:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_Display = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]) / 2;
      break;
    case 0x588:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryVoltage = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]);
      break;
    case 0x597:
      break;
    case 0x5A6:
      break;
    case 0x5A7:
      break;
    case 0x5AD:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryAmps = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2];
      break;
    case 0x5AE:
      break;
    case 0x5F1:
      break;
    case 0x620:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      leadAcidBatteryVoltage = rx_frame.data.u8[1];
      temperatureMin = rx_frame.data.u8[6];  //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];  //Highest temp in battery
      break;
    case 0x670:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x671:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[4] == poll_data_pid) {
            transmit_can_frame(&SANTAFE_7E4_ack,
                               can_config.battery);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[32] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[33] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[34] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[35] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[36] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[37] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[64] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[65] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[66] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[67] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[68] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[69] = (rx_frame.data.u8[7] * 20);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 2) {
            cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[38] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[39] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[40] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[41] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[42] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[43] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[44] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[70] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[71] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[72] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[73] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[74] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[75] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[76] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 6) {
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[45] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[46] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[47] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[48] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[49] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[50] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[51] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[77] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[78] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[79] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[80] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[81] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[82] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[83] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            if (rx_frame.data.u8[6] > 0) {
              batterySOH = rx_frame.data.u8[6];
            }
            if (batterySOH > 100) {
              batterySOH = 100;
            }
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[52] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[53] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[54] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[84] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[85] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[86] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[87] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[88] = (rx_frame.data.u8[5] * 20);
            cellvoltages_mv[89] = (rx_frame.data.u8[6] * 20);
            cellvoltages_mv[90] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 2) {
            cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[31] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 3) {
            cellvoltages_mv[59] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[60] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[61] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[62] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[63] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 4) {
            cellvoltages_mv[91] = (rx_frame.data.u8[1] * 20);
            cellvoltages_mv[92] = (rx_frame.data.u8[2] * 20);
            cellvoltages_mv[93] = (rx_frame.data.u8[3] * 20);
            cellvoltages_mv[94] = (rx_frame.data.u8[4] * 20);
            cellvoltages_mv[95] = (rx_frame.data.u8[5] * 20);

            //Map all cell voltages to the global array, we have sampled them all!
            memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mv, 96 * sizeof(uint16_t));
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          break;
        case 0x27:  //Seventh datarow in PID group
          break;
        case 0x28:  //Eighth datarow in PID group
          break;
      }
      break;
    default:
      break;
  }
}
void transmit_can_battery() {
  unsigned long currentMillis = millis();

  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10 = currentMillis;

    SANTAFE_200.data.u8[6] = (counter_200 << 1);

    checksum_200 = CalculateCRC8(SANTAFE_200);

    SANTAFE_200.data.u8[7] = checksum_200;

    transmit_can_frame(&SANTAFE_200, can_config.battery);
    transmit_can_frame(&SANTAFE_2A1, can_config.battery);
    transmit_can_frame(&SANTAFE_2F0, can_config.battery);
#ifdef DOUBLE_BATTERY
    transmit_can_frame(&SANTAFE_200, can_config.battery_double);
    transmit_can_frame(&SANTAFE_2A1, can_config.battery_double);
    transmit_can_frame(&SANTAFE_2F0, can_config.battery_double);
#endif  //DOUBLE_BATTERY

    counter_200++;
    if (counter_200 > 0xF) {
      counter_200 = 0;
    }
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&SANTAFE_523, can_config.battery);
#ifdef DOUBLE_BATTERY
    transmit_can_frame(&SANTAFE_523, can_config.battery_double);
#endif  //DOUBLE_BATTERY
  }

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    // PID data is polled after last message sent from battery:
    poll_data_pid = (poll_data_pid % 5) + 1;
    SANTAFE_7E4_poll.data.u8[3] = (uint8_t)poll_data_pid;
    transmit_can_frame(&SANTAFE_7E4_poll, can_config.battery);
#ifdef DOUBLE_BATTERY
    transmit_can_frame(&SANTAFE_7E4_poll, can_config.battery_double);
#endif  //DOUBLE_BATTERY
  }
}

#ifdef DOUBLE_BATTERY
void update_values_battery2() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery2.status.real_soc = (battery2_SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery2.status.soh_pptt = (battery2_SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery2.status.voltage_dV = battery2_batteryVoltage;

  datalayer.battery2.status.current_dA = -battery2_batteryAmps;

  datalayer.battery2.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery2.status.real_soc) / 10000) * datalayer.battery2.info.total_capacity_Wh);

  datalayer.battery2.status.max_discharge_power_W = battery2_allowedDischargePower * 10;

  datalayer.battery2.status.max_charge_power_W = battery2_allowedChargePower * 10;

  //Power in watts, Negative = charging batt
  datalayer.battery2.status.active_power_W =
      ((datalayer.battery2.status.voltage_dV * datalayer.battery2.status.current_dA) / 100);

  datalayer.battery2.status.cell_max_voltage_mV = battery2_CellVoltMax_mV;

  datalayer.battery2.status.cell_min_voltage_mV = battery2_CellVoltMin_mV;

  datalayer.battery2.status.temperature_min_dC = battery2_temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery2.status.temperature_max_dC = battery2_temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  if (battery2_leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, battery2_leadAcidBatteryVoltage);
  }
}

void map_can_frame_to_variable_battery2(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1FF:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_StatusBattery = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x4D5:
      break;
    case 0x4DD:
      break;
    case 0x4DE:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4E0:
      break;
    case 0x542:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_SOC_Display = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]) / 2;
      break;
    case 0x588:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_batteryVoltage = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]);
      break;
    case 0x597:
      break;
    case 0x5A6:
      break;
    case 0x5A7:
      break;
    case 0x5AD:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_batteryAmps = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2];
      break;
    case 0x5AE:
      break;
    case 0x5F1:
      break;
    case 0x620:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_leadAcidBatteryVoltage = rx_frame.data.u8[1];
      battery2_temperatureMin = rx_frame.data.u8[6];  //Lowest temp in battery
      battery2_temperatureMax = rx_frame.data.u8[7];  //Highest temp in battery
      break;
    case 0x670:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery2_allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery2_allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x671:
      datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[4] == poll_data_pid) {
            transmit_can_frame(&SANTAFE_7E4_ack,
                               can_config.battery_double);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
          } else if (poll_data_pid == 2) {
            battery2_cellvoltages_mv[0] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[1] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[2] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[3] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[4] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[5] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            battery2_cellvoltages_mv[32] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[33] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[34] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[35] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[36] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[37] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            battery2_cellvoltages_mv[64] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[65] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[66] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[67] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[68] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[69] = (rx_frame.data.u8[7] * 20);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 2) {
            battery2_cellvoltages_mv[6] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[7] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[8] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[9] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[10] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[11] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[12] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            battery2_cellvoltages_mv[38] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[39] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[40] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[41] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[42] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[43] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[44] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            battery2_cellvoltages_mv[70] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[71] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[72] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[73] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[74] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[75] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[76] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 6) {
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            battery2_CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            battery2_cellvoltages_mv[13] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[14] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[15] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[16] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[17] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[18] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[19] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            battery2_cellvoltages_mv[45] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[46] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[47] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[48] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[49] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[50] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[51] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            battery2_cellvoltages_mv[77] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[78] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[79] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[80] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[81] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[82] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[83] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            if (rx_frame.data.u8[6] > 0) {
              battery2_SOH = rx_frame.data.u8[6];
            }
            if (battery2_SOH > 100) {
              battery2_SOH = 100;
            }
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            battery2_CellVmaxNo = rx_frame.data.u8[1];
            battery2_CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            battery2_cellvoltages_mv[20] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[21] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[22] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[23] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[24] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[25] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[26] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            battery2_cellvoltages_mv[52] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[53] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[54] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[55] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[56] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[57] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[58] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            battery2_cellvoltages_mv[84] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[85] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[86] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[87] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[88] = (rx_frame.data.u8[5] * 20);
            battery2_cellvoltages_mv[89] = (rx_frame.data.u8[6] * 20);
            battery2_cellvoltages_mv[90] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 2) {
            battery2_cellvoltages_mv[27] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[28] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[29] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[30] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[31] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 3) {
            battery2_cellvoltages_mv[59] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[60] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[61] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[62] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[63] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 4) {
            battery2_cellvoltages_mv[91] = (rx_frame.data.u8[1] * 20);
            battery2_cellvoltages_mv[92] = (rx_frame.data.u8[2] * 20);
            battery2_cellvoltages_mv[93] = (rx_frame.data.u8[3] * 20);
            battery2_cellvoltages_mv[94] = (rx_frame.data.u8[4] * 20);
            battery2_cellvoltages_mv[95] = (rx_frame.data.u8[5] * 20);

            //Map all cell voltages to the global array, we have sampled them all!
            memcpy(datalayer.battery2.status.cell_voltages_mV, battery2_cellvoltages_mv, 96 * sizeof(uint16_t));
          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          break;
        case 0x27:  //Seventh datarow in PID group
          break;
        case 0x28:  //Eighth datarow in PID group
          break;
      }
      break;
    default:
      break;
  }
}
#endif  //DOUBLE_BATTERY

uint8_t CalculateCRC8(CAN_frame rx_frame) {
  int crc = 0;

  for (uint8_t framepos = 0; framepos < 8; framepos++) {
    crc ^= rx_frame.data.u8[framepos];

    for (uint8_t j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0) {
        crc = (crc << 1) ^ 0x1;
      } else {
        crc <<= 1;
      }
    }
  }
  return (uint8_t)crc;
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Santa Fe PHEV", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.number_of_cells = datalayer.battery.info.number_of_cells;
  datalayer.battery2.info.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  datalayer.battery2.info.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  datalayer.battery2.info.max_cell_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  datalayer.battery2.info.min_cell_voltage_mV = datalayer.battery.info.min_cell_voltage_mV;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
#endif  //DOUBLE_BATTERY
}

#endif
