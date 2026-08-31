#include "GROWATT-LV-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

// Helpers (Growatt LV protocol is big-endian, unlike Pylontech's little-endian)
static inline uint16_t read_u16_be(const CAN_frame& f, int idx) {
  return (uint16_t)((f.data.u8[idx] << 8) | f.data.u8[idx + 1]);
}

static inline int16_t read_s16_be(const CAN_frame& f, int idx) {
  return (int16_t)read_u16_be(f, idx);
}

void GrowattLvBattery::setup(void) {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer.battery.info.chemistry = LFP;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;

  // Allow contactor closing once the BMS itself reports charge/discharge enabled.
  datalayer.system.status.battery_allows_contactor_closing = false;
}

void GrowattLvBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x311: {  // Voltage/current limits + status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      cvl_dV = read_u16_be(rx_frame, 0);  // 0.1V
      ccl_dA = read_u16_be(rx_frame, 2);  // 0.1A
      dcl_dA = read_u16_be(rx_frame, 4);  // 0.1A
      status_word = read_u16_be(rx_frame, 6);
      have_311 = true;

      // Status bits live in the low byte of the big-endian pair (byte 7):
      // bit 5 = discharge enable, bit 6 = charge enable. This is the
      // authoritative source for enable state once seen - see the header
      // comment for why 0x319's own enable bits aren't trusted instead.
      discharge_en = (status_word & 0x0020) != 0;
      charge_en = (status_word & 0x0040) != 0;
      break;
    }

    case 0x312: {  // Protection/warning flags + live pack count
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      prot1 = rx_frame.data.u8[0];
      prot2 = rx_frame.data.u8[1];
      warn1 = rx_frame.data.u8[2];
      warn2 = rx_frame.data.u8[3];
      if (rx_frame.data.u8[4] >= 1 && rx_frame.data.u8[4] <= 16) {
        pack_count = rx_frame.data.u8[4];
      }
      have_312 = true;
      break;
    }

    case 0x313: {  // Pack voltage/current/temperature/SOC/SOH
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      pack_v_cV = read_s16_be(rx_frame, 0);   // 0.01V - confirmed against real hardware
      current_dA = read_s16_be(rx_frame, 2);  // + = charging, same sign as datalayer convention
      temp_dC = read_s16_be(rx_frame, 4);
      soc_pct = rx_frame.data.u8[6] > 100 ? 100 : rx_frame.data.u8[6];
      uint8_t soh = (uint8_t)(rx_frame.data.u8[7] & 0x7F);  // bit 7 is a separate flag
      soh_pct = (soh == 0 || soh > 100) ? 100 : soh;
      have_313 = true;
      break;
    }

    case 0x314:  // Remaining/full capacity, delta-V, cycle count - logging only for now
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;

    case 0x319:  // Force-charge request + fallback enable bits
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      force_chg_2 = (rx_frame.data.u8[0] & 0x04) != 0;  // bit 2
      force_chg_1 = (rx_frame.data.u8[0] & 0x08) != 0;  // bit 3
      // Only used before the first 0x311 arrives; a real capture showed this
      // frame's own enable bits (byte 0 bits 5/6) don't track live state.
      if (!have_311) {
        discharge_en = (rx_frame.data.u8[0] & 0x20) != 0;  // bit 5
        charge_en = (rx_frame.data.u8[0] & 0x40) != 0;     // bit 6
      }
      break;

    case 0x315:  // Cell voltages 1-4 (mV)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer.battery.status.cell_voltages_mV[0] = read_u16_be(rx_frame, 0);
      datalayer.battery.status.cell_voltages_mV[1] = read_u16_be(rx_frame, 2);
      datalayer.battery.status.cell_voltages_mV[2] = read_u16_be(rx_frame, 4);
      datalayer.battery.status.cell_voltages_mV[3] = read_u16_be(rx_frame, 6);
      break;

    case 0x316:  // Cell voltages 5-8 (mV)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer.battery.status.cell_voltages_mV[4] = read_u16_be(rx_frame, 0);
      datalayer.battery.status.cell_voltages_mV[5] = read_u16_be(rx_frame, 2);
      datalayer.battery.status.cell_voltages_mV[6] = read_u16_be(rx_frame, 4);
      datalayer.battery.status.cell_voltages_mV[7] = read_u16_be(rx_frame, 6);
      break;

    case 0x317:  // Cell voltages 9-12 (mV)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer.battery.status.cell_voltages_mV[8] = read_u16_be(rx_frame, 0);
      datalayer.battery.status.cell_voltages_mV[9] = read_u16_be(rx_frame, 2);
      datalayer.battery.status.cell_voltages_mV[10] = read_u16_be(rx_frame, 4);
      datalayer.battery.status.cell_voltages_mV[11] = read_u16_be(rx_frame, 6);
      break;

    case 0x318:  // Cell voltages 13-16 (mV)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer.battery.status.cell_voltages_mV[12] = read_u16_be(rx_frame, 0);
      datalayer.battery.status.cell_voltages_mV[13] = read_u16_be(rx_frame, 2);
      datalayer.battery.status.cell_voltages_mV[14] = read_u16_be(rx_frame, 4);
      datalayer.battery.status.cell_voltages_mV[15] = read_u16_be(rx_frame, 6);
      break;

    // 0x320 (manufacturer/version/date), 0x321 (update status) and the
    // undocumented IDs some GBLI6532 packs also emit (0x322-0x330) are
    // currently ignored - none are needed to drive an inverter safely.
    default:
      break;
  }
}

