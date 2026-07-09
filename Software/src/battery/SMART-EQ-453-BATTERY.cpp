/**
 * Smart EQ / Renault 453 HV Battery support for Battery-Emulator.
 *
 * Protocol:  Classical CAN, 11-bit IDs, 500 kbit/s
 * Transport: ISO-TP (manually implemented, single-slot)
 * Request:   0x79B  service 0x21 + PID, or 0x22 + 2-byte PID
 * Response:  0x7BB  positive response 0x61 / 0x62 + PID + data
 *
 * Safety defaults:
 *   SMART453_ALLOW_CONTACTOR_CONTROL = 0  (contactors disabled)
 *   PID 0x02 (contactor counter) read at startup; abort if it decreases.
 *   No UDS write services are sent.
 *
 * Sources: ED4scan, OVMS Smart EQ, Battery-Emulator Zoe Gen1,
 *          supplied CAN trace, BMS Developer Handoff PDF.
 */

#include "SMART-EQ-453-BATTERY.h"
#include <Arduino.h>
#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"

// ── Startup PID sequence ──────────────────────────────────────────────────────
// Ordered read-only PIDs sent once per BMS session.
// 0x02 is first so we have the baseline counter before any other traffic.
const uint8_t SmartEq453Battery::STARTUP_PIDS[] = {
    0x02,  // HV contactor counter — MUST be read first
    0xEF,  // BMS part number
    0x80,  // BMS identification
    0xA0,  // Battery serial number
    0x90,  // Supplier/production identification
           // 0x22 0x03 0x04 handled separately (different service byte)
};
const uint8_t SmartEq453Battery::NUM_STARTUP_PIDS =
    sizeof(SmartEq453Battery::STARTUP_PIDS) / sizeof(SmartEq453Battery::STARTUP_PIDS[0]);

// ── setup() ───────────────────────────────────────────────────────────────────
void SmartEq453Battery::setup(void) {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  // Contactors are unconditionally disabled until real-hardware validation.
  // Even if SMART453_ALLOW_CONTACTOR_CONTROL is later set to 1, the
  // state machine still requires all safety checks to pass.
  datalayer.system.status.battery_allows_contactor_closing = false;

  datalayer_battery->info.number_of_cells = NUM_CELLS;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->info.total_capacity_Wh = TOTAL_CAPACITY_WH;

  state = Smart453State::LV_POWERED;
}

