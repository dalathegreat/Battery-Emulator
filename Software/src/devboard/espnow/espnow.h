#ifndef _ESPNOW_H_
#define _ESPNOW_H_

// This is sending the Battery Emulator data over ESP-NOW to nearby devices.
//
// ESP-NOW v2 is REQUIRED. v2 lifts the per-packet limit from 250 to 1470 bytes,
// which is what lets us send full-resolution (1 mV) cell voltages as well as the
// event / system-status / extended-vendor frames. v2 is selected automatically by
// the stack when built against ESP-IDF >= 5.4 (this project pins IDF 5.5 via the
// pioarduino platform), so no runtime "enable v2" call is needed.
//
// Wire format is shared verbatim with the receiver: whatever structs are compiled
// here must be compiled identically on the monitoring device.

#include <esp_now.h>  // ESP_NOW_MAX_DATA_LEN_V2
#include "../../system_settings.h"
#include "../utils/events.h"  // EVENT_NOF_EVENTS, EVENTS_LEVEL_TYPE, EMULATOR_STATUS
#include "../utils/types.h"

#if !defined(ESP_NOW_MAX_DATA_LEN_V2)
#error "ESP-NOW v2 (ESP-IDF >= 5.4) required. Update the pioarduino platform pin."
#endif

void init_espnow();
void update_espnow();

struct BATTERY_INFO_TYPE {
  /** uint32_t */
  /** Total energy capacity in Watt-hours
   * Automatically updates depending on battery integration OR from settings page
  */
  uint32_t total_capacity_Wh = 30000;
  uint32_t reported_total_capacity_Wh = 30000;

  /** uint16_t */
  /** The maximum intended packvoltage, in deciVolt. 4900 = 490.0 V */
  uint16_t max_design_voltage_dV = 5000;
  /** The minimum intended packvoltage, in deciVolt. 3300 = 330.0 V */
  uint16_t min_design_voltage_dV = 2500;
  /** The maximum cellvoltage before shutting down, in milliVolt. 4300 = 4.300 V */
  uint16_t max_cell_voltage_mV = 4300;
  /** The minimum cellvoltage before shutting down, in milliVolt. 2700 = 2.700 V */
  uint16_t min_cell_voltage_mV = 2700;
  /** The maxumum allowed deviation between cells, in milliVolt. 500 = 0.500 V */
  uint16_t max_cell_voltage_deviation_mV = 500;

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

  /** Other */
  /** Chemistry of the pack. Autodetect, or force specific chemistry */
  battery_chemistry_enum chemistry = battery_chemistry_enum::NCA;
};  // 24 bytes

struct BATTERY_STATUS_TYPE {
  /** uint32_t */
  /** Remaining energy capacity in Watt-hours */
  uint32_t remaining_capacity_Wh = 0;
  /** The remaining capacity reported to the inverter based on min percentage setting, in Watt-hours
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint32_t reported_remaining_capacity_Wh;
  /** Maximum allowed battery discharge power in Watts. Set by battery */
  uint32_t max_discharge_power_W = 0;
  /** Maximum allowed battery charge power in Watts. Set by battery */
  uint32_t max_charge_power_W = 0;
  /* Some early integrations do not support reading allowed charge power from battery
  On these integrations we need to have the user specify what limits the battery can take */
  /** Overriden allowed battery discharge power in Watts. Set by user */
  uint32_t override_discharge_power_W = 0;
  /** Overriden allowed battery charge power in Watts. Set by user */
  uint32_t override_charge_power_W = 0;

  /** int32_t */
  /** Instantaneous battery power in Watts. Calculated based on voltage_dV and current_dA */
  /* Positive value = Battery Charging */
  /* Negative value = Battery Discharging */
  int32_t active_power_W = 0;
  int32_t total_charged_battery_Wh = 0;
  int32_t total_discharged_battery_Wh = 0;

  /** uint16_t */
  /** Maximum allowed battery discharge current in dA. Calculated based on allowed W and Voltage */
  uint16_t max_discharge_current_dA = 0;
  /** Maximum allowed battery charge current in dA. Calculated based on allowed W and Voltage  */
  uint16_t max_charge_current_dA = 0;
  /** State of health in integer-percent x 100. 9900 = 99.00% */
  uint16_t soh_pptt = 9900;
  /** Instantaneous battery voltage in deciVolts. 3700 = 370.0 V */
  uint16_t voltage_dV = 3700;
  /** Maximum cell voltage currently measured in the pack, in mV */
  uint16_t cell_max_voltage_mV = 3700;
  /** Minimum cell voltage currently measured in the pack, in mV */
  uint16_t cell_min_voltage_mV = 3700;
  /** The "real" SOC reported from the battery, in integer-percent x 100. 9550 = 95.50% */
  uint16_t real_soc;
  /** The SOC reported to the inverter, in integer-percent x 100. 9550 = 95.50%.
   * This value will either be scaled or not scaled depending on the value of
   * battery.settings.soc_scaling_active
   */
  uint16_t reported_soc;
  /** A counter that increases incase a CAN CRC read error occurs */
  uint16_t CAN_error_counter;

  /** int16_t */
  /** Maximum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_max_dC;
  /** Minimum temperature currently measured in the pack, in d°C. 150 = 15.0 °C */
  int16_t temperature_min_dC;
  /** Instantaneous battery current in deciAmpere. 95 = 9.5 A */
  int16_t current_dA;
  /** Instantaneous battery current in deciAmpere. Sum of all batteries in the system 95 = 9.5 A */
  int16_t reported_current_dA;

