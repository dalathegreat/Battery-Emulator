#include "SOFAR-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
// for memcmp/memcpy used in event-driven 0x35A
#include <cstring>

void SofarInverter::
    update_values() {  // This function maps all the values fetched from battery CAN to the correct CAN messages

  // ----- Frame 0x351 – limits/voltages -----
  // Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOFAR_351.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOFAR_351.data.u8[2] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SOFAR_351.data.u8[3] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOFAR_351.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SOFAR_351.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  // Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOFAR_351.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  // ----- Frame 0x355 – SoC / SoH -----
  // SoC deception only to CAN (we do not touch datalayer)
  uint16_t spoofed_soc = datalayer.battery.status.reported_soc;  // 0..10000 pptt
  if (spoofed_soc >= 10000) {
    spoofed_soc = 9900;  // limit to 99%
  }
  SOFAR_355.data.u8[0] = spoofed_soc / 100;                        // %
  SOFAR_355.data.u8[2] = datalayer.battery.status.soh_pptt / 100;  // %

  // ----- Frame 0x356 – pack voltage/current/temp -----
  // Voltage (e.g. 370.0V -> 3700 dV), Current in dA, Temperature in dC
  SOFAR_356.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SOFAR_356.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  SOFAR_356.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  SOFAR_356.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  SOFAR_356.data.u8[4] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SOFAR_356.data.u8[5] = (datalayer.battery.status.temperature_max_dC >> 8);

  // ===== Frame 0x359 – Battery Pack Config 1 (cyclic, 1 Hz) =====
  // Byte0: number of parallel packs; Byte1: number of modules in series (packs in series);
  // Byte2: CAN version; Byte3: number of cells in series; Byte4..7: reserved (0)
  memset(SOFAR_359.data.u8, 0, 8);
  SOFAR_359.data.u8[0] = 0x02;  // Two parallel packs in system
  SOFAR_359.data.u8[1] = 0x01;  // One module in series (single HV pack)
  SOFAR_359.data.u8[2] = 0x01;  // CAN protocol version = 1
  SOFAR_359.data.u8[3] = 79;    // 79S string

  // ===== Frame 0x35E – Manufacturer Name ASCII (cykliczna, 1 Hz) =====
  memset(SOFAR_35E.data.u8, 0, 8);
  // BatteryType should be max 8 chars, e.g. "AMASS"
  strncpy((char*)SOFAR_35E.data.u8, BatteryType, 8);

  // ===== Frame 0x35F – Battery Type & Capacity (cykliczna, 1 Hz) =====
  // Byte0: Battery type (0x01 = Li-ion), Byte1..3: BMS version (vendor-defined),
  // Byte4..5: Nominal capacity (Ah, uint16), Byte6..7: Manufacturer ID (optional)
  // Capacity calculation (approx): Wh / (Vmax * 0.1)
  if (datalayer.battery.info.max_design_voltage_dV > 20) {  //div0 protection
    calculated_capacity_AH =
        (datalayer.battery.info.reported_total_capacity_Wh / (datalayer.battery.info.max_design_voltage_dV * 0.1));
  }
  // Set type + a simple version triplet 1.0.0 (can be adjusted)
  SOFAR_35F.data.u8[0] = 0x01;  // Li-ion
  SOFAR_35F.data.u8[1] = 0x01;  // BMS ver major
  SOFAR_35F.data.u8[2] = 0x00;  // BMS ver minor
  SOFAR_35F.data.u8[3] = 0x00;  // BMS ver patch
  // Nominal capacity (Ah)
  SOFAR_35F.data.u8[4] = calculated_capacity_AH & 0x00FF;
  SOFAR_35F.data.u8[5] = (calculated_capacity_AH >> 8);
  // Optional manufacturer ID left at 0
  // SOFAR_35F.data.u8[6] = 0x00;
  // SOFAR_35F.data.u8[7] = 0x00;

  // ===== Frame 0x30F – Remote command / enable (event/keep-alive) =====
  // Charge and discharge consent dependent on SoC with hysteresis at 99% soc
  uint8_t soc_percent = spoofed_soc / 100;
  uint8_t enable_flags = 0x00;
  if (soc_percent <= 1) {
    enable_flags = 0x02;  // Only charging allowed
  } else if (soc_percent >= 100) {
    enable_flags = 0x01;  // Only discharge allowed
  } else {
    enable_flags = 0x03;  // Both charge and discharge allowed
  }
  // 0x30F byte0: remote command (0=normal), byte1: enable flags (bit0 discharge, bit1 charge)
  SOFAR_30F.data.u8[0] = 0x00;  // Normal mode (no forced modes)
  SOFAR_30F.data.u8[1] = enable_flags;
}

void SofarInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x605:
    case 0x705: {
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;

      // 0x605 format: Byte0=Target(m), Byte1=Inquiry-ID, Byte2=Sub-target(k)
      // 0x705 format: Byte0=Target(m), Byte1=Inquiry-ID, Byte2=Record(n), Byte3=Sub-target(k)
      const uint8_t target_m = rx_frame.data.u8[0];
      const uint8_t inquiry = rx_frame.data.u8[1];
      const bool is705 = (rx_frame.ID == 0x705);
      const uint8_t record_n = is705 ? rx_frame.data.u8[2] : 0;
      const uint8_t sub_k = is705 ? rx_frame.data.u8[3] : rx_frame.data.u8[2];

      // Respond ONLY if addressed to my PACK ID
      if (target_m != datalayer.battery.settings.sofar_user_specified_battery_id) {
        break;  // not my address → no reply
      }

      // Helper to stamp identifiers into payload (m/k/n) – easy to replace with real fields later
      auto stamp_ids = [&](CAN_frame& f) {
        memset(f.data.u8, 0, 8);
        f.data.u8[0] = datalayer.battery.settings.sofar_user_specified_battery_id;  // PACK ID
        f.data.u8[1] = sub_k;                                                       // Module/Sub-target (k) if applies
        f.data.u8[2] = record_n;  // Record Num (n) – only used for 0x705
      };

      if (rx_frame.ID == 0x605) {
        // Realtime/status inquiries → replies 0x670..0x6C0 depending on Inquiry-ID
        switch (inquiry) {
          case 0x00:
            stamp_ids(SOFAR_670);
            transmit_can_frame(&SOFAR_670);
            break;  // 0x670
          case 0x01:
            stamp_ids(SOFAR_671);
            transmit_can_frame(&SOFAR_671);
            break;  // 0x671
          case 0x02:
            stamp_ids(SOFAR_680);
            transmit_can_frame(&SOFAR_680);
            break;  // 0x680
          case 0x03:
            stamp_ids(SOFAR_681);
            transmit_can_frame(&SOFAR_681);
            break;  // 0x681
          case 0x0A:
            stamp_ids(SOFAR_690);
            transmit_can_frame(&SOFAR_690);
            break;  // 0x690
          case 0x0B:
            stamp_ids(SOFAR_691);
            transmit_can_frame(&SOFAR_691);
            break;  // 0x691
          case 0x0C:
            stamp_ids(SOFAR_6A0);
            transmit_can_frame(&SOFAR_6A0);
            break;  // 0x6A0
          case 0x0D:
            stamp_ids(SOFAR_6B0);
            transmit_can_frame(&SOFAR_6B0);
            break;  // 0x6B0
          case 0x0E:
            stamp_ids(SOFAR_6C0);
            transmit_can_frame(&SOFAR_6C0);
            break;  // 0x6C0
          default:  /* unsupported inquiry → no reply */
            break;
        }
      } else {
        // 0x705: history/fault queries → replies 0x770..0x773 and 0x780..0x784
        switch (inquiry) {
          case 0x00:
            stamp_ids(SOFAR_770);
            transmit_can_frame(&SOFAR_770);
            break;  // 0x770
          case 0x01:
            stamp_ids(SOFAR_771);
            transmit_can_frame(&SOFAR_771);
            break;  // 0x771
          case 0x02:
            stamp_ids(SOFAR_772);
            transmit_can_frame(&SOFAR_772);
            break;  // 0x772
          case 0x03:
            stamp_ids(SOFAR_773);
            transmit_can_frame(&SOFAR_773);
            break;  // 0x773
          case 0x04:
            stamp_ids(SOFAR_780);
            transmit_can_frame(&SOFAR_780);
            break;  // 0x780
          case 0x05:
            stamp_ids(SOFAR_781);
            transmit_can_frame(&SOFAR_781);
            break;  // 0x781
          case 0x06:
            stamp_ids(SOFAR_782);
            transmit_can_frame(&SOFAR_782);
            break;  // 0x782
          case 0x07:
            stamp_ids(SOFAR_783);
            transmit_can_frame(&SOFAR_783);
            break;  // 0x783
          case 0x08:
            stamp_ids(SOFAR_784);
            transmit_can_frame(&SOFAR_784);
            break;  // 0x784
          default:  /* unsupported inquiry → no reply */
            break;
        }
      }
      break;
    }

    default:
      break;
  }
}