// ── handle_incoming_can_frame() ───────────────────────────────────────────────
void SmartEq453Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  const uint8_t* d = rx_frame.data.u8;
  const uint8_t dlc = rx_frame.DLC;

  switch (rx_frame.ID) {

    // ── BMS diagnostic response ────────────────────────────────────────────
    case 0x7BB: {
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if (dlc < 1)
        break;
      const uint8_t frame_type = d[0] & 0xF0;

      if (frame_type == 0x00 && !isotp_busy) {
        // Single frame
        const uint8_t sf_len = d[0] & 0x0F;
        if (sf_len < 2 || sf_len > 7 || sf_len > dlc - 1)
          break;
        // Positive response: data[1] = 0x61 or 0x62, data[2] = PID
        if ((d[1] != 0x61 && d[1] != 0x62) || sf_len < 2)
          break;
        const uint8_t pid = d[2];
        // Single-frame payload starts at d[1], length sf_len
        process_isotp_complete(pid, d + 1, sf_len);
        isotp_busy = false;

      } else if (frame_type == 0x10) {
        // First frame
        if (dlc < 8)
          break;
        const uint16_t total_len = (((uint16_t)(d[0] & 0x0F)) << 8) | d[1];
        if (total_len > sizeof(isotp_buf) || total_len < 2)
          break;
        isotp_exp_len = total_len;
        isotp_rxlen = 6;
        memcpy(isotp_buf, d + 2, 6);
        isotp_exp_sn = 1;
        isotp_pid = isotp_buf[1];  // P[1] = PID after service byte P[0]=0x61
        // Send flow control immediately
        transmit_can_frame(&frame_flow_control);

      } else if (frame_type == 0x20 && isotp_rxlen > 0) {
        // Consecutive frame
        const uint8_t sn = d[0] & 0x0F;
        if (sn != isotp_exp_sn) {
          // Sequence error — abort receive
          isotp_rxlen = 0;
          isotp_busy = false;
          datalayer_battery->status.CAN_error_counter++;
          break;
        }
        const uint16_t bytes_to_copy = (isotp_exp_len - isotp_rxlen < 7) ? (isotp_exp_len - isotp_rxlen) : 7;
        memcpy(isotp_buf + isotp_rxlen, d + 1, bytes_to_copy);
        isotp_rxlen += bytes_to_copy;
        isotp_exp_sn = (isotp_exp_sn % 15) + 1;

        if (isotp_rxlen >= isotp_exp_len) {
          // Complete
          process_isotp_complete(isotp_pid, isotp_buf, isotp_rxlen);
          isotp_rxlen = 0;
          isotp_busy = false;
        }
      }
      break;
    }

    // ── Vehicle state 0x350 (passive: observe, never transmit) ────────────
    case 0x350: {
      if (dlc < 1)
        break;
      vehicle_state_byte = d[0];
      // Freshness: if we see 0x350 we know the bus is alive
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    }

    // ── READY status 0x186 (passive: observe only, NEVER transmit) ────────
    // 0x186 is an output of the original vehicle network (BMS or gateway).
    // It must NOT be used as the sole proof of contactor closure.
    case 0x186: {
      if (dlc < 1)
        break;
      f186_ready = (d[0] == 0x19);
      break;
    }

    // ── Precharge correlation 0x1F6 (passive observation) ─────────────────
    // Direction (vehicle cmd vs BMS output) is not yet confirmed.
    // Byte 1: 0x00=idle, 0x30=probable precharge, 0x20=probable ready.
    case 0x1F6: {
      if (dlc < 2)
        break;
      f1f6_byte1 = d[1];
      break;
    }

    // ── Broadcast: pack voltage + DC link (10ms) ──────────────────────────
    // Confirmed from real trace: 0x0C6 bytes 0-1 = 327.37V at *0.01 scale.
    // Bytes 4-5 = DC link voltage (slightly higher during/after precharge).
    // This frame is available immediately on bus-up, before any UDS polling.
    case 0x0C6: {
      if (dlc < 6)
        break;
      decode_bcast_0x0C6(d, dlc);
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    }

    // ── Broadcast: SOC candidate (10ms) ──────────────────────────────────
    // Confirmed from real trace: byte 0 = 188-190 when gauge shows ~94-95%.
    // Scale: raw / 2.0 = %, i.e. raw * 50 = pptt. CANDIDATE — verify during validation.
    // Byte 2: 2-bit rolling counter cycling 0→1→2→3→0 (observed: values 3-6 offset).
    //         Not a balancing count — do not decode.
    case 0x12E: {
      if (dlc < 1)
        break;
      decode_bcast_0x12E(d, dlc);
      break;
    }

    // ── Broadcast: DC link precharge progress (10ms) ──────────────────────
    // Byte 0: precharge percentage (0-100%) for each of 16 measurement channels
    //         that scan through during capacitor charging. 0xFF = not charged/invalid.
    // Byte 4: channel index 0x00-0xF0 (steps of 0x10), one new channel every ~200ms.
    // From trace: 0xFF before precharge; rises 9→99% per channel during C6 precharge.
    // CANDIDATE — needs physical validation.
    case 0x090: {
      if (dlc < 2)
        break;
      decode_bcast_0x090(d, dlc);
      break;
    }

    // ── Broadcast: temperature candidate ──────────────────────────────────
    // Byte 0: raw value, raw / 2.0 = °C (scale candidate).
    // From trace: 74-75 observed throughout (= 37.0-37.5°C). Slowly varying.
    // Consistent with battery pack / coolant temperature. CANDIDATE.
    case 0x3B7: {
      if (dlc < 1)
        break;
      decode_bcast_0x3B7(d, dlc);
      break;
    }

    default:
      break;
  }
}

// ── process_isotp_complete() ──────────────────────────────────────────────────
void SmartEq453Battery::process_isotp_complete(uint8_t pid, const uint8_t* data, uint16_t len) {
  // data[0] = positive-response service (0x61 or 0x62)
  // data[1] = PID echo
  // data[2+] = BMS payload
  if (len < 2)
    return;
  if (data[0] != 0x61 && data[0] != 0x62) {
    // Negative response or unexpected service — log and ignore
    datalayer_battery->status.CAN_error_counter++;
    return;
  }
  if (data[1] != pid) {
    datalayer_battery->status.CAN_error_counter++;
    return;
  }

  // Payload P[] starts at data[0]:
  //   P[0] = service byte (0x61)
  //   P[1] = PID
  //   P[2..] = BMS data
  // All PID decoders below index from P[0].

  switch (pid) {
    case 0x07:
      decode_pid07(data, len);
      last_pid07_ms = millis();
      break;
    case 0x01:
      decode_pid01(data, len);
      break;
    case 0x08:
      decode_pid08(data, len);
      break;
    case 0x04:
      decode_pid04(data, len);
      break;
    case 0x29:
      decode_pid29(data, len);
      break;
    case 0x41:
      decode_pid41_42(data, len, 0);
      break;
    case 0x42:
      decode_pid41_42(data, len, 1);
      break;
    case 0x10:
      decode_pid10_11(data, len, 0);
      break;
    case 0x11:
      decode_pid10_11(data, len, 1);
      break;
    case 0x16:
      decode_pid16(data, len);
      break;
    case 0x25:
      decode_pid25(data, len);
      break;
    case 0x61:
      decode_pid61(data, len);
      break;
    case 0x0B:
      decode_pid0B(data, len);
      break;
    case 0x02:
      decode_pid02(data, len);
      break;
    case 0x80:
      decode_pid80(data, len);
      break;
    case 0xEF:
      decode_pidEF(data, len);
      break;
    case 0xA0:
      decode_pidA0(data, len);
      break;
    case 0xF0 /* pseudo-PID for 0x22/0x0304 production date */:
      decode_pid_22_0304(data, len);
      break;
    default:
      break;
  }
}

