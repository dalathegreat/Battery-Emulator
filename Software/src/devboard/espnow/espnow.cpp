// This is sending the Battery Emulator data over ESP-NOW to nearby devices.
// Requires ESP-NOW v2 (ESP-IDF >= 5.4); see espnow.h for the wire format.

#include "espnow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <cmath>    // lroundf
#include <cstddef>  // offsetof
#include <cstring>  // memcpy / memset
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"  // user_selected_battery_type, BatteryType
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"  // tesla / bydAtto3 metrics
#include "../hal/hal.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#include "Arduino.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

// Reusable send buffers. b_message carries every frame; b_cells / b_bal_status are
// staging for the two variable-length battery frames. (The old b_info / b_status
// globals are gone: those senders memcpy straight from the datalayer.)
struct BATTERY_CELL_STATUS_TYPE b_cells;
struct BATTERY_BALANCING_STATUS_TYPE b_bal_status;
struct ESPNOW_BATTERY_MESSAGE b_message;

// TODO: Add support for configurable MAC address instead of broadcast
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info_t peerInfo;

bool espnow_initialized = false;
unsigned long b_lastUpdateMillis = 0;      // fast cadence: per-battery telemetry
unsigned long b_lastSlowUpdateMillis = 0;  // slow cadence: events / system / extended
int b_num_batteries = 1;

uint16_t emulator_id;

// Per-battery telemetry keeps the original 1 s cadence. Events, system status and
// extended metrics change slowly, so they go out every 5 s. Sending them less
// often also keeps ESP-NOW airtime (and therefore the chip temperature) down.
static const uint32_t ESPNOW_FAST_INTERVAL_MS = 1000;
static const uint32_t ESPNOW_SLOW_INTERVAL_MS = 5000;

// ESP-NOW callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    logging.println("Last ESPNow Packet Send Status: Delivery Fail");
  }
}

// --- single transmit core ---------------------------------------------------
// Fills the 4-byte header and sends whatever is already sitting in
// b_message.esp_message. One implementation, one error string: this replaces the
// four near-identical send_* bodies (and their four separate error literals) that
// the v1 code had, which is where the flash for the new frames comes from.
static void espnow_transmit(uint8_t type, uint8_t battery_id, size_t payload_len) {
  b_message.emulator_id = emulator_id;
  b_message.battery_id = battery_id;
  b_message.esp_message_type = type;

  const size_t total = offsetof(ESPNOW_BATTERY_MESSAGE, esp_message) + payload_len;
  if (esp_now_send(broadcastAddress, (uint8_t*)&b_message, total) != ESP_OK) {
    logging.println("Error sending ESPNow data");
  }
}

// Copy a source blob into the payload buffer, then transmit.
static void espnow_send(uint8_t type, uint8_t battery_id, const void* src, size_t len) {
  memcpy(b_message.esp_message, src, len);
  espnow_transmit(type, battery_id, len);
}

// --- per-battery frames -----------------------------------------------------
static void send_battery_info(DATALAYER_BATTERY_INFO_TYPE& info, uint8_t b_index) {
  espnow_send(BAT_INFO, b_index + 1, &info, sizeof(BATTERY_INFO_TYPE));  // 24 bytes
}

static void send_battery_status(DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t b_index) {
  espnow_send(BAT_STATUS, b_index + 1, &status, sizeof(BATTERY_STATUS_TYPE));  // 80 bytes
}

static void send_battery_cell_status(DATALAYER_BATTERY_INFO_TYPE& info, DATALAYER_BATTERY_STATUS_TYPE& status,
                                     uint8_t b_index) {
  uint8_t n = info.number_of_cells;
  if (n > MAX_AMOUNT_CELLS) {
    n = MAX_AMOUNT_CELLS;
  }
  b_cells.number_of_cells = n;
  for (uint8_t i = 0; i < n; i++) {
    b_cells.cell_voltages_mV[i] = status.cell_voltages_mV[i];  // full 1 mV, no /20 quantization
  }
  // Trim to the cells present: 1 count byte + n * uint16. Each battery sends its
  // own packet, so three 192s packs are three ~385-byte packets, not one big one.
  espnow_send(BAT_CELL_STATUS, b_index + 1, &b_cells, sizeof(uint8_t) + (size_t)n * sizeof(uint16_t));
}

static void send_battery_balancing(DATALAYER_BATTERY_INFO_TYPE& info, DATALAYER_BATTERY_STATUS_TYPE& status,
                                   uint8_t b_index) {
  uint8_t n = info.number_of_cells;
  if (n > MAX_AMOUNT_CELLS) {
    n = MAX_AMOUNT_CELLS;
  }
  b_bal_status.number_of_cells = n;
  for (uint8_t i = 0; i < n; i++) {
    b_bal_status.cell_balancing_status[i] = status.cell_balancing_status[i];
  }
  espnow_send(BAT_BALANCE, b_index + 1, &b_bal_status, sizeof(BATTERY_BALANCING_STATUS_TYPE));  // 193 bytes
}

