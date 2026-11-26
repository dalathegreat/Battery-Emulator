#include "PYLON-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../inverter/INVERTERS.h"

namespace {
// Minimal single-purpose SHA-256 (FIPS 180-4) so the 0x4201 challenge does not depend on which
// mbedtls API version the platform ships.
const uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

inline uint32_t rotr32(uint32_t x, int n) {
  return (x >> n) | (x << (32 - n));
}

// SHA-256 of a message of at most 55 bytes (fits in one 64-byte block).
void sha256_short(const uint8_t* msg, uint8_t len, uint8_t digest[32]) {
  uint8_t block[64] = {0};
  for (uint8_t i = 0; i < len; i++)
    block[i] = msg[i];
  block[len] = 0x80;
  uint32_t bits = (uint32_t)len * 8;
  block[60] = (uint8_t)(bits >> 24);
  block[61] = (uint8_t)(bits >> 16);
  block[62] = (uint8_t)(bits >> 8);
  block[63] = (uint8_t)bits;

  uint32_t w[64];
  for (int i = 0; i < 16; i++)
    w[i] = ((uint32_t)block[4 * i] << 24) | ((uint32_t)block[4 * i + 1] << 16) | ((uint32_t)block[4 * i + 2] << 8) |
           block[4 * i + 3];
  for (int i = 16; i < 64; i++) {
    uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
  for (int i = 0; i < 64; i++) {
    uint32_t t1 = hh + (rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25)) + ((e & f) ^ (~e & g)) + SHA256_K[i] + w[i];
    uint32_t t2 = (rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
    hh = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  h[0] += a;
  h[1] += b;
  h[2] += c;
  h[3] += d;
  h[4] += e;
  h[5] += f;
  h[6] += g;
  h[7] += hh;
  for (int i = 0; i < 8; i++) {
    digest[4 * i] = (uint8_t)(h[i] >> 24);
    digest[4 * i + 1] = (uint8_t)(h[i] >> 16);
    digest[4 * i + 2] = (uint8_t)(h[i] >> 8);
    digest[4 * i + 3] = (uint8_t)h[i];
  }
}
}  // namespace

namespace {
inline void put_u16le(CAN_frame& f, int i, uint16_t v) {
  f.data.u8[i] = (uint8_t)(v & 0xFF);
  f.data.u8[i + 1] = (uint8_t)(v >> 8);
}
// Pylon/Deye current encoding: 0.1 A with +30000 offset (30000 = 0 A, below = charging)
inline uint16_t current_with_offset(int32_t dA) {
  int32_t v = dA + 30000;
  return (uint16_t)(v < 0 ? 0 : (v > 65535 ? 65535 : v));
}
// Pylon temperature encoding: 0.1 degC with +100 degC offset
inline uint16_t temp_pylon(int16_t dC) {
  return (uint16_t)(dC + 1000);
}
// Deye per-tower temperature encoding: whole degC + 40
inline uint16_t temp_deye(int16_t dC) {
  int v = dC / 10 + 40;
  return (uint16_t)(v < 0 ? 0 : v);
}
}  // namespace

void PylonInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //Check what discharge and charge cutoff voltages to send
  if (datalayer.battery_settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    discharge_cutoff_voltage_dV = datalayer.battery_settings.max_user_set_discharge_voltage_dV;
    charge_cutoff_voltage_dV = datalayer.battery_settings.max_user_set_charge_voltage_dV;
  } else {
    discharge_cutoff_voltage_dV = (datalayer.aggregate.min_design_voltage_dV + VOLTAGE_OFFSET_DV);
    charge_cutoff_voltage_dV = (datalayer.aggregate.max_design_voltage_dV - VOLTAGE_OFFSET_DV);
  }

  const bool fault = datalayer.system.status.system_status == FAULT;
  const uint16_t max_charge_dA = fault ? 0 : datalayer.aggregate.max_charge_current_dA;
  const uint16_t max_discharge_dA = fault ? 0 : datalayer.aggregate.max_discharge_current_dA;
  const bool charge_allowed = max_charge_dA != 0;
  const bool discharge_allowed = max_discharge_dA != 0;

  // System description (0x7320)
  int modules = user_selected_inverter_modules;
  if (modules <= 0) {
    // 16 cells x 3.6 V = 57.6 V max per module (round to nearest)
    modules = (datalayer.aggregate.max_design_voltage_dV + 288) / 576;
    if (modules < 1)
      modules = 1;
  }
  int cells_per_module =
      user_selected_inverter_cells_per_module > 0 ? user_selected_inverter_cells_per_module : CELLS_PER_MODULE;
  int voltage_level =
      user_selected_inverter_voltage_level > 0 ? user_selected_inverter_voltage_level : VOLTS_PER_MODULE * modules;
  int ah = user_selected_inverter_ah_capacity > 0 ? user_selected_inverter_ah_capacity : AH_CAPACITY;
  put_u16le(PYLON_7320, 0, TOWERS);
  PYLON_7320.data.u8[2] = (uint8_t)modules;
  PYLON_7320.data.u8[3] = (uint8_t)cells_per_module;
  put_u16le(PYLON_7320, 4, (uint16_t)voltage_level);
  put_u16le(PYLON_7320, 6, (uint16_t)(ah * TOWERS));

  const uint16_t soc = datalayer.aggregate.reported_soc / 100;  // %
  const uint16_t soh = datalayer.aggregate.soh_pptt / 100;      // %

  // 0x4210 ensemble: voltage 0.1 V, current, temperature (battery sends its BMS board temperature;
  // we have none, so max cell temperature), SOC %, SOH %
  put_u16le(PYLON_4210, 0, datalayer.aggregate.voltage_dV);
  put_u16le(PYLON_4210, 2, current_with_offset(datalayer.aggregate.current_dA));
  put_u16le(PYLON_4210, 4, temp_pylon(datalayer.aggregate.temperature_max_dC));
  PYLON_4210.data.u8[6] = (uint8_t)soc;
  PYLON_4210.data.u8[7] = (uint8_t)soh;

  // 0x4220 limits: cut-off voltages 0.1 V, max charge / discharge current (0.1 A, +30000)
  put_u16le(PYLON_4220, 0, charge_cutoff_voltage_dV);
  put_u16le(PYLON_4220, 2, discharge_cutoff_voltage_dV);
  put_u16le(PYLON_4220, 4, current_with_offset(max_charge_dA));
  put_u16le(PYLON_4220, 6, current_with_offset(max_discharge_dA));

  // 0x4230 cell voltage extremes (mV) + cell numbers (not known: 1)
  put_u16le(PYLON_4230, 0, datalayer.aggregate.cell_max_voltage_mV);
  put_u16le(PYLON_4230, 2, datalayer.aggregate.cell_min_voltage_mV);
  put_u16le(PYLON_4230, 4, 1);
  put_u16le(PYLON_4230, 6, 1);

  // 0x4240 / 0x4270 cell / module temperature extremes + numbers (the battery sends the same values in both)
  put_u16le(PYLON_4240, 0, temp_pylon(datalayer.aggregate.temperature_max_dC));
  put_u16le(PYLON_4240, 2, temp_pylon(datalayer.aggregate.temperature_min_dC));
  put_u16le(PYLON_4240, 4, 1);
  put_u16le(PYLON_4240, 6, 1);
  PYLON_4270 = PYLON_4240;
  PYLON_4270.ID = 0x4270;

  // 0x4250 status: 0 = sleep/idle, 1 = charging, 2 = discharging (the battery sends 0 at zero current)
  uint8_t status = 0;
  if (!fault) {
    if (datalayer.aggregate.current_dA < 0)
      status = 1;
    else if (datalayer.aggregate.current_dA > 0)
      status = 2;
  }
  PYLON_4250.data.u8[0] = status;

  // 0x4260 module voltage extremes (mV, 16-bit; the battery has 0.1 V resolution) + module numbers;
  // approximated from the cell extremes
  put_u16le(PYLON_4260, 0, (uint16_t)(datalayer.aggregate.cell_max_voltage_mV * cells_per_module / 100 * 100));
  put_u16le(PYLON_4260, 2, (uint16_t)(datalayer.aggregate.cell_min_voltage_mV * cells_per_module / 100 * 100));
  put_u16le(PYLON_4260, 4, 1);
  put_u16le(PYLON_4260, 6, 1);

  // 0x4280 charge / discharge forbidden
  PYLON_4280.data.u8[0] = charge_allowed ? 0x00 : 0xAA;
  PYLON_4280.data.u8[1] = discharge_allowed ? 0x00 : 0xAA;

  // 0x5210 tower 1: voltage 0.1 V, current (0.1 A +30000), temperature degC+40, SOC %, SOH %
  put_u16le(PYLON_5210, 0, datalayer.aggregate.voltage_dV);
  put_u16le(PYLON_5210, 2, current_with_offset(datalayer.aggregate.current_dA));
  put_u16le(PYLON_5210, 4, temp_deye(datalayer.aggregate.temperature_max_dC));
  PYLON_5210.data.u8[6] = (uint8_t)soc;
  PYLON_5210.data.u8[7] = (uint8_t)soh;

  // 0x5220 tower cell voltages: max mV, module, cell, min mV, module, cell
  put_u16le(PYLON_5220, 0, datalayer.aggregate.cell_max_voltage_mV);
  PYLON_5220.data.u8[2] = 1;
  PYLON_5220.data.u8[3] = 1;
  put_u16le(PYLON_5220, 4, datalayer.aggregate.cell_min_voltage_mV);
  PYLON_5220.data.u8[6] = 1;
  PYLON_5220.data.u8[7] = 1;

  // 0x5230 tower cell temperatures: max degC+40 (u16), module, sensor, min degC+40 (u16), module, sensor
  put_u16le(PYLON_5230, 0, temp_deye(datalayer.aggregate.temperature_max_dC));
  PYLON_5230.data.u8[2] = 1;
  PYLON_5230.data.u8[3] = 0;
  put_u16le(PYLON_5230, 4, temp_deye(datalayer.aggregate.temperature_min_dC));
  PYLON_5230.data.u8[6] = 1;
  PYLON_5230.data.u8[7] = 0;

  // 0x5240 tower current limits: charge, discharge (0.1 A, no offset)
  put_u16le(PYLON_5240, 0, max_charge_dA);
  put_u16le(PYLON_5240, 2, max_discharge_dA);

  // 0x5250 byte 1: module count
  PYLON_5250.data.u8[1] = (uint8_t)modules;

  // 0x52A0 byte 1: modules present
  PYLON_52A0.data.u8[1] = modules > 0 ? 1 : 0;

  // 0x5280 voltage measurement points (0.1 V); bytes 2-3 are the tower voltage
  put_u16le(PYLON_5280, 2, datalayer.aggregate.voltage_dV);

  // 0x52F0 byte 0: bit 2 charge relay closed, bit 3 discharge relay closed
  PYLON_52F0.data.u8[0] = (charge_allowed ? 0x04 : 0x00) | (discharge_allowed ? 0x08 : 0x00);
}

void PylonInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4200:  //Inverter request. The Deye battery broadcasts on its own schedule and ignores the content.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4201:  //Challenge from Deye inverters, answered with SHA-256 of the 8 data bytes
      send_challenge_response(rx_frame);
      break;
    default:
      break;
  }
}