// ── PID decoders ──────────────────────────────────────────────────────────────
// All indices are absolute within the reassembled ISO-TP payload P[].
// P[0]=0x61, P[1]=PID, P[2+]=BMS data.
// Sources: BMS Developer Handoff PDF, ED4scan, OVMS Smart EQ.

void SmartEq453Battery::decode_pid07(const uint8_t* p, uint16_t len) {
  // Minimum required length: P[25] BMS supply = byte index 25
  if (len < 26)
    return;

  cell_min_raw = be16(p + 3);          // P[3..4]
  cell_max_raw = be16(p + 5);          // P[5..6]
  dc_link_voltage_raw = be16(p + 7);   // P[7..8]
  pack_sum_voltage_raw = be16(p + 9);  // P[9..10]
  pack_terminal_v_raw = be16(p + 11);  // P[11..12]
  pack_current_raw = sbe16(p + 13);    // P[13..14]
  contactor_state = p[15];             // P[15]
  hv_plug_interlock = p[16];           // P[16]
  service_disconnect = p[17];          // P[17]
  fusi_raw = be16(p + 18);             // P[18..19]
  safety_mode = p[22];                 // P[22]
  relay_status = (p[23] >> 4) & 0x03;  // P[23] bits [5:4]
  relay_permit = (p[23] >> 6) & 0x03;  // P[23] bits [7:6]
  vehicle_mode_req = p[24];            // P[24]
  bms_12v_supply_raw = p[25];          // P[25]
}

void SmartEq453Battery::decode_pid01(const uint8_t* p, uint16_t len) {
  // P[3..6]: max charge current,   signed BE32 / 1024.0 A
  // P[7..10]: max discharge current, signed BE32 / 1024.0 A
  // Correction: use normal signed 32-bit, NOT 65535 multiplier (ED4scan error).
  if (len < 11)
    return;
  charge_limit_raw = sbe32(p + 3);
  discharge_limit_raw = sbe32(p + 7);
}

void SmartEq453Battery::decode_pid08(const uint8_t* p, uint16_t len) {
  // P[5..6]: min internal cell/SOC value
  // P[7..8]: max internal cell/SOC value
  // P[13..14]: initial capacity estimate
  // P[15..16]: current estimated capacity
  // Scale: raw / 16 (candidate — confirm against 0x61 and 0x25)
  if (len < 17)
    return;
  soc_min_raw = be16(p + 5);
  soc_max_raw = be16(p + 7);
  capacity_init_raw = be16(p + 13);
  capacity_current_raw = be16(p + 15);
}

void SmartEq453Battery::decode_pid04(const uint8_t* p, uint16_t len) {
  // Layout confirmed by ED4scan + OVMS Smart EQ:
  //   [0] = P[2..3]:  overall pack minimum temperature
  //   [1] = P[4..5]:  overall pack maximum temperature  ← bit 15 is an offset flag
  //   [2] = P[6..7]:  overall pack average temperature
  //   [3..29] = P[8..61]:  27 individual sensor readings
  // Scale: BE16 / 64.0 = °C for all entries.
  // If bit 15 of temperature_raw[1] is set, subtract 0x0A00 from individual
  // readings [3+] to obtain true values (ED4scan/OVMS firmware quirk).
  if (len < 4)
    return;
  const uint16_t channels = (len - 2) / 2;
  const uint16_t store = channels < 31 ? channels : 31;
  for (uint16_t i = 0; i < store; i++) {
    temperature_raw[i] = be16(p + 2 + i * 2);
  }
  if (store > 1 && (temperature_raw[1] & 0x8000)) {
    temperature_raw[1] &= 0x7FFF;  // clear flag; remaining value is true max
    for (uint16_t i = 3; i < store; i++) {
      if (temperature_raw[i] >= 0x0A00) {
        temperature_raw[i] -= 0x0A00;
      }
    }
  }
}

void SmartEq453Battery::decode_pid29(const uint8_t* p, uint16_t len) {
  // P[2..3]: isolation resistance, signed BE16 (kΩ)
  // P[4]:    isolation / DC-fault status flags (bit meanings unresolved)
  if (len < 5)
    return;
  isolation_kohm = sbe16(p + 2);
  isolation_flags = p[4];
}

void SmartEq453Battery::decode_pid41_42(const uint8_t* p, uint16_t len, uint8_t block) {
  // 48 cells per block, each BE16 / 1024.0 V.
  // Cell data starts at P[3].
  // block 0 = cells 0..47,  block 1 = cells 48..95
  if (len < 3 + 48 * 2)
    return;
  const uint8_t base = block * 48;
  for (uint8_t i = 0; i < 48; i++) {
    cell_voltage_raw[base + i] = be16(p + 3 + i * 2);
  }
}

void SmartEq453Battery::decode_pid10_11(const uint8_t* p, uint16_t len, uint8_t block) {
  // 48 cells per block, each BE16.
  // Physical unit (Ω or mΩ) is not yet verified — store raw only.
  if (len < 3 + 48 * 2)
    return;
  const uint8_t base = block * 48;
  for (uint8_t i = 0; i < 48; i++) {
    cell_resist_raw[base + i] = be16(p + 3 + i * 2);
  }
}

