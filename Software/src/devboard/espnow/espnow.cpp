// Battery Emulator telemetry over ESP-NOW. See espnow.h for the wire format.
//
// Design notes:
//  - One frame is emitted per update_espnow() tick, spaced by ESPNOW_FRAME_SPACING_MS.
//    ESP-NOW wants the previous send to complete before the next one is queued; pacing
//    the frames also keeps a three-battery system from bursting a dozen packets at once
//    and hitting ESP_ERR_ESPNOW_NO_MEM.
//  - Everything is serialized into one static buffer with byte-at-a-time little-endian
//    writes. No String, no printf, no dynamic allocation in the send path, and no
//    unaligned struct access.

#include "espnow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <string.h>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../hal/hal.h"
#include "../safety/safety.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#include "../utils/millis64.h"
#include "../wifi/wifi.h"
#include "Arduino.h"

extern const char* version_number;  // defined in Software.cpp

// How often a full round of frames is started.
#define ESPNOW_CYCLE_MS 1000
// Cell voltage / balancing frames are the largest payload; send them less often.
#define ESPNOW_CELL_CYCLE_MS 5000
// Minimum gap between two frames.
#define ESPNOW_FRAME_SPACING_MS 20
// How often the event backlog is re-sent when nothing new has happened. A receiver that
// boots after the emulator gets the history within this long; a new event does not wait
// for it, because a change in the batch contents triggers a send immediately.
#define ESPNOW_EVENT_CYCLE_MS 10000

// Worst case frame: header + the cell count/index records + the full cell voltage array
// + the balancing bitset + slack for record overhead.
static constexpr size_t ESPNOW_TX_BUFFER_SIZE =
    ESPNOW_HEADER_SIZE + 16 + (MAX_AMOUNT_CELLS * 2) + ((MAX_AMOUNT_CELLS + 7) / 8) + 16;

static uint8_t tx_buffer[ESPNOW_TX_BUFFER_SIZE];
static size_t tx_len = 0;
static bool tx_overflow = false;

// ESP-NOW v2 frame limit. Lower this to talk to a receiver that has not raised its own
// receive buffer above the 250 byte default; the cell array is chunked to fit.
#ifndef ESPNOW_MAX_PAYLOAD
#define ESPNOW_MAX_PAYLOAD ESP_NOW_MAX_DATA_LEN_V2
#endif

static constexpr size_t max_payload =
    (ESPNOW_TX_BUFFER_SIZE < (size_t)ESPNOW_MAX_PAYLOAD) ? ESPNOW_TX_BUFFER_SIZE : (size_t)ESPNOW_MAX_PAYLOAD;

// Cells that fit in one ESPNOW_FRAME_CELLS: 2 bytes of voltage plus 1 bit of balancing
// state each (8 cells = 8*2 + 1 = 17 bytes), after the header and four record preambles.
static constexpr uint16_t calc_cells_per_chunk(size_t payload) {
  const size_t overhead = ESPNOW_HEADER_SIZE + 16;
  const size_t room = (payload > overhead) ? (payload - overhead) : 0;
  const size_t fit = room * 8u / 17u;
  return static_cast<uint16_t>(fit < 1u ? 1u : (fit > MAX_AMOUNT_CELLS ? MAX_AMOUNT_CELLS : fit));
}
static constexpr uint16_t cells_per_chunk = calc_cells_per_chunk(max_payload);

static bool espnow_initialized = false;
static uint16_t emulator_id = 0;
static uint8_t num_batteries = 1;

// Send schedule state
enum send_phase_t { PHASE_IDLE, PHASE_SYSTEM, PHASE_BATTERY, PHASE_CELLS, PHASE_EVENTS };
static send_phase_t phase = PHASE_IDLE;
static uint32_t cycle_start_ms = 0;
static uint32_t last_frame_ms = 0;
static uint32_t cells_last_ms = 0;
static bool cells_due = false;
static uint8_t cursor_battery = 0;
static uint16_t cursor_cell = 0;
static uint8_t cursor_event = 0;
// The batch currently being transmitted: the ESPNOW_EVENT_REPLAY most recent occurrences,
// most recent first. Rebuilt at the start of every cycle that sends events.
static EVENTS_ENUM_TYPE event_batch[ESPNOW_EVENT_REPLAY];
static uint8_t event_batch_count = 0;
static uint32_t events_last_ms = 0;
static bool events_due = false;
// Newest timestamp in the last batch built. A newer one means something happened and the
// backlog is sent at once instead of waiting out ESPNOW_EVENT_CYCLE_MS.
static uint64_t event_newest_sent = 0;