  /** uint8_t */
  /** A counter set each time a new message comes from battery.
   * This value then gets decremented every second. Incase we reach 0
   * we report the battery as missing entirely on the CAN bus.
   */
  uint8_t CAN_battery_still_alive = CAN_STILL_ALIVE;
  /** The current battery status, which for now has the name real_bms_status */
  real_bms_status_enum real_bms_status = BMS_DISCONNECTED;
  /** LED mode, customizable by user */
  led_mode_enum led_mode = CLASSIC;
  /** Balancing status */
  balancing_status_enum balancing_status = BALANCING_STATUS_UNKNOWN;
};  // 80 bytes

struct BATTERY_BALANCING_STATUS_TYPE {
  /** All balancing resistors status inside the pack, either on(1) or off(0).
   * Use with battery.info.number_of_cells to get valid data.
   * Not available for all battery manufacturers.
   */
  bool cell_balancing_status[MAX_AMOUNT_CELLS];

  /** uint8_t */
  /** Total number of cells in the pack */
  uint8_t number_of_cells;

};  // 193 bytes

// Cell voltages at full 1 mV resolution (was uint8_t at 20 mV steps under v1).
//
// Layout is COUNT-FIRST so the on-air length can be trimmed to only the cells
// that are actually present:  [number_of_cells][v0][v1]...[v_{n-1}]
//   -> wire length = 1 + n * 2  (max 1 + 192 * 2 = 385 bytes, well under the
//      1470-byte v2 limit, so a single packet holds one full pack even at 192s).
//
// NOTE for the receiver: both the element type (uint8_t -> uint16_t) and the
// field order (array-then-count -> count-then-array) changed versus the v1 code.
struct BATTERY_CELL_STATUS_TYPE {
  uint8_t number_of_cells;
  uint16_t cell_voltages_mV[MAX_AMOUNT_CELLS];
} __packed;

// One active event. Mirrors the fields MQTT publishes, minus the human-readable
// strings: the receiver derives event name / severity text / message from
// event_handle and level using its own copy of the events tables, so we don't
// pay the flash or the airtime to serialise strings here.
struct ESPNOW_EVENT_RECORD {
  uint8_t event_handle;   // EVENTS_ENUM_TYPE index (EVENT_NOF_EVENTS < 256)
  uint8_t level;          // EVENTS_LEVEL_TYPE (severity)
  uint8_t data;           // event-specific payload, e.g. the offending cell number
  uint8_t occurrences;    // occurences since boot (source field is uint8_t)
  uint32_t timestamp_ms;  // low 32 bits of the 64-bit event timestamp
} __packed;               // 8 bytes

// System-level status frame (BAT_SYSTEM). Answers "is the emulator healthy, and
// how hot is the chip" without the receiver needing any battery context.
struct ESPNOW_SYSTEM_STATUS {
  uint8_t emulator_status;     // EMULATOR_STATUS: OK / WARNING / ERROR / UPDATING
  uint8_t system_status;       // system_status_enum (finer-grained state machine)
  int16_t cpu_temperature_dC;  // ESP32 core temperature in 0.1 °C. 235 = 23.5 °C
  uint32_t uptime_s;           // seconds since boot
  uint32_t free_heap;          // free heap in bytes
} __packed;

enum espnow_vendor_enum : uint8_t { ESPNOW_VENDOR_NONE = 0, ESPNOW_VENDOR_TESLA = 1, ESPNOW_VENDOR_BYD = 2 };

// Extended per-integration metrics frame (BAT_EXTENDED). Only the union member
// matching `vendor` is valid. Values are RAW (same as the datalayer holds); the
// receiver applies the same scaling MQTT does (see comments below).
struct ESPNOW_EXTENDED {
  uint8_t vendor;  // espnow_vendor_enum
  union {
    struct {
      uint16_t dcdc_lv_bus_mV_raw;   // * 0.0390625 = Volt
      uint16_t dcdc_lv_current_raw;  // * 0.1       = Ampere
    } tesla;
    struct {
      uint32_t autocal_dwell_ms;      // accumulated dwell, milliseconds
      float autocal_drift_percent;    // SOC drift, percent
      uint8_t autocal_in_taper;       // bool: currently in critical taper
      uint8_t autocal_cooldown_ready;  // bool: cooldown complete
    } byd;
  } data;
} __packed;

enum espnow_message_enum {
  BAT_INFO = 1,
  BAT_STATUS = 2,
  BAT_BALANCE = 3,
  BAT_CELL_STATUS = 4,
  BAT_EVENTS = 5,     // [count][ESPNOW_EVENT_RECORD * count]
  BAT_SYSTEM = 6,     // ESPNOW_SYSTEM_STATUS
  BAT_EXTENDED = 7    // ESPNOW_EXTENDED
};

// Largest payload we ever put on the wire: the ESP-NOW v2 limit minus the 4-byte
// message header (emulator_id + battery_id + esp_message_type). Sizing the buffer
// once, here, also removes the latent out-of-bounds write the previous
// flexible-array `esp_message[]` had (it had zero backing storage yet was memcpy'd
// into with up to 193 bytes).
static constexpr size_t ESPNOW_MAX_PAYLOAD = ESP_NOW_MAX_DATA_LEN_V2 - 4;

struct ESPNOW_BATTERY_MESSAGE {
  uint16_t emulator_id;
  uint8_t battery_id;
  uint8_t esp_message_type;
  uint8_t esp_message[ESPNOW_MAX_PAYLOAD];
} __packed;

// Every frame must fit the shared buffer.
static_assert(1 + MAX_AMOUNT_CELLS * 2 <= ESPNOW_MAX_PAYLOAD, "cell frame exceeds ESP-NOW v2 payload");
static_assert(1 + EVENT_NOF_EVENTS * sizeof(ESPNOW_EVENT_RECORD) <= ESPNOW_MAX_PAYLOAD,
              "event frame exceeds ESP-NOW v2 payload");

#endif  // _ESPNOW_H_