void SmartEq453Battery::decode_pid16(const uint8_t* p, uint16_t len) {
  // P[3..P[98]]: 96 one-byte per-cell balancing values
  // P[99..100]:  overall balancing state, BE16
  // Do not assume each per-cell byte is Boolean.
  if (len < 101)
    return;
  memcpy(balancing_raw, p + 3, 96);
  balancing_state_raw = be16(p + 99);
}

void SmartEq453Battery::decode_pid25(const uint8_t* p, uint16_t len) {
  // P[2]: SOC recalibration state
  // P[3..4]: internal mean SOC value, BE16 / 16 (candidate)
  if (len < 5)
    return;
  // Store for later: mean_internal_soc = be16(p+3) / 16.0
  // No dedicated field exposed yet; retain raw for cross-check.
}

void SmartEq453Battery::decode_pid61(const uint8_t* p, uint16_t len) {
  // Layout differs between BMS hardware/software revisions.
  // Store entire raw payload; do not substitute calculated values for BMS-reported SOH.
  const uint16_t store = len < sizeof(pid61_raw) ? len : sizeof(pid61_raw);
  memcpy(pid61_raw, p, store);
  pid61_len = static_cast<uint8_t>(store);

  // Candidate decode from OVMS Smart EQ (verify against actual BMS revision):
  // data[6..7] = full capacity field, raw * 10 / 3600 Ah
  // data[8]    = SOH, raw / 2 percent
  // data[9..12] = BMS mileage, BE32 km
  // Do not use these as authoritative without revision verification.
}

void SmartEq453Battery::decode_pid0B(const uint8_t* p, uint16_t len) {
  // P[2..3]: measured capacity BE16
  // Candidate: capacity_as = raw / 10.0 As → / 3600.0 = Ah
  // Must verify against ~52 Ah expected; retain raw if implausible.
  if (len < 4)
    return;
  capacity_measured_raw = be16(p + 2);
}

void SmartEq453Battery::decode_pid02(const uint8_t* p, uint16_t len) {
  // HV contactor cycle counter — safety critical.
  // Read at session start and end; abort if it unexpectedly decreases.
  // Exact byte layout: treat response bytes P[2..] as a counter.
  // Until physical validation, store the first 4 bytes as BE32.
  if (len < 6)
    return;
  const uint32_t value = be32(p + 2);

  if (!contactor_counter_valid) {
    contactor_counter_initial = value;
    contactor_counter_current = value;
    contactor_counter_valid = true;
  } else {
    contactor_counter_current = value;
    check_contactor_counter_safety();
  }
}

void SmartEq453Battery::decode_pid80(const uint8_t* p, uint16_t len) {
  const uint16_t store = len < sizeof(bms_id_raw) ? len : sizeof(bms_id_raw);
  memcpy(bms_id_raw, p, store);
  bms_id_len = static_cast<uint8_t>(store);
}

void SmartEq453Battery::decode_pidEF(const uint8_t* p, uint16_t len) {
  const uint16_t store = len < sizeof(bms_pn_raw) ? len : sizeof(bms_pn_raw);
  memcpy(bms_pn_raw, p, store);
  bms_pn_len = static_cast<uint8_t>(store);
}

void SmartEq453Battery::decode_pidA0(const uint8_t* p, uint16_t len) {
  const uint16_t store = len < sizeof(batt_serial_raw) ? len : sizeof(batt_serial_raw);
  memcpy(batt_serial_raw, p, store);
  batt_serial_len = static_cast<uint8_t>(store);
}

void SmartEq453Battery::decode_pid_22_0304(const uint8_t* p, uint16_t len) {
  // Production date: P[4]=year representation, P[5]=month, P[6]=day
  // Year encoding may be full year, offset or BCD — log raw, do not convert.
  const uint16_t store = len < sizeof(prod_date_raw) ? len : sizeof(prod_date_raw);
  memcpy(prod_date_raw, p, store);
  prod_date_len = static_cast<uint8_t>(store);
}

// ── Safety: contactor counter check ──────────────────────────────────────────
// ── Broadcast frame decoders ──────────────────────────────────────────────────

void SmartEq453Battery::decode_bcast_0x0C6(const uint8_t* d, uint8_t dlc) {
  // bytes 0-1: pack voltage, BE16 * 0.01 V → dV = raw / 10
  // bytes 4-5: DC link voltage, same scale
  // Confirmed from trace: 0x7FE1 = 32737 * 0.01 = 327.37V (plausible ~85% SOC)
  // Current (bytes 2-3) not decoded here: at idle the raw is ~0x8000 (offset centre)
  // and the scaling is not confirmed from the trace alone.
  const uint16_t v_raw = be16(d);
  const uint16_t dc_raw = be16(d + 4);

  // Sanity: reject clearly invalid values (< 200V or > 450V)
  if (v_raw < 20000 || v_raw > 45000)
    return;

  bcast_voltage_raw = v_raw;
  bcast_dc_link_raw = dc_raw;
  bcast_voltage_valid = true;
  last_bcast_ms = millis();
}