// ---------------------------------------------------------------------------------------
// Peers
// ---------------------------------------------------------------------------------------

static const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Resolved destination list. esp_now_send(nullptr, ...) walks the registered peer list,
// but that walk skips broadcast/multicast entries, so a broadcast-only configuration
// would never transmit. Always send to explicit addresses instead.
static uint8_t peer_macs[ESPNOW_MAX_PEERS][6];
static uint8_t peer_count = 0;

// Parses the configured MAC list. Accepts any separators: "AA:BB:CC:DD:EE:FF",
// "aa-bb-cc-dd-ee-ff", "AABBCCDDEEFF", comma / semicolon / whitespace separated.
// Returns the number of peers registered.
static uint8_t register_configured_peers() {
  esp_now_peer_info_t peer = {};
  peer.channel = 0;  // follow the current Wi-Fi channel
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;

  uint8_t mac[6];
  uint8_t nibbles = 0;
  uint8_t added = 0;
  bool truncated = false;
  bool malformed = false;

  const char* p = espnow_peer_macs.c_str();
  // One extra pass with a virtual terminator so a MAC at the very end is flushed.
  for (size_t i = 0;; i++) {
    const char c = p[i];
    int8_t v = -1;
    if (c >= '0' && c <= '9') {
      v = static_cast<int8_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      v = static_cast<int8_t>(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      v = static_cast<int8_t>(c - 'A' + 10);
    }

    if (v >= 0) {
      if (nibbles < 12) {
        if ((nibbles & 1) == 0) {
          mac[nibbles / 2] = static_cast<uint8_t>(v << 4);
        } else {
          mac[nibbles / 2] = static_cast<uint8_t>(mac[nibbles / 2] | v);
        }
      }
      nibbles++;
    } else if ((c == ':' || c == '.') || (c == '-' && nibbles > 0 && nibbles < 12)) {
      // Octet separator inside one address ("AA:BB:.." / "aa-bb-.."): keep accumulating.
      continue;
    } else {
      if (nibbles > 0) {
        if (nibbles == 12) {
          if (added < ESPNOW_MAX_PEERS) {
            memcpy(peer.peer_addr, mac, 6);
            if (esp_now_add_peer(&peer) == ESP_OK) {
              memcpy(peer_macs[added], mac, 6);
              added++;
            } else {
              malformed = true;
            }
          } else {
            truncated = true;
          }
        } else {
          malformed = true;
        }
        nibbles = 0;
      }
      if (c == '\0') {
        break;
      }
    }
  }

  if (malformed) {
    logging.println("ESPNow: ignored malformed entry in the receiver MAC list");
  }
  if (truncated) {
    logging.printf("ESPNow: receiver MAC list truncated to %d peers\n", ESPNOW_MAX_PEERS);
  }
  return added;
}

// ---------------------------------------------------------------------------------------
// TLV writer
// ---------------------------------------------------------------------------------------

static inline void put_u8(uint8_t v) {
  if (tx_len < sizeof(tx_buffer)) {
    tx_buffer[tx_len++] = v;
  } else {
    tx_overflow = true;
  }
}

// Writes the key + tag (+ escaped length) preamble. Returns false if the record does not
// fit in the negotiated payload, so the caller can drop it instead of corrupting a frame.
static bool put_header(uint8_t key, uint8_t type, size_t len) {
  size_t need = 2 + len + (len > 255 ? 2 : (len > ESPNOW_LEN_CODE_MAX_INLINE ? 1 : 0));
  if (tx_len + need > max_payload) {  // max_payload is clamped to the buffer at compile time
    tx_overflow = true;
    return false;
  }
  put_u8(key);
  if (len <= ESPNOW_LEN_CODE_MAX_INLINE) {
    put_u8(static_cast<uint8_t>((type << 5) | len));
  } else if (len <= 255) {
    put_u8(static_cast<uint8_t>((type << 5) | ESPNOW_LEN_CODE_U8));
    put_u8(static_cast<uint8_t>(len));
  } else {
    put_u8(static_cast<uint8_t>((type << 5) | ESPNOW_LEN_CODE_U16));
    put_u8(static_cast<uint8_t>(len & 0xFF));
    put_u8(static_cast<uint8_t>((len >> 8) & 0xFF));
  }
  return true;
}

// Little-endian integer of "len" bytes. Covers every UINT/INT/BOOL key.
static void put_int(uint8_t key, uint8_t type, uint64_t value, uint8_t len) {
  if (!put_header(key, type, len)) {
    return;
  }
  for (uint8_t i = 0; i < len; i++) {
    put_u8(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
  }
}

static inline void put_u8_field(uint8_t key, uint8_t value) {
  put_int(key, ESPNOW_TYPE_UINT, value, 1);
}
static inline void put_u16_field(uint8_t key, uint16_t value) {
  put_int(key, ESPNOW_TYPE_UINT, value, 2);
}
static inline void put_u32_field(uint8_t key, uint32_t value) {
  put_int(key, ESPNOW_TYPE_UINT, value, 4);
}
static inline void put_i16_field(uint8_t key, int16_t value) {
  put_int(key, ESPNOW_TYPE_INT, static_cast<uint16_t>(value), 2);
}
static inline void put_i32_field(uint8_t key, int32_t value) {
  put_int(key, ESPNOW_TYPE_INT, static_cast<uint32_t>(value), 4);
}
static inline void put_bool_field(uint8_t key, bool value) {
  put_int(key, ESPNOW_TYPE_BOOL, value ? 1u : 0u, 1);
}
static inline void put_enum_field(uint8_t key, uint8_t value) {
  put_int(key, ESPNOW_TYPE_UINT, value, 1);
}

static void put_float_field(uint8_t key, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  put_int(key, ESPNOW_TYPE_FLOAT, bits, 4);
}

static void put_bytes_field(uint8_t key, uint8_t type, const void* data, size_t len) {
  if (!put_header(key, type, len)) {
    return;
  }
  const uint8_t* src = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < len; i++) {
    put_u8(src[i]);
  }
}

static void put_str_field(uint8_t key, const char* value) {
  if (value == nullptr) {
    return;
  }
  size_t len = strlen(value);
  if (len > 255) {
    len = 255;
  }
  put_bytes_field(key, ESPNOW_TYPE_STR, value, len);
}

// ---------------------------------------------------------------------------------------
// Frame plumbing
// ---------------------------------------------------------------------------------------

static void begin_frame(uint8_t frame_type, uint8_t battery_id, uint8_t flags) {
  tx_len = 0;
  tx_overflow = false;
  const uint32_t uptime_s = static_cast<uint32_t>(millis64() / 1000u);
  put_u8(ESPNOW_MAGIC_0);
  put_u8(ESPNOW_MAGIC_1);
  put_u8(ESPNOW_PROTOCOL_VERSION);
  put_u8(frame_type);
  put_u8(static_cast<uint8_t>(emulator_id & 0xFF));
  put_u8(static_cast<uint8_t>((emulator_id >> 8) & 0xFF));
  put_u8(battery_id);
  put_u8(flags);
  put_u8(static_cast<uint8_t>(uptime_s & 0xFF));
  put_u8(static_cast<uint8_t>((uptime_s >> 8) & 0xFF));
  put_u8(static_cast<uint8_t>((uptime_s >> 16) & 0xFF));
  put_u8(static_cast<uint8_t>((uptime_s >> 24) & 0xFF));
}

static void end_frame() {
  if (tx_overflow) {
    logging.println("ESPNow: frame truncated, not sent");
    return;
  }
  esp_err_t err = ESP_OK;
  for (uint8_t i = 0; i < peer_count; i++) {
    const esp_err_t e = esp_now_send(peer_macs[i], tx_buffer, tx_len);
    if (e != ESP_OK) {
      err = e;
    }
  }
  if (err != ESP_OK) {
    logging.printf("ESPNow send failed: %s\n", esp_err_to_name(err));
  }
}

// ---------------------------------------------------------------------------------------
// Battery accessors
// ---------------------------------------------------------------------------------------

static const DATALAYER_BATTERY_TYPE* battery_data(uint8_t i) {
  if (i == 0) {
    return &datalayer.battery;
  }
  return (i == 1) ? &datalayer.battery2 : &datalayer.battery3;
}

static Battery* battery_instance(uint8_t i) {
  if (i == 0) {
    return battery;
  }
  return (i == 1) ? battery2 : battery3;
}

static bool battery_is_detected(uint8_t i) {
  if (i == 0) {
    return battery_detected;
  }
  return (i == 1) ? battery2_detected : battery3_detected;
}

// ---------------------------------------------------------------------------------------
// Frame builders
// ---------------------------------------------------------------------------------------

static void send_system_frame() {
  begin_frame(ESPNOW_FRAME_SYSTEM, 0, 0);

  put_str_field(ESPNOW_KEY_FW_VERSION, version_number);
  put_str_field(ESPNOW_KEY_HOSTNAME, WiFi.getHostname());

  uint8_t mac[6] = {0};
  const uint64_t efuse = ESP.getEfuseMac();
  for (uint8_t i = 0; i < 6; i++) {
    mac[i] = static_cast<uint8_t>((efuse >> (8 * i)) & 0xFF);
  }
  put_bytes_field(ESPNOW_KEY_SOURCE_MAC, ESPNOW_TYPE_BYTES, mac, sizeof(mac));

  put_enum_field(ESPNOW_KEY_SYSTEM_STATUS, static_cast<uint8_t>(datalayer.system.status.system_status));
  put_enum_field(ESPNOW_KEY_PAUSE_STATUS, static_cast<uint8_t>(emulator_pause_status));
  put_enum_field(ESPNOW_KEY_EVENT_LEVEL, static_cast<uint8_t>(get_event_level()));
  put_enum_field(ESPNOW_KEY_EMULATOR_STATUS, static_cast<uint8_t>(get_emulator_status()));

  if (datalayer.system.info.CPU_measurement_enabled) {
    put_float_field(ESPNOW_KEY_CPU_TEMP_C, datalayer.system.info.CPU_temperature);
  }
  put_u32_field(ESPNOW_KEY_CPU_FREE_HEAP, datalayer.system.info.CPU_free_heap);
  put_u8_field(ESPNOW_KEY_BATTERY_COUNT, num_batteries);
  if (wifi_connected()) {
    put_int(ESPNOW_KEY_WIFI_RSSI_DBM, ESPNOW_TYPE_INT, static_cast<uint8_t>(static_cast<int8_t>(WiFi.RSSI())), 1);
    const IPAddress ip = WiFi.localIP();
    const uint8_t ip_bytes[4] = {ip[0], ip[1], ip[2], ip[3]};
    put_bytes_field(ESPNOW_KEY_IP_ADDRESS, ESPNOW_TYPE_BYTES, ip_bytes, sizeof(ip_bytes));
    put_str_field(ESPNOW_KEY_SSID, WiFi.SSID().c_str());
  }
  // Live mode bit rather than wifiap_enabled: the AP is torn down on provisioning
  // timeout while the setting stays true, and a receiver showing the setting would lie.
  put_bool_field(ESPNOW_KEY_AP_ACTIVE, (WiFi.getMode() & WIFI_MODE_AP) != 0);
  put_u8_field(ESPNOW_KEY_INVERTER_ALIVE, datalayer.system.status.CAN_inverter_still_alive);
  put_u8_field(ESPNOW_KEY_CONTACTORS, datalayer.system.status.contactors_engaged);
  put_bool_field(ESPNOW_KEY_DC_BUS_LIVE, datalayer.system.status.dc_bus_live);
  put_bool_field(ESPNOW_KEY_EQUIPMENT_STOP, datalayer.system.info.equipment_stop_active);

  end_frame();
}

static void send_battery_frame(uint8_t index) {
  const DATALAYER_BATTERY_TYPE* d = battery_data(index);
  Battery* bat = battery_instance(index);

  begin_frame(ESPNOW_FRAME_BATTERY, static_cast<uint8_t>(index + 1), 0);

  // Nameplate
  put_u8_field(ESPNOW_KEY_NUMBER_OF_CELLS, d->info.number_of_cells);
  put_enum_field(ESPNOW_KEY_CHEMISTRY, static_cast<uint8_t>(d->info.chemistry));
  if (d->info.total_capacity_Wh != 0u) {
    put_u32_field(ESPNOW_KEY_TOTAL_CAPACITY_WH, d->info.total_capacity_Wh);
  }
  put_u32_field(ESPNOW_KEY_REPORTED_CAPACITY_WH, d->info.reported_total_capacity_Wh);
  put_u16_field(ESPNOW_KEY_MAX_DESIGN_VOLTAGE_DV, d->info.max_design_voltage_dV);
  put_u16_field(ESPNOW_KEY_MIN_DESIGN_VOLTAGE_DV, d->info.min_design_voltage_dV);
  put_u16_field(ESPNOW_KEY_MAX_CELL_DESIGN_MV, d->info.max_cell_voltage_mV);
  put_u16_field(ESPNOW_KEY_MIN_CELL_DESIGN_MV, d->info.min_cell_voltage_mV);
  put_u16_field(ESPNOW_KEY_MAX_CELL_DEVIATION_MV, d->info.max_cell_voltage_deviation_mV);

  // Link state is always reported, so a receiver can tell "no battery" from "0 %".
  put_bool_field(ESPNOW_KEY_BATTERY_DETECTED, battery_is_detected(index));
  put_u8_field(ESPNOW_KEY_CAN_ALIVE, d->status.CAN_battery_still_alive);
  put_u16_field(ESPNOW_KEY_CAN_ERROR_COUNTER, d->status.CAN_error_counter);
  put_enum_field(ESPNOW_KEY_REAL_BMS_STATUS, static_cast<uint8_t>(d->status.real_bms_status));
  put_enum_field(ESPNOW_KEY_LED_MODE, static_cast<uint8_t>(d->status.led_mode));

  // Measurements are only emitted once the battery has actually been seen, mirroring the
  // MQTT gating: otherwise the datalayer defaults look like real readings for the first
  // minute after boot.
  if (battery_is_detected(index) && d->status.CAN_battery_still_alive && esp32hal->system_booted_up()) {
    put_u16_field(ESPNOW_KEY_SOC_PPTT, d->status.reported_soc);
    put_u16_field(ESPNOW_KEY_SOC_REAL_PPTT, d->status.real_soc);
    put_u16_field(ESPNOW_KEY_SOH_PPTT, d->status.soh_pptt);
    put_u16_field(ESPNOW_KEY_VOLTAGE_DV, d->status.voltage_dV);
    put_i16_field(ESPNOW_KEY_CURRENT_DA, d->status.current_dA);
    put_i16_field(ESPNOW_KEY_REPORTED_CURRENT_DA, d->status.reported_current_dA);
    put_i32_field(ESPNOW_KEY_ACTIVE_POWER_W, d->status.active_power_W);
    put_u32_field(ESPNOW_KEY_REMAINING_CAPACITY_WH, d->status.remaining_capacity_Wh);
    put_u32_field(ESPNOW_KEY_REPORTED_REMAIN_WH, d->status.reported_remaining_capacity_Wh);
    put_u32_field(ESPNOW_KEY_MAX_CHARGE_POWER_W, d->status.max_charge_power_W);
    put_u32_field(ESPNOW_KEY_MAX_DISCHARGE_POWER_W, d->status.max_discharge_power_W);
    put_u16_field(ESPNOW_KEY_MAX_CHARGE_CURRENT_DA, d->status.max_charge_current_dA);
    put_u16_field(ESPNOW_KEY_MAX_DISCHARGE_CURRENT_DA, d->status.max_discharge_current_dA);
    put_u32_field(ESPNOW_KEY_OVERRIDE_CHARGE_W, d->status.override_charge_power_W);
    put_u32_field(ESPNOW_KEY_OVERRIDE_DISCHARGE_W, d->status.override_discharge_power_W);
    put_i16_field(ESPNOW_KEY_TEMPERATURE_MAX_DC, d->status.temperature_max_dC);
    put_i16_field(ESPNOW_KEY_TEMPERATURE_MIN_DC, d->status.temperature_min_dC);

    const uint8_t cells = d->info.number_of_cells;
    if (cells != 0u && d->status.cell_voltages_mV[cells - 1] != 0u) {
      put_u16_field(ESPNOW_KEY_CELL_MAX_MV, d->status.cell_max_voltage_mV);
      put_u16_field(ESPNOW_KEY_CELL_MIN_MV, d->status.cell_min_voltage_mV);
    }

    if (bat != nullptr && bat->supports_charged_energy()) {
      put_i32_field(ESPNOW_KEY_TOTAL_CHARGED_WH, d->status.total_charged_battery_Wh);
      put_i32_field(ESPNOW_KEY_TOTAL_DISCHARGED_WH, d->status.total_discharged_battery_Wh);
    }
    if (d->status.insulation_resistance_available) {
      put_u16_field(ESPNOW_KEY_INSULATION_KOHM, d->status.insulation_resistance_kOhm);
    }

    uint16_t active_cells = 0;
    for (uint8_t i = 0; i < cells; i++) {
      if (d->status.cell_balancing_status[i]) {
        active_cells++;
      }
    }
    put_u16_field(ESPNOW_KEY_BALANCING_ACTIVE_CELLS, active_cells);
    put_enum_field(ESPNOW_KEY_BALANCING_STATUS, static_cast<uint8_t>(d->status.balancing_status));

    const ChargingState charging_state = get_charging_state(d->status.current_dA);
    put_enum_field(ESPNOW_KEY_CHARGING_STATE, static_cast<uint8_t>(charging_state));
    put_enum_field(ESPNOW_KEY_LIMITING_FACTOR,
                   static_cast<uint8_t>(get_limiting_factor(
                       charging_state, d->settings.inverter_limits_charge, d->settings.inverter_limits_discharge,
                       d->settings.user_settings_limit_charge, d->settings.user_settings_limit_discharge)));

    if (index == 0 && (user_selected_battery_type == BatteryType::TeslaModel3Y ||
                       user_selected_battery_type == BatteryType::TeslaModelSX)) {
      put_i16_field(ESPNOW_KEY_DCDC_CURRENT_DA,
                    static_cast<int16_t>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent));
      // Raw unit is 0.0390625 V; 1/0.0390625 == 25.6, so *1000/25.6 == *125/3.2. Scaled
      // with integer maths to millivolts to keep floats out of the send path.
      put_u16_field(
          ESPNOW_KEY_DCDC_VOLTAGE_MV,
          static_cast<uint16_t>((static_cast<uint32_t>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 625u) / 16u));
    }
    if (user_selected_battery_type == BatteryType::BydAtto3) {
      const DATALAYER_INFO_BYDATTO3& byd = (index == 1) ? datalayer_extended.bydAtto3_2 : datalayer_extended.bydAtto3;
      put_bool_field(ESPNOW_KEY_AUTOCAL_TAPER, byd.autocal_crit_taper);
      put_u32_field(ESPNOW_KEY_AUTOCAL_DWELL_S, byd.autocal_dwell_accumulated_ms / 1000u);
      put_bool_field(ESPNOW_KEY_AUTOCAL_COOLDOWN_READY, byd.autocal_crit_cooldown_ready);
      put_float_field(ESPNOW_KEY_AUTOCAL_SOC_DRIFT, byd.autocal_drift_percent);
    }
  }

  end_frame();
}