void GrowattLvBattery::update_values() {
  datalayer.battery.status.voltage_dV = pack_v_cV / 10;  // 0.01V -> 0.1V
  datalayer.battery.status.current_dA = current_dA;

  datalayer.battery.status.real_soc = (uint16_t)soc_pct * 100;
  datalayer.battery.status.soh_pptt = (uint16_t)soh_pct * 100;

  // Per-pack ceiling scales with the live pack count from 0x312 - a fixed
  // single-pack ceiling would wrongly halve a 2-pack system's real
  // capability. Belt-and-braces: also zero the limit outright whenever the
  // BMS itself has charge/discharge disabled, since PYLON-LV-CAN's own
  // enable logic is derived from SOC/voltage windows and doesn't consult
  // this driver's enable state directly - the safety guarantee has to come
  // from the current magnitude being genuinely zero, not just from a flag.
  const uint16_t ceiling_dA = (uint16_t)(MAX_CURRENT_PER_PACK_dA * (uint16_t)pack_count);
  datalayer.battery.status.max_charge_current_dA = charge_en ? (ccl_dA > ceiling_dA ? ceiling_dA : ccl_dA) : 0;
  datalayer.battery.status.max_discharge_current_dA = discharge_en ? (dcl_dA > ceiling_dA ? ceiling_dA : dcl_dA) : 0;

  if (have_311) {
    uint16_t cvl = cvl_dV;
    if (cvl > MAX_PACK_VOLTAGE_DV) {
      cvl = MAX_PACK_VOLTAGE_DV;
    }
    if (cvl < MIN_PACK_VOLTAGE_DV) {
      cvl = MIN_PACK_VOLTAGE_DV;
    }
    datalayer.battery.info.max_design_voltage_dV = cvl;
  }

  datalayer.battery.status.temperature_min_dC = temp_dC;
  datalayer.battery.status.temperature_max_dC = temp_dC;

  uint16_t cell_max = 0;
  uint16_t cell_min = 0xFFFF;
  for (int i = 0; i < 16; i++) {
    uint16_t mv = datalayer.battery.status.cell_voltages_mV[i];
    if (mv == 0) {
      continue;  // not yet populated
    }
    if (mv > cell_max) {
      cell_max = mv;
    }
    if (mv < cell_min) {
      cell_min = mv;
    }
  }
  if (cell_max > 0) {
    datalayer.battery.status.cell_max_voltage_mV = cell_max;
    datalayer.battery.status.cell_min_voltage_mV = cell_min;
  }

  // Conservative: only allow contactor closing once the BMS itself has
  // reported live status with no protection flag set. Not conditioned on
  // charge_en/discharge_en together - a BMS legitimately enables only one
  // direction near the SOC extremes, which isn't a fault.
  datalayer.system.status.battery_allows_contactor_closing = have_311 && (prot1 == 0) && (prot2 == 0);
}

void GrowattLvBattery::transmit_can(unsigned long currentMillis) {
  if (currentMillis - previousMillis1000 < INTERVAL_1_S) {
    return;
  }
  previousMillis1000 = currentMillis;

  transmit_can_frame(&BMS_301);
}