void SmartEq453Battery::decode_bcast_0x12E(const uint8_t* d, uint8_t dlc) {
  // byte 0: SOC, raw / 2.0 = %  →  pptt = raw * 50
  // Confirmed from trace: byte0 = 188-190 when estimated SOC ~94-95%.
  // Scale candidate — verify against displayed SOC during physical validation.
  const uint8_t raw = d[0];

  // Reject out-of-range (max raw = 200 → 100%, min sane = 0)
  if (raw > 200)
    return;

  bcast_soc_raw = raw;
  bcast_soc_valid = true;
}

void SmartEq453Battery::decode_bcast_0x090(const uint8_t* d, uint8_t dlc) {
  // Byte 0: DC link charge percentage (0-100%) or 0xFF (capacitors not charged).
  // Byte 4 is a channel index (0x00-0xF0). All 16 channels scan during precharge,
  // so the latest reading is an estimate of the current DC link charge level.
  const uint8_t pct = d[0];
  if (pct == 0xFF) {
    bcast_precharge_valid = false;
    bcast_precharge_pct = 0xFF;
    return;
  }
  if (pct > 100)
    return;
  bcast_precharge_pct = pct;
  bcast_precharge_valid = true;
  datalayer_extended.smart453.precharge_pct = pct;
}

void SmartEq453Battery::decode_bcast_0x3B7(const uint8_t* d, uint8_t dlc) {
  // Byte 0: temperature candidate, raw / 2.0 = °C.
  // Trace: 74-75 → 37.0-37.5°C throughout startup. Slowly oscillates by ±0.5°C.
  const uint8_t raw = d[0];
  if (raw == 0xFF)
    return;
  bcast_temp_raw = raw;
  bcast_temp_valid = true;
}

void SmartEq453Battery::check_contactor_counter_safety() {
  if (!contactor_counter_valid)
    return;
  if (contactor_counter_current < contactor_counter_initial) {
    // Unexpected decrease — abort all diagnostic activity immediately.
    // This is a hard safety abort: the BMS may have a counter decrement bug.
    state = Smart453State::FAULT;
    if (allows_contactor_closing) {
      *allows_contactor_closing = false;
    }
    set_event(EVENT_STALE_VALUE, 0x02);
  }
}