// Emits one chunk of the cell arrays. Cell voltages go out as raw millivolts.
static void send_cells_frame(uint8_t index, uint16_t first_cell, uint16_t count, bool more) {
  const DATALAYER_BATTERY_TYPE* d = battery_data(index);

  begin_frame(ESPNOW_FRAME_CELLS, static_cast<uint8_t>(index + 1), more ? ESPNOW_FLAG_MORE_CHUNKS : 0);

  put_u16_field(ESPNOW_KEY_CELL_COUNT, d->info.number_of_cells);
  put_u16_field(ESPNOW_KEY_CELL_INDEX, first_cell);

  if (put_header(ESPNOW_KEY_CELL_VOLTAGES_MV, ESPNOW_TYPE_ARR16, static_cast<size_t>(count) * 2u)) {
    for (uint16_t i = 0; i < count; i++) {
      const uint16_t mV = d->status.cell_voltages_mV[first_cell + i];
      put_u8(static_cast<uint8_t>(mV & 0xFF));
      put_u8(static_cast<uint8_t>(mV >> 8));
    }
  }

  const size_t bitset_bytes = (static_cast<size_t>(count) + 7u) / 8u;
  if (put_header(ESPNOW_KEY_CELL_BALANCING, ESPNOW_TYPE_BITS, bitset_bytes)) {
    for (size_t b = 0; b < bitset_bytes; b++) {
      uint8_t byte = 0;
      for (uint8_t bit = 0; bit < 8; bit++) {
        const size_t cell = b * 8u + bit;
        if (cell < count && d->status.cell_balancing_status[first_cell + cell]) {
          byte = static_cast<uint8_t>(byte | (1u << bit));
        }
      }
      put_u8(byte);
    }
  }

  end_frame();
}