// --- system-wide frames (battery_id 0) --------------------------------------
// Snapshot of every currently-active event. Built directly into the shared
// payload buffer (no ~1 KB local array on the 4 KB connectivity-task stack).
static void send_events(void) {
  uint8_t* buf = b_message.esp_message;
  size_t len = sizeof(uint8_t);  // reserve buf[0] for the record count
  uint8_t count = 0;

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    const EVENTS_STRUCT_TYPE* ev = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (ev == nullptr || ev->occurences == 0) {
      continue;  // match MQTT: only report events that have actually fired
    }
    if (len + sizeof(ESPNOW_EVENT_RECORD) > ESPNOW_MAX_PAYLOAD) {
      break;  // safety: never overrun the buffer (cannot happen with 126 events)
    }
    ESPNOW_EVENT_RECORD rec;
    rec.event_handle = (uint8_t)i;
    rec.level = (uint8_t)ev->level;
    rec.data = ev->data;
    rec.occurrences = ev->occurences;
    rec.timestamp_ms = (uint32_t)ev->timestamp;  // low 32 bits is enough for "age" on the monitor
    memcpy(buf + len, &rec, sizeof(rec));
    len += sizeof(rec);
    count++;
  }

  buf[0] = count;
  espnow_transmit(BAT_EVENTS, 0, len);  // send even when count == 0 (clears stale events on the monitor)
}

static void send_system_status(void) {
  ESPNOW_SYSTEM_STATUS s;
  s.emulator_status = (uint8_t)get_emulator_status();
  s.system_status = (uint8_t)datalayer.system.status.system_status;
  s.cpu_temperature_dC = (int16_t)lroundf(datalayer.system.info.CPU_temperature * 10.0f);
  s.uptime_s = (uint32_t)(millis() / 1000UL);
  s.free_heap = datalayer.system.info.CPU_free_heap;
  espnow_send(BAT_SYSTEM, 0, &s, sizeof(s));
}

static void send_extended(void) {
  ESPNOW_EXTENDED e;
  memset(&e, 0, sizeof(e));

  switch (user_selected_battery_type) {
    case BatteryType::TeslaModel3Y:
    case BatteryType::TeslaModelSX:
      e.vendor = ESPNOW_VENDOR_TESLA;
      e.data.tesla.dcdc_lv_bus_mV_raw = datalayer_extended.tesla.battery_dcdcLvBusVolt;
      e.data.tesla.dcdc_lv_current_raw = datalayer_extended.tesla.battery_dcdcLvOutputCurrent;
      break;
    case BatteryType::BydAtto3:
      e.vendor = ESPNOW_VENDOR_BYD;
      e.data.byd.autocal_dwell_ms = datalayer_extended.bydAtto3.autocal_dwell_accumulated_ms;
      e.data.byd.autocal_drift_percent = datalayer_extended.bydAtto3.autocal_drift_percent;
      e.data.byd.autocal_in_taper = datalayer_extended.bydAtto3.autocal_crit_taper ? 1 : 0;
      e.data.byd.autocal_cooldown_ready = datalayer_extended.bydAtto3.autocal_crit_cooldown_ready ? 1 : 0;
      break;
    default:
      return;  // no extended metrics for this battery type
  }
  espnow_send(BAT_EXTENDED, 0, &e, sizeof(e));
}

void init_espnow() {
  // Check wifi status
  // Wifi Should be already set before initializing ESPNow
  if ((WiFi.getMode() != WIFI_AP_STA) && (WiFi.getMode() != WIFI_STA)) {
    logging.println("Wifi should be initialized before using ESPNow");
    return;
  }

  // Init ESPNow
  if (esp_now_init() != ESP_OK) {
    logging.println("Error initializing ESPNow");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent)));

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    logging.println("Failed to add ESPNow peer");
    return;
  }

  // Get last 2 bytes from Mac as Emulator ID
  emulator_id = ESP.getEfuseMac() & 0xFFFF;

  // Count configured batteries
  if (battery2)
    b_num_batteries++;
  if (battery3)
    b_num_batteries++;

  espnow_initialized = true;
}

void update_espnow() {
  if (!espnow_initialized) {
    return;
  }

  auto currentMillis = millis();

  // Fast cadence: per-battery telemetry (unchanged behaviour vs v1).
  if (currentMillis - b_lastUpdateMillis >= ESPNOW_FAST_INTERVAL_MS) {
    b_lastUpdateMillis = currentMillis;

    for (int battery_index = 0; battery_index < b_num_batteries; battery_index++) {
      if (battery_index == 0 && battery != nullptr) {
        send_battery_info(datalayer.battery.info, battery_index);
        send_battery_status(datalayer.battery.status, battery_index);
        send_battery_cell_status(datalayer.battery.info, datalayer.battery.status, battery_index);
        send_battery_balancing(datalayer.battery.info, datalayer.battery.status, battery_index);
      } else if (battery_index == 1) {
        send_battery_info(datalayer.battery2.info, battery_index);
        send_battery_status(datalayer.battery2.status, battery_index);
        send_battery_cell_status(datalayer.battery2.info, datalayer.battery2.status, battery_index);
        send_battery_balancing(datalayer.battery2.info, datalayer.battery2.status, battery_index);
      } else {
        send_battery_info(datalayer.battery3.info, battery_index);
        send_battery_status(datalayer.battery3.status, battery_index);
        send_battery_cell_status(datalayer.battery3.info, datalayer.battery3.status, battery_index);
        send_battery_balancing(datalayer.battery3.info, datalayer.battery3.status, battery_index);
      }
    }
  }

  // Slow cadence: system-wide frames.
  if (currentMillis - b_lastSlowUpdateMillis >= ESPNOW_SLOW_INTERVAL_MS) {
    b_lastSlowUpdateMillis = currentMillis;
    send_events();
    send_system_status();
    send_extended();
  }
}