void SofarInverter::transmit_can(unsigned long currentMillis) {

  static uint8_t last_35A_payload[8];
  static bool have_last_35A = false;

  if ((unsigned long)(currentMillis - previousMillis1s) >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;
    //Runtime frames – 1000 ms
    transmit_can_frame(&SOFAR_351);
    transmit_can_frame(&SOFAR_355);
    transmit_can_frame(&SOFAR_356);
    //Config/identity frames
    transmit_can_frame(&SOFAR_359);
    transmit_can_frame(&SOFAR_35E);
    transmit_can_frame(&SOFAR_35F);
  }

  // 0x30F – only when active, keep-alive ~1 Hz
  bool remote_cmd_active = (SOFAR_30F.data.u8[0] != 0) || (SOFAR_30F.data.u8[1] != 0) || (SOFAR_30F.data.u8[2] != 0) ||
                           (SOFAR_30F.data.u8[3] != 0);
  if (remote_cmd_active && (unsigned long)(currentMillis - last_command_millis) >= INTERVAL_1_S) {
    last_command_millis = currentMillis;
    transmit_can_frame(&SOFAR_30F);
  }

  // 0x35A – ONLY when content changes (edge-triggered), with light debounce
  if (!have_last_35A || (memcmp(last_35A_payload, SOFAR_35A.data.u8, 8) != 0)) {
    if ((unsigned long)(currentMillis - last_35A_sent_millis) >= INTERVAL_200_MS) {
      memcpy(last_35A_payload, SOFAR_35A.data.u8, 8);
      have_last_35A = true;
      last_35A_sent_millis = currentMillis;
      transmit_can_frame(&SOFAR_35A);
    }
  }
}

bool SofarInverter::setup() {  // Performs one time setup at startup over CAN bus
  // Dynamically set CAN ID according to which battery index we are on
  uint16_t base_offset = (datalayer.battery.settings.sofar_user_specified_battery_id << 12);

  auto init_frame = [&](CAN_frame& frame, uint16_t base_id) {
    frame.FD = false;
    frame.ext_ID = true;
    frame.DLC = 8;
    frame.ID = base_id + base_offset;
    memset(frame.data.u8, 0, 8);
  };

  // Cyclic BMS→PCS frames
  init_frame(SOFAR_351, 0x351);
  init_frame(SOFAR_355, 0x355);
  init_frame(SOFAR_356, 0x356);
  init_frame(SOFAR_30F, 0x30F);
  init_frame(SOFAR_359, 0x359);
  init_frame(SOFAR_35E, 0x35E);
  init_frame(SOFAR_35F, 0x35F);
  init_frame(SOFAR_35A, 0x35A);

  // Example legacy replies (already present)
  init_frame(SOFAR_683, 0x683);
  init_frame(SOFAR_684, 0x684);
  init_frame(SOFAR_685, 0x685);
  init_frame(SOFAR_690, 0x690);

  // Replies for 0x605 (realtime queries)
  init_frame(SOFAR_670, 0x670);
  init_frame(SOFAR_671, 0x671);
  init_frame(SOFAR_680, 0x680);
  init_frame(SOFAR_681, 0x681);
  init_frame(SOFAR_691, 0x691);
  init_frame(SOFAR_6A0, 0x6A0);
  init_frame(SOFAR_6B0, 0x6B0);
  init_frame(SOFAR_6C0, 0x6C0);

  // Replies for 0x705 (history/fault queries)
  init_frame(SOFAR_770, 0x770);
  init_frame(SOFAR_771, 0x771);
  init_frame(SOFAR_772, 0x772);
  init_frame(SOFAR_773, 0x773);
  init_frame(SOFAR_780, 0x780);
  init_frame(SOFAR_781, 0x781);
  init_frame(SOFAR_782, 0x782);
  init_frame(SOFAR_783, 0x783);
  init_frame(SOFAR_784, 0x784);

  return true;
}
