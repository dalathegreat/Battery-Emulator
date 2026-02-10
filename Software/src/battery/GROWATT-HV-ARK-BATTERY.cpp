#include "GROWATT-HV-ARK-BATTERY.h"

#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

// Helpers (Growatt HV protocol is big-endian)
static inline uint16_t read_u16_be(const CAN_frame& f, int idx) {
  return (uint16_t)((f.data.u8[idx] << 8) | f.data.u8[idx + 1]);
}

static inline int16_t read_s16_be(const CAN_frame& f, int idx) {
  return (int16_t)read_u16_be(f, idx);
}

static inline void write_u16_be(CAN_frame& f, int idx, uint16_t v) {
  f.data.u8[idx] = (uint8_t)(v >> 8);
  f.data.u8[idx + 1] = (uint8_t)(v & 0xFF);
}

static inline void write_u32_be(CAN_frame& f, int idx, uint32_t v) {
  f.data.u8[idx] = (uint8_t)(v >> 24);
  f.data.u8[idx + 1] = (uint8_t)((v >> 16) & 0xFF);
  f.data.u8[idx + 2] = (uint8_t)((v >> 8) & 0xFF);
  f.data.u8[idx + 3] = (uint8_t)(v & 0xFF);
}

void GrowattHvArkBattery::setup(void) {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  // Provide sane defaults (will be overwritten by incoming frames once the battery is talking)
  datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = 150;

  // Allow contactor closing once we have a healthy, awake battery.
  datalayer.system.status.battery_allows_contactor_closing = false;
}

void GrowattHvArkBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x3110: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      max_charge_voltage_dV = read_u16_be(rx_frame, 0);
      max_charge_current_dA = (int16_t)read_u16_be(rx_frame, 2);
      max_discharge_current_dA = (int16_t)read_u16_be(rx_frame, 4);

      const uint8_t b6 = rx_frame.data.u8[6];
      const uint8_t b7 = rx_frame.data.u8[7];

      // Table 1 bits (0x3110)
      battery_fault_present = (b7 & (1u << 2)) != 0;
      battery_sleeping = (b7 & (1u << 4)) != 0;
      battery_no_discharge = (b7 & (1u << 5)) != 0;
      battery_no_charge = (b7 & (1u << 6)) != 0;

      (void)b6;  // reserved bits currently unused
      break;
    }

    case 0x3120:
      // Protection + Alarm bitfields (not yet mapped into the generic datalayer)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;

    case 0x3130: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      pack_voltage_dV = read_u16_be(rx_frame, 0);
      pack_current_dA = read_s16_be(rx_frame, 2);
      temp_max_dC = read_s16_be(rx_frame, 4);
      soc_pct = rx_frame.data.u8[6];
      soh_pct = rx_frame.data.u8[7] & 0x7F;  // bit7 is a flag per spec
      break;
    }

    case 0x3140: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      remaining_capacity_10mAh = read_u16_be(rx_frame, 0);
      full_capacity_10mAh = read_u16_be(rx_frame, 2);
      break;
    }

    case 0x3150: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      discharge_cutoff_voltage_dV = read_u16_be(rx_frame, 0);
      // Byte2-3: main control unit temperature (0.1C)
      // We treat it as an additional temperature source; max already comes from 0x3130.
      // int16_t mcu_temp_dC = read_s16_be(rx_frame, 2);
      const uint16_t total_cells = read_u16_be(rx_frame, 4);
      // const uint16_t modules_series = read_u16_be(rx_frame, 6);

      if (total_cells > 0 && total_cells <= 65535) {
        datalayer.battery.info.number_of_cells = total_cells;
      }
      break;
    }

    case 0x3160: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temp_min_dC = read_s16_be(rx_frame, 6);
      break;
    }

    case 0x3190: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_max_mV = read_u16_be(rx_frame, 1);
      cell_min_mV = read_u16_be(rx_frame, 3);
      break;
    }

    case 0x3200: {
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      const uint16_t cell_charge_cutoff_mV = read_u16_be(rx_frame, 6);
      if (cell_charge_cutoff_mV > 0) {
        datalayer.battery.info.max_cell_voltage_mV = cell_charge_cutoff_mV;
      }
      break;
    }

    // Other frames (0x3170..0x3290, 0x3F00) are currently ignored.
    default:
      break;
  }
}