// Collects the ESPNOW_EVENT_REPLAY most recent occurrences into event_batch, most recent
// first. Insertion sort over a fixed array: EVENT_NOF_EVENTS is a few hundred and the list
// is ten long, so this is cheaper than it looks and allocates nothing.
static void build_event_batch() {
  uint64_t stamps[ESPNOW_EVENT_REPLAY];
  event_batch_count = 0;

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_ENUM_TYPE handle = static_cast<EVENTS_ENUM_TYPE>(i);
    const EVENTS_STRUCT_TYPE* ev = get_event_pointer(handle);
    if (ev == nullptr || ev->occurences == 0) {
      continue;
    }
    // Full and older than everything held: nothing to do.
    if (event_batch_count == ESPNOW_EVENT_REPLAY && ev->timestamp <= stamps[ESPNOW_EVENT_REPLAY - 1]) {
      continue;
    }
    uint8_t pos = event_batch_count < ESPNOW_EVENT_REPLAY ? event_batch_count : ESPNOW_EVENT_REPLAY - 1;
    while (pos > 0 && stamps[pos - 1] < ev->timestamp) {
      stamps[pos] = stamps[pos - 1];
      event_batch[pos] = event_batch[pos - 1];
      pos--;
    }
    stamps[pos] = ev->timestamp;
    event_batch[pos] = handle;
    if (event_batch_count < ESPNOW_EVENT_REPLAY) {
      event_batch_count++;
    }
  }

  event_newest_sent = event_batch_count > 0 ? stamps[0] : 0;
}

