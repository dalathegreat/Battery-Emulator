#include "DALY-BMS.h"
#ifdef DALY_BMS
#include <cstdint>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"
#include "RENAULT-TWIZY.h"
#include "RJXZS-BMS.h"

/* Do not change code below unless you are sure what you are doing */

static uint32_t lastPacket = 0;
static int16_t temperature_min_dC = 0;
static int16_t temperature_max_dC = 0;
static int16_t current_dA = 0;
static uint16_t voltage_dV = 0;
static uint32_t remaining_capacity_mAh = 0;
static uint16_t cellvoltages_mV[48] = {0};
static uint16_t cellvoltage_min_mV = 0;
static uint16_t cellvoltage_max_mV = 0;
static uint16_t SOC = 0;
static bool has_fault = false;

void update_values_battery() {
  datalayer.battery.status.real_soc = SOC;
  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)
  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0)
  datalayer.battery.status.remaining_capacity_Wh = (remaining_capacity_mAh * (uint32_t)voltage_dV) / 10000;

  datalayer.battery.status.max_charge_power_W = (BATTERY_MAX_CHARGE_AMP * voltage_dV) / 100;
  datalayer.battery.status.max_discharge_power_W = (BATTERY_MAX_DISCHARGE_AMP * voltage_dV) / 100;

  // limit power when SoC is low or high
  uint32_t adaptive_power_limit = 999999;
  if (SOC < 2000)
    adaptive_power_limit = ((uint32_t)SOC * POWER_PER_PERCENT) / 100;
  else if (SOC > 8000)
    adaptive_power_limit = ((10000 - (uint32_t)SOC) * POWER_PER_PERCENT) / 100;

  if (adaptive_power_limit < datalayer.battery.status.max_charge_power_W)
    datalayer.battery.status.max_charge_power_W = adaptive_power_limit;
  if (SOC < 2000 && adaptive_power_limit < datalayer.battery.status.max_discharge_power_W)
    datalayer.battery.status.max_discharge_power_W = adaptive_power_limit;

  // always allow to charge at least a little bit
  if (datalayer.battery.status.max_charge_power_W < POWER_PER_PERCENT)
    datalayer.battery.status.max_charge_power_W = POWER_PER_PERCENT;

  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages_mV, sizeof(cellvoltages_mV));
  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min_mV;
  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max_mV;

  datalayer.battery.status.temperature_min_dC = temperature_min_dC;
  datalayer.battery.status.temperature_max_dC = temperature_max_dC;

  datalayer.battery.status.real_bms_status = has_fault ? BMS_FAULT : BMS_ACTIVE;
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "DALY RS485", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = CELL_COUNT;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.total_capacity_Wh = BATTERY_WH_MAX;
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

#ifdef DEBUG_VIA_USB
void dump_buff(const char* msg, uint8_t* buff, uint8_t len) {
  Serial.print("[DALY-BMS] ");
  Serial.print(msg);
  for (int i = 0; i < len; i++) {
    Serial.print(buff[i] >> 4, HEX);
    Serial.print(buff[i] & 0xf, HEX);
    Serial.print(" ");
  }
  Serial.println();
}
#endif

void decode_packet(uint8_t command, uint8_t data[8]) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  switch (command) {
    case 0x90:
      voltage_dV = decode_uint16be(data, 0);
      current_dA = decode_int16be(data, 4) - 30000;
      SOC = decode_uint16be(data, 6) * 10;
      break;
    case 0x91:
      cellvoltage_max_mV = decode_uint16be(data, 0);
      cellvoltage_min_mV = decode_uint16be(data, 3);
      break;
    case 0x92:
      temperature_max_dC = (data[0] - 40) * 10;
      temperature_min_dC = (data[2] - 40) * 10;
      break;
    case 0x93:
      remaining_capacity_mAh = decode_uint32be(data, 4);
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
      // for now we do not handle individual faults. All of them are 0 when ok, and 1 when a fault occurs
      has_fault = false;
      for (int i = 0; i < 8; i++) {
        if (data[i] != 0x00) {
          has_fault = true;
        }
      }
      break;
  }
}

void transmit_rs485() {
  static uint8_t nextCommand = 0x90;

  if (millis() - lastPacket > 60) {
    uint8_t tx_buff[13] = {0};
    tx_buff[0] = 0xA5;
    tx_buff[1] = 0x40;
    tx_buff[2] = nextCommand;
    tx_buff[3] = 8;
    tx_buff[12] = calculate_checksum(tx_buff);

#ifdef DEBUG_VIA_USB
    dump_buff("transmitting: ", tx_buff, 13);
#endif

    Serial2.write(tx_buff, 13);
    lastPacket = millis();

    nextCommand++;
    if (nextCommand > 0x98)
      nextCommand = 0x90;
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

#ifdef DEBUG_VIA_USB
      dump_buff("dropping partial rx: ", recv_buff, recv_len);
#endif
      recv_len = 0;
    }

    if (recv_len > 12) {
#ifdef DEBUG_VIA_USB
      dump_buff("decoding successfull rx: ", recv_buff, recv_len);
#endif
      decode_packet(recv_buff[2], &recv_buff[4]);
      recv_len = 0;
      lastPacket = millis();
    }
  }
}

#endif