void GrowattHvArkBattery::update_values() {
  // Core measurements
  datalayer.battery.status.voltage_dV = pack_voltage_dV;
  datalayer.battery.status.current_dA = pack_current_dA;
  datalayer.battery.status.temperature_max_dC = temp_max_dC;
  datalayer.battery.status.temperature_min_dC = temp_min_dC;

  // SOC/SOH scaling (0-100 -> 0-10000 / 0-10000)
  datalayer.battery.status.real_soc = (uint16_t)soc_pct * 100;
  datalayer.battery.status.soh_pptt = (uint16_t)soh_pct * 100;

  // Limits
  datalayer.battery.status.max_charge_current_dA = battery_no_charge ? 0 : max_charge_current_dA;
  datalayer.battery.status.max_discharge_current_dA = battery_no_discharge ? 0 : max_discharge_current_dA;

  if (max_charge_voltage_dV > 0) {
    datalayer.battery.info.max_design_voltage_dV = max_charge_voltage_dV;
  }
  if (discharge_cutoff_voltage_dV > 0) {
    datalayer.battery.info.min_design_voltage_dV = discharge_cutoff_voltage_dV;
  }

  // Cell voltages
  datalayer.battery.status.cell_max_voltage_mV = cell_max_mV;
  datalayer.battery.status.cell_min_voltage_mV = cell_min_mV;
  // Populate at least two entries for UIs/logic that expect some per-cell values.
  datalayer.battery.status.cell_voltages_mV[0] = cell_max_mV;
  datalayer.battery.status.cell_voltages_mV[1] = cell_min_mV;

  // Capacity: Use full capacity (bytes 2-3 in 0x3140, in 0.01Ah units) and scale by SOC.
  // Remaining capacity = full_capacity * (SOC% / 100)
  if (pack_voltage_dV > 0 && full_capacity_10mAh > 0) {
    // Calculate total capacity in Wh: (0.01Ah * 10000 counts) * voltage(dV) / 1000
    const uint32_t full_Wh = ((uint32_t)full_capacity_10mAh * (uint32_t)pack_voltage_dV) / 1000u;
    datalayer.battery.info.total_capacity_Wh = full_Wh;

    // Calculate remaining capacity based on SOC: full_capacity * SOC% / 100
    // soc_pct is 0-100, so: (full_capacity_10mAh * soc_pct / 100) gives remaining in 0.01Ah
    const uint32_t rem_capacity_10mAh = ((uint32_t)full_capacity_10mAh * (uint32_t)soc_pct) / 100u;
    const uint32_t rem_Wh = (rem_capacity_10mAh * (uint32_t)pack_voltage_dV) / 1000u;
    datalayer.battery.status.remaining_capacity_Wh = rem_Wh;
  }

  // Power limits (W): dA*dV/100 = (A*10)*(V*10)/100
  const int32_t v_dV = (int32_t)pack_voltage_dV;
  datalayer.battery.status.max_charge_power_W = (int32_t)datalayer.battery.status.max_charge_current_dA * v_dV / 100;
  datalayer.battery.status.max_discharge_power_W =
      (int32_t)datalayer.battery.status.max_discharge_current_dA * v_dV / 100;

  // Contactor closing policy (conservative): allow only when awake and no fault indicated.
  datalayer.system.status.battery_allows_contactor_closing = (!battery_sleeping) && (!battery_fault_present);
}

void GrowattHvArkBattery::transmit_can(unsigned long currentMillis) {
  // Send 1 Hz command set.
  if (currentMillis - previousMillis1000 < INTERVAL_1_S) {
    return;
  }
  previousMillis1000 = currentMillis;

  // --- 0x3010 Heartbeat ---
  send_times++;
  write_u16_be(PCS_3010, 0, send_times);
  PCS_3010.data.u8[2] = 0x00;  // Safety specification / region marker (0 = ignore)

  // --- 0x3020 Control ---
  PCS_3020.data.u8[0] = 0xAA;  // Charging command
  PCS_3020.data.u8[1] = 0xAA;  // Discharging command
  PCS_3020.data.u8[2] = 0x00;  // Shielding external communication failure
  PCS_3020.data.u8[3] = 0x00;  // Clearing battery fault
  PCS_3020.data.u8[4] = 0x00;  // ISO detection command
  PCS_3020.data.u8[7] = 0xAA;  // Wake-up (exit sleeping)

  // --- 0x3030 Time ---
  // If no real RTC is available, we still provide a monotonically increasing counter.
  epoch_time_s++;
  write_u32_be(PCS_3030, 0, epoch_time_s);
  PCS_3030.data.u8[7] = 0x01;  // PCS working status: 01 = Operating

  transmit_can_frame(&PCS_3010);
  transmit_can_frame(&PCS_3020);
  transmit_can_frame(&PCS_3030);
}
