#ifndef GROWATT_LV_BATTERY_H
#define GROWATT_LV_BATTERY_H

#include "../datalayer/datalayer.h"
#include "CanBattery.h"

// Battery-facing integration for the Growatt low-voltage (48V) BMS CAN
// protocol - GBLI-series packs (e.g. GBLI6532), used stacked/parallel behind
// a Growatt hybrid inverter's PCS port.
//
// Role:
//  - This implementation behaves as the PCS (inverter) side of the link.
//  - It sends the PCS->BMS keepalive (0x301) at 1 Hz; the real hardware
//    enables charge/discharge within 2-4s of seeing this frame.
//  - It parses BMS->PCS frames (0x311-0x314, 0x315-0x318, 0x319) and
//    populates the datalayer.
//
// Byte layout and bit positions below were confirmed against a real
// GBLI6532 (2-pack system) paired with a genuine Growatt SPH6000 inverter,
// not inferred from the spec alone - see the capture-backed writeup this PR
// is based on (github.com/schmellic/growatt2solis). Notably:
//  - 0x311 byte 7 bits 5/6 are the charge/discharge-enable bits, and are
//    authoritative: a real idle->discharge->charge capture showed them
//    track live BMS state correctly, while 0x319's own enable bits (byte 0
//    bits 5/6) stayed constant throughout, including during a real ~10A
//    discharge. 0x319's bits are used only before the first 0x311 arrives.
//  - 0x312 byte 4 is the live parallel pack count, confirmed against a real
//    2-pack system reporting exactly 2x the single-pack current ceiling.
//  - 0x313 bytes 0-1 (pack voltage) are 0.01V, not 0.1V - confirmed by
//    plausible voltage range (53.2-53.4V) through a real charge/discharge
//    cycle; the 0.1V reading would imply ~530V.
//
// Protocol reference:
//  Growatt BMS CAN-Bus protocol, low voltage V1.04. Standard (11-bit) CAN
//  IDs, 500 kbit/s, big-endian.
class GrowattLvBattery : public CanBattery {
 public:
  GrowattLvBattery() {}

  ~GrowattLvBattery() {}

  void setup(void) override;
  void handle_incoming_can_frame(CAN_frame rx_frame) override;
  void update_values() override;
  void transmit_can(unsigned long currentMillis) override;

  static constexpr const char* Name = "Growatt LV (GBLI-series) battery via CAN, 500kbit/s";

 private:
  // GBLI6532 datasheet: 5kW / 104.2A max charge/discharge per pack. A real
  // 2-pack system's own reported CCL/DCL matched 2x this value exactly, so
  // the ceiling below scales with the live pack count from 0x312 byte 4.
  static const int MAX_CURRENT_PER_PACK_dA = 1042;

  // Solis accepts 40.0-60.0V; GBLI6532 datasheet range is 48.0-57.6V.
  static const int MAX_PACK_VOLTAGE_DV = 576;
  static const int MIN_PACK_VOLTAGE_DV = 480;

  unsigned long previousMillis1000 = 0;

  // --- Outgoing (PCS -> BMS) ---
  // Payload taken from the example in Growatt's protocol document. Its
  // semantics are undocumented; a real capture showed it static across an
  // entire session (idle, discharge, charge, cold boot) with no counter or
  // checksum, and a real Growatt inverter sending it unchanged is what gets
  // a real GBLI6532 to enable within seconds - see growatt2solis's own
  // README for the full commissioning writeup this port is based on.
  CAN_frame BMS_301 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x301,
                       .data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};

  // --- Parsed battery state ---
  uint16_t cvl_dV = 0;
  uint16_t ccl_dA = 0;
  uint16_t dcl_dA = 0;
  uint16_t status_word = 0;
  bool have_311 = false;

  uint8_t prot1 = 0, prot2 = 0, warn1 = 0, warn2 = 0;
  uint8_t pack_count = 1;
  bool have_312 = false;

  int16_t pack_v_cV = 0;   // 0.01V units
  int16_t current_dA = 0;  // 0.1A, + = charging
  int16_t temp_dC = 0;     // 0.1 degC
  uint8_t soc_pct = 0;
  uint8_t soh_pct = 100;
  bool have_313 = false;

  bool charge_en = false;
  bool discharge_en = false;
  bool force_chg_1 = false;
  bool force_chg_2 = false;
};

#endif