// True when the event table holds an occurrence newer than anything in the last batch.
static bool event_backlog_changed() {
  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* ev = get_event_pointer(static_cast<EVENTS_ENUM_TYPE>(i));
    if (ev != nullptr && ev->occurences > 0 && ev->timestamp > event_newest_sent) {
      return true;
    }
  }
  return false;
}

static void send_event_frame(EVENTS_ENUM_TYPE handle, const EVENTS_STRUCT_TYPE* ev, uint8_t index, uint8_t total) {
  begin_frame(ESPNOW_FRAME_EVENT, 0, (index + 1 < total) ? ESPNOW_FLAG_MORE_CHUNKS : 0);

  put_u8_field(ESPNOW_KEY_EVENT_INDEX, index);
  put_u8_field(ESPNOW_KEY_EVENT_TOTAL, total);
  put_u16_field(ESPNOW_KEY_EVENT_ID, static_cast<uint16_t>(handle));
  put_str_field(ESPNOW_KEY_EVENT_NAME, get_event_enum_string(handle));
  put_enum_field(ESPNOW_KEY_EVENT_SEVERITY, static_cast<uint8_t>(ev->level));
  put_enum_field(ESPNOW_KEY_EVENT_STATE, static_cast<uint8_t>(ev->state));
  put_u8_field(ESPNOW_KEY_EVENT_COUNT, ev->occurences);
  put_i16_field(ESPNOW_KEY_EVENT_DATA_I16, ev->data);
  put_int(ESPNOW_KEY_EVENT_MILLIS, ESPNOW_TYPE_UINT, ev->timestamp, 8);
  put_str_field(ESPNOW_KEY_EVENT_MESSAGE, get_event_message_string(handle).c_str());

  end_frame();
}