void PylonInverter::transmit_can(unsigned long currentMillis) {
  // Same dispatcher as the battery firmware: every 10 ms tick the table is scanned in order, each slot's
  // counter advances, and the scan stops at the first due frame, which is sent. Slots after it miss that
  // tick, so the 5 s frames actually repeat about every 5.5 s and gaps inside a burst slowly grow -
  // exactly what the real battery does.
  if (!tickStarted) {
    previousTickMillis = currentMillis;
    tickStarted = true;
  }
  int ticks = 0;
  while (currentMillis - previousTickMillis >= TICK_MS && ticks < 100) {
    previousTickMillis += TICK_MS;
    ticks++;
    for (auto& slot : schedule) {
      if (slot.elapsed < 65000)
        slot.elapsed++;
      if (slot.elapsed >= slot.period) {
        transmit_can_frame(slot.frame);
        slot.elapsed = 0;
        break;
      }
    }
  }
  if (ticks == 100)
    previousTickMillis = currentMillis;  // far behind (e.g. first call): resynchronise
}

void PylonInverter::send_challenge_response(const CAN_frame& challenge) {
  // Matches Deye HVCU firmware (HVCUE1625C16N01, V2.05.1): 8 bytes are always hashed (bytes beyond
  // DLC taken as zero), and the reply is the first 8 bytes of the digest, unaffected by the
  // byte-order/offset options.
  uint8_t msg[8] = {0};
  for (int i = 0; i < 8 && i < challenge.DLC; i++) {
    msg[i] = challenge.data.u8[i];
  }
  uint8_t digest[32];
  sha256_short(msg, 8, digest);
  for (int i = 0; i < 8; i++) {
    PYLON_4202.data.u8[i] = digest[i];
  }
  transmit_can_frame(&PYLON_4202);
}

bool PylonInverter::setup() {
  return true;
}