// ── update_values() ────────────────────────────────────────────────────────────
void SmartEq453Battery::update_values() {
  const unsigned long now = millis();
  const bool bcast_fresh = bcast_voltage_valid && (now - last_bcast_ms < BCAST_STALE_MS);
  const bool uds_voltage_fresh = (pack_sum_voltage_raw > 0);
  const bool uds_soc_fresh = (soc_max_raw > 0);

  // ── Pack voltage ─────────────────────────────────────────────────────────
  // Priority: UDS PID 0x07 cell-sum > broadcast 0x0C6.
  // Broadcast is available from the very first 0x0C6 frame (t+10ms);
  // UDS is available only after the first successful poll cycle (~500ms).
  if (uds_voltage_fresh) {
    // UDS: raw / 64.0 V → dV = raw * 10 / 64
    datalayer_battery->status.voltage_dV =
        static_cast<uint16_t>((static_cast<uint32_t>(pack_sum_voltage_raw) * 10) / 64);
  } else if (bcast_fresh) {
    // Broadcast 0x0C6: raw * 0.01 V → dV = raw / 10
    datalayer_battery->status.voltage_dV = static_cast<uint16_t>(bcast_voltage_raw / 10);
  }

  // DC link voltage from broadcast 0x0C6 bytes 4-5 → extended datalayer for display
  if (bcast_voltage_valid) {
    datalayer_extended.smart453.bcast_dc_link_dV = static_cast<uint16_t>(bcast_dc_link_raw / 10);
  }

  // ── Current: UDS only (broadcast current scaling not confirmed from trace) ─
  // raw / 32.0 A → × 10 dA = raw * 10 / 32
  datalayer_battery->status.current_dA = static_cast<int16_t>((static_cast<int32_t>(pack_current_raw) * 10) / 32);

  // ── Cell voltages: raw / 1024.0 V → mV = raw * 1000 / 1024
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    if (cell_voltage_raw[i] > 0) {
      datalayer_battery->status.cell_voltages_mV[i] =
          static_cast<uint16_t>((static_cast<uint32_t>(cell_voltage_raw[i]) * 1000) / 1024);
    }
  }

  // ── Cell min/max from PID 0x07 (faster than cell scan)
  if (cell_min_raw > 0) {
    datalayer_battery->status.cell_min_voltage_mV =
        static_cast<uint16_t>((static_cast<uint32_t>(cell_min_raw) * 1000) / 1024);
  }
  if (cell_max_raw > 0) {
    datalayer_battery->status.cell_max_voltage_mV =
        static_cast<uint16_t>((static_cast<uint32_t>(cell_max_raw) * 1000) / 1024);
  }

  // ── Temperatures ─────────────────────────────────────────────────────────
  // PID 0x04 indices (ED4scan + OVMS confirmed):
  //   [0] = overall pack min, [1] = overall pack max (bit 15 cleared by decoder)
  //   [3..29] = 27 individual sensor readings (not yet mapped to cell array)
  // Use BMS-reported min/max directly — they are pre-computed over all 27 sensors.
  const bool uds_temp_fresh = (temperature_raw[0] > 0 || temperature_raw[1] > 0);
  if (temperature_raw[0] > 0) {
    datalayer_battery->status.temperature_min_dC =
        static_cast<int16_t>((static_cast<int32_t>(temperature_raw[0]) * 10) / 64);
  }
  if (temperature_raw[1] > 0) {
    datalayer_battery->status.temperature_max_dC =
        static_cast<int16_t>((static_cast<int32_t>(temperature_raw[1]) * 10) / 64);
  }

  // Broadcast temperature fallback: use 0x3B7 when no UDS temperatures received yet.
  // raw / 2 °C  →  dC = raw * 5. Candidate scale — confirm against known reference.
  if (!uds_temp_fresh && bcast_temp_valid) {
    const int16_t t_dC = static_cast<int16_t>(bcast_temp_raw) * 5;
    datalayer_battery->status.temperature_min_dC = t_dC;
    datalayer_battery->status.temperature_max_dC = t_dC;
  }

  // ── SOC ─────────────────────────────────────────────────────────────────
  // Priority: UDS PID 0x08 > broadcast 0x12E.
  // Broadcast gives a coarse reading (0.5% steps) immediately on bus-up.
  // UDS gives a finer reading once polling starts.
  if (uds_soc_fresh) {
    // raw / 16.0 → pptt: raw * 100 / 16, clamped to 0–10000
    uint32_t soc_pptt = (static_cast<uint32_t>(soc_max_raw) * 100) / 16;
    if (soc_pptt > 10000)
      soc_pptt = 10000;
    datalayer_battery->status.real_soc = static_cast<uint16_t>(soc_pptt);
  } else if (bcast_soc_valid) {
    // broadcast 0x12E byte 0: raw / 2 = %  →  pptt = raw * 50
    // Confirmed from trace: raw=188-190 ≙ 94-95% SOC
    uint32_t soc_pptt = static_cast<uint32_t>(bcast_soc_raw) * 50;
    if (soc_pptt > 10000)
      soc_pptt = 10000;
    datalayer_battery->status.real_soc = static_cast<uint16_t>(soc_pptt);
  }

  // ── SOH: from PID 0x61 candidate (revision-specific — do not trust blindly)
  // datalayer_battery->status.soh_pptt = ...;  // left at default until verified

  // ── Charge/discharge limits from PID 0x01
  // Positive = charge limit, negative = discharge limit (sign convention TBD)
  if (charge_limit_raw > 0) {
    // A / 1024.0 → W at pack voltage
    const uint32_t v_dV = datalayer_battery->status.voltage_dV;
    if (v_dV > 0) {
      datalayer_battery->status.max_charge_power_W =
          static_cast<uint32_t>((static_cast<int64_t>(charge_limit_raw) * v_dV) / (1024 * 10));
    }
    datalayer_battery->status.max_charge_current_dA = static_cast<uint16_t>(charge_limit_raw * 10 / 1024);
  }
  if (discharge_limit_raw < 0) {
    const uint32_t v_dV = datalayer_battery->status.voltage_dV;
    if (v_dV > 0) {
      datalayer_battery->status.max_discharge_power_W =
          static_cast<uint32_t>((static_cast<int64_t>(-discharge_limit_raw) * v_dV) / (1024 * 10));
    }
    datalayer_battery->status.max_discharge_current_dA = static_cast<uint16_t>((-discharge_limit_raw) * 10 / 1024);
  }

  // ── Remaining capacity estimate
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000.0) * datalayer_battery->info.total_capacity_Wh);

  // ── Balancing status (raw values, interpretation pending)
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    datalayer_battery->status.cell_balancing_status[i] = (balancing_raw[i] != 0);
  }

  // ── BMS status (precharge_control.cpp uses this to gate software precharge) ─
  // BMS_ACTIVE  = contactors confirmed closed (PID 0x07 contactor_state == 2)
  // BMS_STANDBY = bus alive, handshake in progress (C3–C7 or precharging)
  // BMS_FAULT   = latching; only set, never cleared here
  // BMS_DISCONNECTED = no meaningful state yet
  if (state == Smart453State::FAULT) {
    datalayer_battery->status.real_bms_status = BMS_FAULT;
  } else if (datalayer_battery->status.real_bms_status != BMS_FAULT) {
    if (contactor_state == 2) {
      datalayer_battery->status.real_bms_status = BMS_ACTIVE;
    } else {
      switch (state) {
        case Smart453State::BMS_RESPONDING:
        case Smart453State::STATE_C3:
        case Smart453State::STATE_C4:
        case Smart453State::STATE_C5:
        case Smart453State::STATE_C6:
        case Smart453State::STATE_C7_READY:
        case Smart453State::WAIT_RELAY_PERMIT:
        case Smart453State::PRECHARGING:
        case Smart453State::SHUTDOWN_REQUESTED:
        case Smart453State::SHUTTING_DOWN:
          datalayer_battery->status.real_bms_status = BMS_STANDBY;
          break;
        default:  // POWER_OFF, LV_POWERED, WAKE_REQUESTED, WAIT_BMS_RESPONSE
          datalayer_battery->status.real_bms_status = BMS_DISCONNECTED;
          break;
      }
    }
  }

  // ── Isolation monitoring ──────────────────────────────────────────────────
  // PID 0x29: fire BATTERY_CAUTION when BMS reports any isolation flags.
  // Low absolute threshold (<100 kΩ) is an additional guard.
  if (isolation_flags != 0) {
    set_event(EVENT_BATTERY_CAUTION, isolation_flags);
  }
  if (isolation_kohm > 0 && isolation_kohm < 100) {
    set_event(EVENT_BATTERY_CAUTION, static_cast<uint8_t>(isolation_kohm));
  }

  // ── HVIL monitoring ───────────────────────────────────────────────────────
  // Only report after PID 0x07 has been received (last_pid07_ms > 0),
  // to avoid false positives from the default zero value.
  if (last_pid07_ms > 0) {
    if (hv_plug_interlock == 0) {
      set_event(EVENT_HVIL_FAILURE, 0);
    } else {
      clear_event(EVENT_HVIL_FAILURE);
    }
  }

  // ── Contactor permission
  // Only set true when ALL safety conditions are met AND
  // SMART453_ALLOW_CONTACTOR_CONTROL == 1.