// ---------------------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------------------

void init_espnow() {
  // Wi-Fi has to be up before ESP-NOW is initialized.
  if ((WiFi.getMode() != WIFI_AP_STA) && (WiFi.getMode() != WIFI_STA)) {
    logging.println("Wifi should be initialized before using ESPNow");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    logging.println("Error initializing ESPNow");
    return;
  }

  // No send callback is registered. Nothing useful could be done in it (broadcast peers
  // report failure on some SoCs and it runs from the high priority Wi-Fi task), and
  // esp_now_send_cb_t changed its first argument from const uint8_t* to
  // const wifi_tx_info_t* in ESP-IDF 5.5 - not registering one keeps this source building
  // against both.

  const uint8_t peers = register_configured_peers();
  if (peers == 0) {
    esp_now_peer_info_t peer = {};
    peer.channel = 0;
    peer.encrypt = false;
    peer.ifidx = WIFI_IF_STA;
    memcpy(peer.peer_addr, broadcast_mac, 6);
    if (esp_now_add_peer(&peer) != ESP_OK) {
      logging.println("Failed to add ESPNow broadcast peer");
      return;
    }
    memcpy(peer_macs[0], broadcast_mac, 6);
    peer_count = 1;
    logging.println("ESPNow: broadcasting to all receivers");
  } else {
    peer_count = peers;
    logging.printf("ESPNow: sending to %d configured receiver(s)\n", peers);
  }

  emulator_id = static_cast<uint16_t>(ESP.getEfuseMac() & 0xFFFF);

  num_batteries = 1;
  if (battery2) {
    num_batteries++;
  }
  if (battery3) {
    num_batteries++;
  }

  logging.printf("ESPNow: protocol v%d, max %u byte frames, %u cells per frame\n", ESPNOW_PROTOCOL_VERSION,
                 static_cast<unsigned>(max_payload), static_cast<unsigned>(cells_per_chunk));

  espnow_initialized = true;
}

