#include "DALY-BMS.h"
#include <cstdint>
#include "../include.h"
#include "RJXZS-BMS.h"
#ifdef DALY_BMS
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RENAULT-TWIZY.h"

/* Do not change code below unless you are sure what you are doing */

static int16_t temperature_min = 0;
static int16_t temperature_max = 0;
static int16_t current_dA = 0;
static uint16_t voltage_dV = 0;
static uint16_t remaining_capacity_Ah = 0;
static uint16_t cellvoltages_mV[48] = {0};
static uint16_t cellvoltage_min = 0;
static uint16_t cellvoltage_max = 0;
static uint16_t SOC = 0;

void update_values_battery() {
  datalayer.battery.status.real_soc = SOC;
  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)
  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0)
  datalayer.battery.status.remaining_capacity_Wh = (remaining_capacity_Ah * DESIGN_PACK_VOLTAGE_DB) / 10;

  datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_ALLOWED_W;
  datalayer.battery.status.max_discharge_power_W = MAX_DISCHARGE_POWER_ALLOWED_W;

  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mV, sizeof(cellvoltages_mV));
  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min;
  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max;

  datalayer.battery.status.temperature_min_dC = temperature_min;
  datalayer.battery.status.temperature_max_dC = temperature_max;
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "DALY RS485", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = CELL_COUNT;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}

uint8_t calculate_checksum(uint8_t buff[12]) {
  uint8_t check = 0;
  for (uint8_t i = 0; i < 12; i++) {
    check += buff[i];
  }
  return check;
}

uint16_t decode_uint16be(uint8_t data[8], uint8_t offset) {
  uint16_t upper = data[offset];
  uint16_t lower = data[offset + 1];
  return (upper << 8) | lower;
}
int16_t decode_int16be(uint8_t data[8], uint8_t offset) {
  int16_t upper = data[offset];
  int16_t lower = data[offset + 1];
  return (upper << 8) | lower;
}
uint32_t decode_uint32be(uint8_t data[8], uint8_t offset) {
  return (((uint32_t)data[offset]) << 24) | (((uint32_t)data[offset + 1]) << 16) | (((uint32_t)data[offset + 2]) << 8) |
         ((uint32_t)data[offset + 3]);
}

void decode_packet(uint8_t command, uint8_t data[8]) {
  switch (command) {
    case 0x90:
      voltage_dV = decode_uint16be(data, 0);
      current_dA = decode_int16be(data, 4) - 30000;
      SOC = decode_uint16be(data, 6) * 10;
      break;
    case 0x91:
      cellvoltage_max = decode_uint16be(data, 0);
      cellvoltage_min = decode_uint16be(data, 3);
      break;
    case 0x92:
      temperature_max = decode_int16be(data, 0) - 40;
      temperature_min = decode_int16be(data, 2) - 40;
      break;
    case 0x93:
      remaining_capacity_Ah = decode_uint32be(data, 4);
      break;
    case 0x94:
      break;
    case 0x95:
      if (data[0] > 0 && data[0] <= 16) {
        uint8_t frame_index = (data[0] - 1) * 3;
        cellvoltages_mV[frame_index + 0] = decode_uint16be(data, 1);
        cellvoltages_mV[frame_index + 1] = decode_uint16be(data, 3);
        cellvoltages_mV[frame_index + 2] = decode_uint16be(data, 5);
      }
      break;
    case 0x96:
      break;
    case 0x97:
      break;
    case 0x98:
      break;
  }
}

void transmit_rs485() {
  static uint32_t lastSend = 0;
  static uint8_t nextCommand = 0x90;

  if (millis() - lastSend > 10) {
    uint8_t tx_buff[13] = {0};
    tx_buff[0] = 0xA5;
    tx_buff[1] = 0x80;
    tx_buff[2] = nextCommand;
    tx_buff[3] = 8;
    tx_buff[12] = calculate_checksum(tx_buff);

    Serial2.write(tx_buff, 13);
    lastSend = millis();
  }
}

void receive_RS485() {
  static uint8_t recv_buff[13] = {0};
  static uint8_t recv_len = 0;

  while (Serial2.available()) {
    recv_buff[recv_len] = Serial2.read();
    recv_len++;

    if (recv_len > 0 && recv_buff[0] != 0xA5 || recv_len > 1 && recv_buff[1] != 0x01 ||
        recv_len > 2 && (recv_buff[2] < 0x90 || recv_buff[2] > 0x98) || recv_len > 3 && recv_buff[3] != 8 ||
        recv_len > 12 && recv_buff[12] != calculate_checksum(recv_buff)) {

      recv_len = 0;
    }

    if (recv_len > 12) {
      decode_packet(recv_buff[2], &recv_buff[4]);
      recv_len = 0;
    }
  }
}

#endif