#if SMART453_ALLOW_CONTACTOR_CONTROL
  const bool safety_ok = (hv_plug_interlock == 1) && (service_disconnect == 1) && (fusi_raw == 0) &&
                         (safety_mode == 0) && (relay_permit == 1) && (contactor_counter_valid) &&
                         (state == Smart453State::STATE_C7_READY || state == Smart453State::CONTACTORS_CLOSED) &&
                         // PID 0x07 data must not be stale
                         (millis() - last_pid07_ms < 2000UL);

  if (allows_contactor_closing) {
    *allows_contactor_closing = safety_ok;
  }
#else
  if (allows_contactor_closing) {
    *allows_contactor_closing = false;
  }
#endif
}

// ── transmit_can() ────────────────────────────────────────────────────────────
void SmartEq453Battery::transmit_can(unsigned long currentMillis) {
  // ── 1. Keepalive 0x634 at 100 ms ────────────────────────────────────────
  if (currentMillis - last_keepalive_ms >= KEEPALIVE_MS) {
    last_keepalive_ms = currentMillis;
    // Transmit awake-mode keepalive. The trace shows 0x80 0x00 0x32 0x00
    // at ~100 ms in the already-awake state. This is the only periodic frame
    // we transmit — all other frames in the trace are passively observed.
    transmit_can_frame(&frame_634_awake);
  }

  // ── 2. ISO-TP timeout guard ──────────────────────────────────────────────
  check_isotp_timeout(currentMillis);

  // ── 3. State machine update ──────────────────────────────────────────────
  update_state_machine(currentMillis);

  // ── 4. Diagnostic scheduler ─────────────────────────────────────────────
  if (poll_is_safe_to_send()) {
    schedule_next_poll(currentMillis);
  }
}

// ── State machine ─────────────────────────────────────────────────────────────
void SmartEq453Battery::update_state_machine(unsigned long now) {
  (void)now;

  switch (state) {
    case Smart453State::LV_POWERED:
      // Once we start seeing CAN traffic, advance to BMS_RESPONDING.
      if (datalayer_battery->status.CAN_battery_still_alive == CAN_STILL_ALIVE) {
        state = Smart453State::BMS_RESPONDING;
      }
      break;

    case Smart453State::BMS_RESPONDING:
      // Track 0x350 vehicle state
      if (vehicle_state_byte == 0xC3)
        state = Smart453State::STATE_C3;
      else if (vehicle_state_byte == 0xC4)
        state = Smart453State::STATE_C4;
      else if (vehicle_state_byte == 0xC5)
        state = Smart453State::STATE_C5;
      else if (vehicle_state_byte == 0xC6)
        state = Smart453State::STATE_C6;
      else if (vehicle_state_byte == 0xC7)
        state = Smart453State::STATE_C7_READY;
      break;

    case Smart453State::STATE_C3:
    case Smart453State::STATE_C4:
    case Smart453State::STATE_C5:
      if (vehicle_state_byte == 0xC6)
        state = Smart453State::STATE_C6;
      else if (vehicle_state_byte == 0xC7)
        state = Smart453State::STATE_C7_READY;
      else if (vehicle_state_byte == 0xC0)
        state = Smart453State::BMS_RESPONDING;
      break;

    case Smart453State::STATE_C6:
      // Precharge window: observe but do not act on 0x1F6 or 0x186 alone.
      // The BMS contactor state from PID 0x07 is the authoritative source.
      if (vehicle_state_byte == 0xC7) {
        // Additional confirmation from 0x186 and PID 0x07 required.
        if (f186_ready && contactor_state == 2) {
          state = Smart453State::STATE_C7_READY;
        } else {
          // C7 without full BMS confirmation — wait for PID 0x07
          state = Smart453State::STATE_C7_READY;  // still advance but keep contactor locked
        }
      } else if (vehicle_state_byte == 0xC3 || vehicle_state_byte == 0xC0) {
        state = Smart453State::BMS_RESPONDING;
      }
      break;

    case Smart453State::STATE_C7_READY:
      // Running state — continue polling
      if (vehicle_state_byte == 0xC0 || vehicle_state_byte == 0xC9) {
        state = Smart453State::BMS_RESPONDING;
      }
      break;

    case Smart453State::FAULT:
      // Latching fault — do not automatically recover
      if (allows_contactor_closing)
        *allows_contactor_closing = false;
      break;

    default:
      break;
  }
}