void update_espnow() {
  if (!espnow_initialized) {
    return;
  }

  const uint32_t now = millis();

  if (phase == PHASE_IDLE) {
    if (now - cycle_start_ms < ESPNOW_CYCLE_MS) {
      return;
    }
    cycle_start_ms = now;
    cells_due = (now - cells_last_ms) >= ESPNOW_CELL_CYCLE_MS;
    // Re-send the backlog on its own timer, or straight away if something new happened.
    events_due = ((now - events_last_ms) >= ESPNOW_EVENT_CYCLE_MS) || event_backlog_changed();
    if (events_due) {
      events_last_ms = now;
      build_event_batch();
    }
    cursor_battery = 0;
    cursor_cell = 0;
    cursor_event = 0;
    phase = PHASE_SYSTEM;
  }

  // One frame per tick, so the Wi-Fi stack always finishes a send before the next starts.
  if (now - last_frame_ms < ESPNOW_FRAME_SPACING_MS) {
    return;
  }
  last_frame_ms = now;

  switch (phase) {
    case PHASE_SYSTEM:
      send_system_frame();
      phase = PHASE_BATTERY;
      break;

    case PHASE_BATTERY:
      if (cursor_battery < num_batteries) {
        send_battery_frame(cursor_battery++);
        break;
      }
      cursor_battery = 0;
      if (cells_due) {
        cells_last_ms = now;
        phase = PHASE_CELLS;
      } else {
        phase = PHASE_EVENTS;
      }
      break;

    case PHASE_CELLS: {
      if (cursor_battery >= num_batteries) {
        cursor_battery = 0;
        phase = PHASE_EVENTS;
        break;
      }
      const DATALAYER_BATTERY_TYPE* d = battery_data(cursor_battery);
      const uint16_t cells = d->info.number_of_cells;
      if (cells == 0u || cells > MAX_AMOUNT_CELLS) {
        cursor_battery++;
        cursor_cell = 0;
        break;
      }
      uint16_t count = static_cast<uint16_t>(cells - cursor_cell);
      if (count > cells_per_chunk) {
        count = cells_per_chunk;
      }
      const bool more = (cursor_cell + count) < cells;
      send_cells_frame(cursor_battery, cursor_cell, count, more);
      cursor_cell = static_cast<uint16_t>(cursor_cell + count);
      if (!more) {
        cursor_battery++;
        cursor_cell = 0;
      }
      break;
    }

    case PHASE_EVENTS: {
      // One frame of the prepared batch per tick, most recent first. The batch is a
      // snapshot, so an event arriving mid-transmission is picked up by the next cycle
      // rather than shifting the indices of the one in flight.
      if (events_due && cursor_event < event_batch_count) {
        const EVENTS_ENUM_TYPE handle = event_batch[cursor_event];
        const EVENTS_STRUCT_TYPE* ev = get_event_pointer(handle);
        if (ev != nullptr) {
          send_event_frame(handle, ev, cursor_event, event_batch_count);
        }
        cursor_event++;
        return;
      }
      phase = PHASE_IDLE;
      break;
    }

    case PHASE_IDLE:
    default:
      break;
  }
}