// ── Diagnostic scheduler ──────────────────────────────────────────────────────
bool SmartEq453Battery::poll_is_safe_to_send() const {
  // Only send a new request when:
  // 1. No ISO-TP transaction is in progress.
  // 2. The BMS has been observed on the bus.
  // 3. Not in FAULT state.
  return !isotp_busy && (state != Smart453State::FAULT) && (state != Smart453State::LV_POWERED) &&
         (state != Smart453State::POWER_OFF);
}

void SmartEq453Battery::schedule_next_poll(unsigned long now) {
  // Phase 0: startup PIDs — run them once in order before steady-state.
  if (poll_phase == 0) {
    if (startup_pid_index < NUM_STARTUP_PIDS) {
      send_diag_request(0x21, STARTUP_PIDS[startup_pid_index]);
      startup_pid_index++;
      return;
    }
    // After startup PIDs, also request service 0x22 production date
    if (startup_pid_index == NUM_STARTUP_PIDS) {
      send_diag_request_22(0x03, 0x04);
      startup_pid_index++;
      return;
    }
    poll_phase = 1;  // Switch to steady-state
  }

  // Phase 1: steady-state polling by interval bucket.
  // Fast: 0x07, 0x01  (500 ms)
  if (now - last_poll_fast_ms >= POLL_FAST_MS) {
    last_poll_fast_ms = now;
    static uint8_t fast_idx = 0;
    const uint8_t fast_pids[] = {0x07, 0x01};
    send_diag_request(0x21, fast_pids[fast_idx % 2]);
    fast_idx++;
    return;
  }

  // Medium: 0x08, 0x29  (1000 ms)
  if (now - last_poll_medium_ms >= POLL_MEDIUM_MS) {
    last_poll_medium_ms = now;
    static uint8_t med_idx = 0;
    const uint8_t med_pids[] = {0x08, 0x29};
    send_diag_request(0x21, med_pids[med_idx % 2]);
    med_idx++;
    return;
  }

  // Slow: 0x41, 0x42, 0x04  (5000 ms)
  if (now - last_poll_slow_ms >= POLL_SLOW_MS) {
    last_poll_slow_ms = now;
    static uint8_t slow_idx = 0;
    const uint8_t slow_pids[] = {0x41, 0x42, 0x04};
    send_diag_request(0x21, slow_pids[slow_idx % 3]);
    slow_idx++;
    return;
  }

  // Very slow: 0x10, 0x11, 0x16, 0x25, 0x61, 0x0B  (30000 ms)
  if (now - last_poll_vslow_ms >= POLL_VSLOW_MS) {
    last_poll_vslow_ms = now;
    static uint8_t vslow_idx = 0;
    const uint8_t vslow_pids[] = {0x10, 0x11, 0x16, 0x25, 0x61, 0x0B};
    send_diag_request(0x21, vslow_pids[vslow_idx % 6]);
    vslow_idx++;
    return;
  }
}

void SmartEq453Battery::send_diag_request(uint8_t service, uint8_t pid) {
  frame_diag_request.data.u8[0] = 0x02;
  frame_diag_request.data.u8[1] = service;
  frame_diag_request.data.u8[2] = pid;
  frame_diag_request.data.u8[3] = 0x00;
  frame_diag_request.data.u8[4] = 0x00;
  frame_diag_request.data.u8[5] = 0x00;
  frame_diag_request.data.u8[6] = 0x00;
  frame_diag_request.data.u8[7] = 0x00;
  isotp_busy = true;
  isotp_pid = pid;
  isotp_rxlen = 0;
  isotp_request_ts = millis();
  transmit_can_frame(&frame_diag_request);
}

void SmartEq453Battery::send_diag_request_22(uint8_t pid_high, uint8_t pid_low) {
  frame_diag_request_22.data.u8[0] = 0x03;
  frame_diag_request_22.data.u8[1] = 0x22;
  frame_diag_request_22.data.u8[2] = pid_high;
  frame_diag_request_22.data.u8[3] = pid_low;
  isotp_busy = true;
  isotp_pid = 0xF0;  // production date uses pseudo-PID 0xF0 for dispatch
  isotp_rxlen = 0;
  isotp_request_ts = millis();
  transmit_can_frame(&frame_diag_request_22);
}

void SmartEq453Battery::check_isotp_timeout(unsigned long now) {
  if (!isotp_busy)
    return;
  if (now - isotp_request_ts > ISOTP_TIMEOUT_MS) {
    isotp_busy = false;
    isotp_rxlen = 0;
    datalayer_battery->status.CAN_error_counter++;
  }
}
