#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <float.h>
#include <freertos/FreeRTOS.h>
#include <limits.h>
#include <math.h>
#include <src/communication/nvm/comm_nvm.h>
#include <stdlib.h>
#include <string>
#include <type_traits>
#include "../../battery/BATTERIES.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../datalayer/battery_aggregate.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/espnow/espnow.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/network/hostname.h"
#include "../../devboard/network/network_status.h"
#include "../../devboard/safety/safety.h"
#include "../utils/events.h"
#include "../utils/timer.h"
#include "../webserver/webserver.h"
#include "mqtt_client.h"

std::string mqtt_user;
std::string mqtt_password;

bool mqtt_enabled = false;
bool ha_autodiscovery_enabled = false;
std::string ha_autodiscovery_topic = "homeassistant";
bool mqtt_transmit_all_cellvoltages = false;
bool mqtt_publish_heap_metrics = false;
uint16_t mqtt_timeout_ms = 2000;
uint16_t mqtt_publish_interval_ms = 5000;

const int mqtt_port_default = 0;
const char* mqtt_server_default = "";

int mqtt_port = mqtt_port_default;
std::string mqtt_server = mqtt_server_default;

#define MQTT_QOS 0  // MQTT Quality of Service (0, 1, or 2) //TODO: Should this be configurable?

// Cap on the esp-mqtt outbox, in bytes. QoS>0 traffic queues in the outbox while the broker
// or Wi-Fi is unresponsive; without a limit it grows on the heap until OOM. With the limit,
// enqueueing fails instead — a bounded, recoverable failure.
#define MQTT_OUTBOX_LIMIT_BYTES (16 * 1024)

esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t client;
char mqtt_msg[MQTT_MSG_BUFFER_SIZE];
MyTimer publish_global_timer(0);  // Will be configured with mqtt_publish_interval_ms on first use
MyTimer check_global_timer(800);  // check timmer - low-priority MQTT checks, where responsiveness is not critical.

// Cell voltage and balancing payloads are large (~1-2 KB per battery) but slow-moving, so
// they are published on a fixed 60 s cadence, independent of mqtt_publish_interval_ms.
// cell_data_due starts true so the first payload goes out on the first publish cycle, and
// is only cleared once a full voltage+balancing round was published successfully.
#define MQTT_CELL_DATA_INTERVAL_MS 60000
static MyTimer cell_data_timer(MQTT_CELL_DATA_INTERVAL_MS);
static bool cell_data_due = true;

bool client_started = false;
static String lwt_topic = "";

static String topic_name = "";
static String default_entity_id_prefix = "";
static String device_name = "";
static String device_id = "";

static bool publish_common_info(void);
static bool publish_cell_voltages(void);
static bool publish_events(void);

// A dropped broker connection makes every publish fail (QoS 0 returns -1 while the client
// is not connected), and publish_values() runs again every mqtt_publish_interval_ms, so
// logging each failure unconditionally repeats the same line for the whole outage. Pending
// events make it worse: they are deliberately retried until they go out, so publish_events()
// fails on every cycle from the moment EVENT_MQTT_DISCONNECT is raised until reconnect.
// Log the first failure of an outage only; publish_values() re-arms the latch after a
// complete successful cycle.
static bool publish_failure_logged = false;

static void log_publish_failure(const char* what) {
  if (!publish_failure_logged) {
    publish_failure_logged = true;
    logging.printf("%s MQTT msg could not be sent\n", what);
  }
}

/** Publish global values and call callbacks for specific modules */
static void publish_values(void) {

  // "/status online" is published retained on MQTT_EVENT_CONNECTED (standard LWT
  // pattern), not re-published every cycle.

  if (publish_events() == false) {
    return;
  }

  if (publish_common_info() == false) {
    return;
  }

  if (mqtt_transmit_all_cellvoltages) {
    if (cell_data_timer.elapsed()) {
      cell_data_due = true;
    }
    if (publish_cell_voltages() == false) {
      return;
    }
  }

  // Whole cycle went out: arm the failure log again so the next outage is reported.
  publish_failure_logged = false;
}

static bool ha_common_info_published = false;
static bool ha_cell_voltages_published = false;
static bool ha_events_published = false;
static bool ha_buttons_published = false;

// Set from the MQTT_EVENT_CONNECTED handler, acted on by mqtt_client_loop(). The handler
// runs on the esp-mqtt task, so publishing the button configs from it would use shared_doc
// and mqtt_msg concurrently with the publish cycle running on the MQTT task.
static volatile bool pending_buttons_discovery = false;

// JSON messages are built directly into the MQTT transmit buffer, avoiding a second
// document-sized allocation on the constrained devices.
class JsonObjectWriter {
 public:
  JsonObjectWriter(char* buffer, size_t capacity) : buffer_(buffer), capacity_(capacity) { clear(); }

  class Value {
   public:
    Value(JsonObjectWriter& writer, const char* key) : writer_(writer), key_(key) {}
    Value& operator=(const String& value) {
      writer_.addString(key_, value.c_str());
      return *this;
    }
    Value& operator=(const std::string& value) {
      writer_.addString(key_, value.c_str());
      return *this;
    }
    Value& operator=(const char* value) {
      writer_.addString(key_, value == nullptr ? "" : value);
      return *this;
    }
    Value& operator=(bool value) {
      writer_.addBoolean(key_, value);
      return *this;
    }
    Value& operator=(float value) {
      writer_.addFloat(key_, value);
      return *this;
    }
    Value& operator=(double value) {
      writer_.addDouble(key_, value);
      return *this;
    }
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, Value&>::type operator=(
        T value) {
      writer_.addInteger(key_, value);
      return *this;
    }

   private:
    JsonObjectWriter& writer_;
    const char* key_;
  };

  Value operator[](const char* key) { return Value(*this, key); }

  void clear() {
    length_ = 0;
    first_ = true;
    valid_ = capacity_ >= 3;
    if (valid_) {
      buffer_[0] = '{';
      buffer_[1] = '\0';
      length_ = 1;
    } else if (capacity_ != 0) {
      buffer_[0] = '\0';
    }
  }

  void addStringArray(const char* key, const char* value) {
    beginValue(key);
    append("[");
    appendEscapedString(value);
    append("]");
  }

  void addObjectStringArray(const char* key, const char* object_key, const char* value) {
    beginValue(key);
    append("[{");
    appendEscapedString(object_key);
    append(":");
    appendEscapedString(value);
    append("}]");
  }

  void addRaw(const char* key, const char* value) {
    beginValue(key);
    append(value);
  }

  bool finish() {
    if (!valid_) {
      return false;
    }
    append("}");
    return valid_;
  }

 private:
  void append(const char* text) {
    const size_t size = strlen(text);
    if (!valid_ || size > capacity_ - length_ - 1) {
      valid_ = false;
      return;
    }
    memcpy(buffer_ + length_, text, size);
    length_ += size;
    buffer_[length_] = '\0';
  }

  void appendChar(char value) {
    if (!valid_ || length_ + 1 >= capacity_) {
      valid_ = false;
      return;
    }
    buffer_[length_++] = value;
    buffer_[length_] = '\0';
  }

  void appendEscapedString(const char* value) {
    static const char hex[] = "0123456789abcdef";
    appendChar('"');
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p != '\0'; ++p) {
      switch (*p) {
        case '"':
          append("\\\"");
          break;
        case '\\':
          append("\\\\");
          break;
        case '\b':
          append("\\b");
          break;
        case '\f':
          append("\\f");
          break;
        case '\n':
          append("\\n");
          break;
        case '\r':
          append("\\r");
          break;
        case '\t':
          append("\\t");
          break;
        default:
          if (*p < 0x20) {
            append("\\u00");
            appendChar(hex[*p >> 4]);
            appendChar(hex[*p & 0x0f]);
          } else {
            appendChar(static_cast<char>(*p));
          }
          break;
      }
    }
    appendChar('"');
  }

  void beginValue(const char* key) {
    if (!valid_) {
      return;
    }
    if (!first_) {
      append(",");
    }
    first_ = false;
    appendEscapedString(key);
    append(":");
  }

  void addString(const char* key, const char* value) {
    beginValue(key);
    appendEscapedString(value);
  }

  void addBoolean(const char* key, bool value) {
    beginValue(key);
    append(value ? "true" : "false");
  }

  template <typename T>
  void addInteger(const char* key, T value) {
    char number[24];
    if (std::is_signed<T>::value) {
      snprintf(number, sizeof(number), "%lld", static_cast<long long>(value));
    } else {
      snprintf(number, sizeof(number), "%llu", static_cast<unsigned long long>(value));
    }
    beginValue(key);
    append(number);
  }

  void addFloat(const char* key, float value) {
    if (!isfinite(value)) {
      beginValue(key);
      append("null");
      return;
    }
    char number[24];
    snprintf(number, sizeof(number), "%.9g", static_cast<double>(value));
    beginValue(key);
    append(number);
  }

  void addDouble(const char* key, double value) {
    if (!isfinite(value)) {
      beginValue(key);
      append("null");
      return;
    }
    char number[32];
    snprintf(number, sizeof(number), "%.17g", value);
    beginValue(key);
    append(number);
  }

  char* buffer_;
  size_t capacity_;
  size_t length_ = 0;
  bool first_ = true;
  bool valid_ = false;
};

static JsonObjectWriter shared_doc(mqtt_msg, sizeof(mqtt_msg));

static bool serialize_mqtt_json(JsonObjectWriter& doc) {
  if (!doc.finish()) {
    mqtt_msg[0] = '\0';
    logging.println("MQTT JSON message exceeds buffer");
    return false;
  }
  return true;
}

// FNV-1a over the version string. A hash rather than the string itself keeps this to a
// single primitive NVS entry, which is all that is needed to tell "same firmware as when
// discovery was last published" from "updated since". Never returns 0, so a missing NVS
// key (which reads back as 0) can never be mistaken for a matching signature.
uint32_t mqtt_firmware_signature(void) {
  uint32_t hash = 2166136261u;
  for (const char* c = version_number; *c != '\0'; c++) {
    hash = (hash ^ (uint8_t)*c) * 16777619u;
  }
  return (hash == 0u) ? 1u : hash;
}

// True once every discovery config that applies to this configuration has gone out. Cell
// voltage configs only count when they are actually published (MQTTCELLV), and they are
// only marked done once the cell count is known for every present battery.
static bool autodiscovery_complete(void) {
  return ha_common_info_published && ha_events_published && ha_buttons_published &&
         (ha_cell_voltages_published || !mqtt_transmit_all_cellvoltages);
}

// Clears the one-shot setting and records the firmware the configs were published from.
// The configs are retained at the broker, no need for emulator republishing at each boot.
static void store_autodiscovery_done(void) {
  ha_autodiscovery_enabled = false;  // switches the publish paths to state-only for this session
  BatteryEmulatorSettingsStore settings;
  settings.saveBool("HADISC", false);
  settings.saveUInt("HADISCFW", mqtt_firmware_signature());
  LOG_SET_NEXT_SEVERITY(5);  // notice
  logging.println("Home Assistant autodiscovery published");
}

// RAII guard resets the shared writer on scope entry and exit.
struct DocClearGuard {
  JsonObjectWriter& doc;
  explicit DocClearGuard(JsonObjectWriter& d) : doc(d) { doc.clear(); }
  ~DocClearGuard() { doc.clear(); }
};

struct SensorConfig {
  // Base (battery #1 / un-suffixed) entity id. The per-battery variants ("_2", "_3") are
  // generated on the fly at discovery time instead of being stored.
  const char* entity_id;
  const char* name;
  const char* unit;
  const char* device_class;

  // A function that returns true for the battery instance if it supports this config.
  // Plain function pointer: unlike std::function it needs no heap allocation.
  bool (*condition)(Battery*);
};

static bool always(Battery* b) {
  return true;
}

// The SOC window is a property of the installation, not of a pack: with several batteries the
// packs carry what they would with scaling switched off, so a per-pack "scaled" entity would
// only duplicate its "real" twin. The scaled figures live on the aggregate topic instead.
static bool single_pack(Battery* b) {
  return datalayer.system.info.configured_batteries < 2;
}
static bool supports_charged(Battery* b) {
  return b->supports_charged_energy();
}

// For the installation-level entities, which have no single Battery to ask. A Leaf reports no
// lifetime energy counters, so on an all-Leaf install these would be two entities pinned at 0.
static bool any_pack_supports_charged(Battery* unused) {
  for (Battery* bat : {battery, battery2, battery3}) {
    if (bat != nullptr && bat->supports_charged_energy()) {
      return true;
    }
  }
  return false;
}
static bool supports_tesla_dcdc_metrics(Battery* b) {
  return b != nullptr && (user_selected_battery_type == BatteryType::TeslaModel3Y ||
                          user_selected_battery_type == BatteryType::TeslaModelSX);
}
static bool supports_byd_autocal_metrics(Battery* b) {
  return b != nullptr && user_selected_battery_type == BatteryType::BydAtto3;
}
static bool supports_byd_metrics(Battery* b) {
  return b != nullptr && user_selected_battery_type == BatteryType::BydAtto3;
}
static bool supports_insulation(Battery* b) {
  return b != nullptr && b->supports_insulation_resistance();
}
static bool supports_leaf_metrics(Battery* b) {
  return b != nullptr && user_selected_battery_type == BatteryType::NissanLeaf;
}
// Emulator-level condition: the heap diagnostics are opt-in from the MQTT settings page.
static bool heap_metrics_enabled(Battery* b) {
  return mqtt_publish_heap_metrics;
}

static const SensorConfig batterySensorConfigTemplate[] = {
    {"SOC", "SoC (scaled)", "%", "battery", single_pack},
    {"SOC_real", "SoC (real)", "%", "battery", always},
    // No device_class: "battery" would file this next to the state of charge in Home Assistant
    // and take its icon, which is misleading for a health figure. state_class and the unit are
    // set explicitly further down instead.
    {"state_of_health", "State of Health", "%", "", always},
    {"temperature_min", "Temperature Min", "°C", "temperature", always},
    {"temperature_max", "Temperature Max", "°C", "temperature", always},
    {"stat_batt_power", "Battery Power", "W", "power", always},
    {"battery_current", "Battery Current", "A", "current", always},
    {"cell_max_voltage", "Cell Max Voltage", "V", "voltage", always},
    {"cell_min_voltage", "Cell Min Voltage", "V", "voltage", always},
    {"cell_voltage_delta", "Cell Voltage Delta", "mV", "voltage", always},
    {"battery_voltage", "Battery Voltage", "V", "voltage", always},
    {"total_capacity", "Total Capacity", "Wh", "energy", always},
    {"remaining_capacity", "Remaining Capacity (scaled)", "Wh", "energy", single_pack},
    {"remaining_capacity_real", "Remaining Capacity (real)", "Wh", "energy", always},
    {"max_discharge_power", "Max Discharge Power", "W", "power", always},
    {"max_charge_power", "Max Charge Power", "W", "power", always},
    {"charged_energy", "Battery Charged Energy", "Wh", "energy", supports_charged},
    {"discharged_energy", "Battery Discharged Energy", "Wh", "energy", supports_charged},
    {"insulation_resistance", "Insulation Resistance", "kΩ", "", supports_insulation},
    {"balancing_active_cells", "Balancing Cells", "", "", always},
    {"balancing_status", "Balancing Status", "", "", always},
    {"charging_state", "Charging State", "", "", always},
    // What is limiting the inverter is one answer for the whole installation, not a pack's. With
    // several packs it lives on the aggregate topic instead of being repeated on every pack.
    {"limiting_factor", "Limiting Factor", "", "", single_pack},
    {"dc_dc_current", "DC-DC Current", "A", "current", supports_tesla_dcdc_metrics},
    {"dc_dc_voltage", "DC-DC Voltage", "V", "voltage", supports_tesla_dcdc_metrics},
    {"autocal_taper", "BYD Auto-cal: Taper Complete", "", "", supports_byd_autocal_metrics},
    {"autocal_dwell_s", "BYD Auto-cal: Dwell Time", "s", "duration", supports_byd_autocal_metrics},
    {"autocal_cooldown_ready", "BYD Auto-cal: Cooldown Ready", "", "", supports_byd_autocal_metrics},
    {"autocal_soc_drift", "BYD Auto-cal: SOC Drift", "%", "", supports_byd_autocal_metrics},
    {"min_cell_number", "Min Cell Number", "", "", supports_byd_metrics},
    {"max_cell_number", "Max Cell Number", "", "", supports_byd_metrics},
    {"leaf_hx", "Hx", "%", "", supports_leaf_metrics},
    {"leaf_soh_raw", "State of Health (raw)", "%", "", supports_leaf_metrics},
    {"leaf_vbat", "VBAT +12 level", "V", "voltage", supports_leaf_metrics},
    {"leaf_capacity_ah", "Actual capacity (Ah)", "Ah", "", supports_leaf_metrics},
    {"leaf_capacity", "Actual capacity", "kWh", "energy_storage", supports_leaf_metrics},
    {"charge_session", "BYD Charge: Session", "", "", supports_byd_autocal_metrics},
    {"charge_grant", "BYD Charge: Grant From Battery", "", "", supports_byd_autocal_metrics},
    {"charge_bms_mode", "BYD Charge: Battery Mode", "", "", supports_byd_autocal_metrics},
    {"charge_term_cell_max", "BYD Charge: Termination Cell Max", "mV", "voltage", supports_byd_autocal_metrics},
    {"charge_term_cell_min", "BYD Charge: Termination Cell Min", "mV", "voltage", supports_byd_autocal_metrics},
    {"charge_term_cell_delta", "BYD Charge: Termination Cell Spread", "mV", "voltage", supports_byd_autocal_metrics},
    {"charge_term_cell_max_num", "BYD Charge: Termination High Cell #", "", "", supports_byd_autocal_metrics},
    {"charge_term_cell_min_num", "BYD Charge: Termination Low Cell #", "", "", supports_byd_autocal_metrics}};

// The installation as the inverter sees it, published on its own topic when more than one
// battery is configured. Entity ids get "_multi" where batteries 1, 2 and 3 get "", "_2" and
// "_3"; the names carry no suffix at all, because the unqualified "SoC" sitting beside "SoC 1"
// and "SoC 2" is the installation. With a single pack this is never published: it would only
// repeat battery #1.
static const SensorConfig aggregateSensorConfigTemplate[] = {
    {"SOC", "SoC (scaled)", "%", "battery", always},
    {"SOC_real", "SoC (real)", "%", "battery", always},
    {"state_of_health", "State of Health", "%", "", always},
    {"battery_voltage", "Battery Voltage", "V", "voltage", always},
    {"battery_current", "Battery Current", "A", "current", always},
    {"stat_batt_power", "Battery Power", "W", "power", always},
    {"total_capacity", "Total Capacity (real)", "Wh", "energy", always},
    {"total_capacity_scaled", "Total Capacity (scaled)", "Wh", "energy", always},
    {"remaining_capacity_real", "Remaining Capacity (real)", "Wh", "energy", always},
    {"remaining_capacity", "Remaining Capacity (scaled)", "Wh", "energy", always},
    {"max_charge_power", "Max Charge Power", "W", "power", always},
    {"max_discharge_power", "Max Discharge Power", "W", "power", always},
    {"max_charge_current", "Max Charge Current", "A", "current", always},
    {"max_discharge_current", "Max Discharge Current", "A", "current", always},
    {"cell_max_voltage", "Cell Max Voltage", "V", "voltage", always},
    {"cell_min_voltage", "Cell Min Voltage", "V", "voltage", always},
    {"temperature_max", "Temperature Max", "°C", "temperature", always},
    {"temperature_min", "Temperature Min", "°C", "temperature", always},
    {"charged_energy", "Battery Charged Energy", "Wh", "energy", any_pack_supports_charged},
    {"discharged_energy", "Battery Discharged Energy", "Wh", "energy", any_pack_supports_charged},
    {"charging_state", "Charging State", "", "", always},
    {"limiting_factor", "Limiting Factor", "", "", always}};

static const SensorConfig globalSensorConfigTemplate[] = {
    {"bms_status", "BMS Status", "", "", always},
    {"pause_status", "Pause Status", "", "", always},
    {"event_level", "Event Level", "", "", always},
    {"emulator_status", "Emulator Status", "", "", always},
    {"emulator_uptime", "Emulator Uptime", "s", "duration", always},
    {"cpu_temp", "CPU Temperature", "°C", "temperature", always},
    {"software_version", "Emulator Version", "", "", always},
    // Internal-RAM heap diagnostics, mirroring the ESPHome debug component sensors
    // (free / block / min_free / fragmentation). Only published when enabled in settings.
    {"heap_free", "Heap Free", "B", "data_size", heap_metrics_enabled},
    {"heap_max_block", "Heap Max Block", "B", "data_size", heap_metrics_enabled},
    {"heap_min_free", "Heap Min Free", "B", "data_size", heap_metrics_enabled},
    {"heap_fragmentation", "Heap Fragmentation", "%", "", heap_metrics_enabled}};

// The battery instances the MQTT module publishes for. Battery #1 keeps the historical
// un-suffixed topic ("<name>/info") and entity ids, so single-battery setups see no change.
struct BatteryTarget {
  Battery** bat;                       // global battery instance pointer (may point to nullptr)
  const DATALAYER_BATTERY_TYPE* data;  // matching datalayer entry
  const bool* detected;                // set once at least one CAN frame was ever received
  int index;                           // 1-based battery number
  const char* id_suffix;               // suffix for entity ids / unique ids ("", "_2", "_3")
  const char* name_suffix;             // suffix for display names ("", " 2", " 3")
};

// Display-name suffix for a pack. Battery #1 is normally un-suffixed, but once there is more
// than one pack an unqualified "SoC" sitting next to "SoC 2" reads as the installation's rather
// than the first pack's, so it gets " 1" too. Entity ids and unique ids are deliberately left
// alone: renaming those would orphan every existing Home Assistant entity and break history.
static const char* display_name_suffix(const BatteryTarget& target) {
  if (target.index == 1 && datalayer.system.info.configured_batteries > 1) {
    return " 1";
  }
  return target.name_suffix;
}

static const BatteryTarget battery_targets[] = {
    {&battery, &datalayer.battery, &battery_detected, 1, "", ""},
    {&battery2, &datalayer.battery2, &battery2_detected, 2, "_2", " 2"},
    {&battery3, &datalayer.battery3, &battery3_detected, 3, "_3", " 3"},
};

// Per-battery state topics: "<name>/info", "<name>/info_2", "<name>/info_3".
// Built once in init_mqtt(). Publishing each battery to its own topic (with identical,
// un-suffixed JSON keys) keeps the payload size constant per battery instead of growing
// with the battery count.
static String info_topics[3];

// "<name>/info_multi", following the "<name>/info_2" pattern. Only used with several batteries.
static String aggregate_topic;

static const SensorConfig buttonConfigs[] = {{"BMSRESET", "Reset BMS", nullptr, nullptr, nullptr},
                                             {"PAUSE", "Pause charge/discharge", nullptr, nullptr, nullptr},
                                             {"RESUME", "Resume charge/discharge", nullptr, nullptr, nullptr},
                                             {"RESTART", "Reboot Emulator", nullptr, nullptr, nullptr},
                                             {"STOP", "Open Contactors", nullptr, nullptr, nullptr}};

// All commands the emulator subscribes to. The matching topics are precomputed once in
// init_mqtt() so that mqtt_message_received() does not rebuild temporary Strings on
// every received message.
enum ButtonCommand {
  BTN_BMSRESET = 0,
  BTN_PAUSE,
  BTN_RESUME,
  BTN_RESTART,
  BTN_STOP,
  BTN_SET_LIMITS,
  BTN_ESPNOW_RUN,
  BTN_SET_SCALESOC,
  BTN_COUNT
};
static const char* button_commands[BTN_COUNT] = {"BMSRESET", "PAUSE",      "RESUME",     "RESTART",
                                                 "STOP",     "SET_LIMITS", "ESPNOW_RUN", "SET_SCALESOC"};
static String button_command_topics[BTN_COUNT];

static String generateCommonInfoAutoConfigTopic(const char* entity_id) {
  return String(ha_autodiscovery_topic.c_str()) + "/sensor/" + topic_name + "/" + String(entity_id) + "/config";
}

static String generateCellVoltageAutoConfigTopic(int cell_number, String battery_suffix) {
  return String(ha_autodiscovery_topic.c_str()) + "/sensor/" + topic_name + "/cell_voltage" + battery_suffix +
         String(cell_number) + "/config";
}

static String generateEventsAutoConfigTopic(const char* entity_id) {
  return String(ha_autodiscovery_topic.c_str()) + "/sensor/" + topic_name + "/" + String(entity_id) + "/config";
}

static String generateButtonAutoConfigTopic(const char* subtype) {
  return String(ha_autodiscovery_topic.c_str()) + "/button/" + topic_name + "/" + String(subtype) + "/config";
}

static String generateSensorDefaultEntityId(const String& object_id) {
  return "sensor." + object_id;
}

bool set_common_discovery_attributes(JsonObjectWriter& doc) {
  char device_json[512];
  JsonObjectWriter device(device_json, sizeof(device_json));
  device.addStringArray("identifiers", device_id.c_str());
  device["model"] = "Battery Emulator";
  device["manufacturer"] = "FOSS";
  device["name"] = device_name;
  device["hw_version"] = esp32hal->name();
  device["sw_version"] = version_number;
  const String configuration_url = "http://" + network_localIP().toString();
  device["configuration_url"] = configuration_url;
  if (!device.finish()) {
    logging.println("MQTT discovery device JSON exceeds buffer");
    return false;
  }
  doc.addRaw("device", device_json);
  doc.addObjectStringArray("availability", "topic", lwt_topic.c_str());
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["enabled_by_default"] = true;
  return true;
}

void set_battery_voltage_attributes(JsonObjectWriter& doc, int i, int cellNumber, const String& state_topic,
                                    const String& default_entity_id_prefix, const String& battery_name_suffix) {
  const String default_entity_object_id = default_entity_id_prefix + "battery_voltage_cell" + String(cellNumber);
  doc["name"] = "Battery" + battery_name_suffix + " Cell Voltage " + String(cellNumber);
  doc["default_entity_id"] = generateSensorDefaultEntityId(default_entity_object_id);
  doc["unique_id"] = topic_name + default_entity_id_prefix + "_battery_voltage_cell" + String(cellNumber);
  doc["device_class"] = "voltage";
  doc["state_class"] = "measurement";
  doc["state_topic"] = state_topic;
  doc["unit_of_measurement"] = "V";
  doc["suggested_display_precision"] = 3;
  doc["icon"] = "mdi:current-dc";
  doc["value_template"] = "{{ value_json.cell_voltages[" + String(i) + "] }}";
}

static String generateButtonTopic(const char* subtype) {
  return topic_name + "/command/" + String(subtype);
}

static const char* get_balancing_status_text(balancing_status_enum status) {
  switch (status) {
    case BALANCING_STATUS_UNKNOWN:
      return "Unknown";
    case BALANCING_STATUS_ERROR:
      return "Error";
    case BALANCING_STATUS_READY:
      return "Ready";
    case BALANCING_STATUS_ACTIVE:
      return "Active";
    case BALANCING_STATUS_BLOCKED:
      return "Pending";  //Cells are flagged for balancing but the BMS is not bleeding them yet
    default:
      return "Unknown";
  }
}

// Fills the document with the state values for one battery. All keys are un-suffixed
// string literals, avoiding a temporary String allocation for every metric on every
// publish cycle and for every battery.
// Fills the document with datalayer.aggregate: the installation, not a pack. Keys match the
// per-battery ones where the meaning is the same, so a value_template reads the same either way.
static void set_aggregate_attributes(JsonObjectWriter& doc) {
  const DATALAYER_AGGREGATE_TYPE& a = datalayer.aggregate;
  doc["SOC"] = ((float)a.reported_soc) / 100.0f;
  doc["SOC_real"] = ((float)a.real_soc) / 100.0f;
  if (a.soh_available) {  // unknown in Home Assistant until some pack has decoded one
    doc["state_of_health"] = ((float)a.soh_pptt) / 100.0f;
  }
  doc["battery_voltage"] = ((float)a.voltage_dV) / 10.0f;
  doc["battery_current"] = ((float)a.current_dA) / 10.0f;
  doc["stat_batt_power"] = ((float)a.active_power_W);
  doc["total_capacity"] = ((float)a.total_capacity_Wh);
  doc["total_capacity_scaled"] = ((float)a.reported_total_capacity_Wh);
  doc["remaining_capacity_real"] = ((float)a.remaining_capacity_Wh);
  doc["remaining_capacity"] = ((float)a.reported_remaining_capacity_Wh);
  doc["max_charge_power"] = ((float)a.max_charge_power_W);
  doc["max_discharge_power"] = ((float)a.max_discharge_power_W);
  doc["max_charge_current"] = ((float)a.max_charge_current_dA) / 10.0f;
  doc["max_discharge_current"] = ((float)a.max_discharge_current_dA) / 10.0f;
  doc["cell_max_voltage"] = ((float)a.cell_max_voltage_mV) / 1000.0f;
  doc["cell_min_voltage"] = ((float)a.cell_min_voltage_mV) / 1000.0f;
  doc["temperature_max"] = ((float)a.temperature_max_dC) / 10.0f;
  doc["temperature_min"] = ((float)a.temperature_min_dC) / 10.0f;
  // Omitted unless some pack actually counts them, so Home Assistant shows "unknown" rather
  // than a lifetime total of 0 Wh that will never move.
  if (any_pack_supports_charged(nullptr)) {
    doc["charged_energy"] = ((float)a.total_charged_battery_Wh);
    doc["discharged_energy"] = ((float)a.total_discharged_battery_Wh);
  }
  const ChargingState charging_state = get_charging_state(a.current_dA);
  doc["charging_state"] = charging_state_to_text(charging_state);
  doc["limiting_factor"] = limiting_factor_to_text(get_limiting_factor(
      charging_state, datalayer.battery_settings.inverter_limits_charge,
      datalayer.battery_settings.inverter_limits_discharge, datalayer.battery_settings.user_settings_limit_charge,
      datalayer.battery_settings.user_settings_limit_discharge));
}

void set_battery_attributes(JsonObjectWriter& doc, const DATALAYER_BATTERY_TYPE& battery_data, int battery_index,
                            bool battery_supports_charged) {
  // Scaled figures are only a pack's own where that pack is the whole installation. With
  // several batteries the window is applied to datalayer.aggregate and published on its own
  // topic, and these keys would just repeat the real ones - so they are left out entirely
  // rather than published as duplicates. See single_pack().
  const bool pack_is_the_installation = (datalayer.system.info.configured_batteries < 2);
  if (pack_is_the_installation) {
    doc["SOC"] = ((float)battery_data.status.reported_soc) / 100.0f;
  }
  doc["SOC_real"] = ((float)battery_data.status.real_soc) / 100.0f;
  // Omit until the integration has decoded a real state of health, so HA shows "unknown"
  // instead of the soh_pptt default presented as if it had been read from the pack.
  if (battery_data.status.soh_available) {
    doc["state_of_health"] = ((float)battery_data.status.soh_pptt) / 100.0f;
  }
  doc["temperature_min"] = ((float)((int16_t)battery_data.status.temperature_min_dC)) / 10.0f;
  doc["temperature_max"] = ((float)((int16_t)battery_data.status.temperature_max_dC)) / 10.0f;
  // A current sensor fitted in place of the batteries' own stands in for these (pack_current_dA())
  doc["stat_batt_power"] = ((float)pack_power_W(battery_data.status));
  doc["battery_current"] = ((float)pack_current_dA(battery_data.status)) / 10.0f;
  doc["battery_voltage"] = ((float)battery_data.status.voltage_dV) / 10.0f;
  if (battery_data.info.number_of_cells != 0u &&
      battery_data.status.cell_voltages_mV[battery_data.info.number_of_cells - 1] != 0u) {
    doc["cell_max_voltage"] = ((float)battery_data.status.cell_max_voltage_mV) / 1000.0f;
    doc["cell_min_voltage"] = ((float)battery_data.status.cell_min_voltage_mV) / 1000.0f;
    doc["cell_voltage_delta"] =
        ((float)battery_data.status.cell_max_voltage_mV) - ((float)battery_data.status.cell_min_voltage_mV);
  }
  // Not every integration knows the pack capacity immediately (some derive it from
  // received data); omit until nonzero so HA shows "unknown" instead of 0 Wh.
  if (battery_data.info.total_capacity_Wh != 0u) {
    doc["total_capacity"] = ((float)battery_data.info.total_capacity_Wh);
  }
  doc["remaining_capacity_real"] = ((float)battery_data.status.remaining_capacity_Wh);
  if (pack_is_the_installation) {
    doc["remaining_capacity"] = ((float)battery_data.status.reported_remaining_capacity_Wh);
  }
  // max_charge_power_W on a pack is not that pack's own figure: the safety layer, the SOC taper
  // and the inverter filter all rewrite it in place, and for pack 1 that makes it the whole
  // installation's decision. With several packs publish what each BMS actually asked for, so
  // the three topics mean the same thing; the installation's limits are on the aggregate topic.
  // A single pack is the installation, so it keeps reporting the final limit as it always has.
  if (pack_is_the_installation) {
    doc["max_discharge_power"] = ((float)battery_data.status.max_discharge_power_W);
    doc["max_charge_power"] = ((float)battery_data.status.max_charge_power_W);
  } else {
    doc["max_discharge_power"] = ((float)battery_data.status.bms_max_discharge_power_W);
    doc["max_charge_power"] = ((float)battery_data.status.bms_max_charge_power_W);
  }
  // Omit until the integration has decoded a valid sample so HA shows "unknown"
  // instead of a false 0 kOhm at boot.
  if (battery_data.status.insulation_resistance_available) {
    doc["insulation_resistance"] = battery_data.status.insulation_resistance_kOhm;
  }

  if (battery_supports_charged) {
    // Note: reads the charged/discharged totals of THIS battery. The previous implementation
    // always read battery #1's totals, so batteries 2/3 reported battery #1's energy counters.
    if (battery_data.status.total_charged_battery_Wh != 0 && battery_data.status.total_discharged_battery_Wh != 0) {
      doc["charged_energy"] = ((float)battery_data.status.total_charged_battery_Wh);
      doc["discharged_energy"] = ((float)battery_data.status.total_discharged_battery_Wh);
    }
  }

  // Add balancing data
  uint16_t active_cells = 0;
  if (battery_data.info.number_of_cells != 0u) {
    for (size_t i = 0; i < battery_data.info.number_of_cells; ++i) {
      if (battery_data.status.cell_balancing_status[i]) {
        active_cells++;
      }
    }
  }
  doc["balancing_active_cells"] = active_cells;
  doc["balancing_status"] = get_balancing_status_text(battery_data.status.balancing_status);
  // Direction is genuinely this pack's: parallel packs at different SOC push current into each
  // other. What is limiting the inverter is not - that is one answer for the installation, so
  // with several packs it is published once on the aggregate topic instead of the same answer
  // appearing on every pack.
  ChargingState charging_state = get_charging_state(pack_current_dA(battery_data.status));
  doc["charging_state"] = charging_state_to_text(charging_state);
  if (pack_is_the_installation) {
    doc["limiting_factor"] = limiting_factor_to_text(get_limiting_factor(
        charging_state, datalayer.battery_settings.inverter_limits_charge,
        datalayer.battery_settings.inverter_limits_discharge, datalayer.battery_settings.user_settings_limit_charge,
        datalayer.battery_settings.user_settings_limit_discharge));
  }
  if (battery_index == 1 && supports_tesla_dcdc_metrics(::battery)) {
    doc["dc_dc_current"] = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent) * 0.1f;
    doc["dc_dc_voltage"] = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 0.01f;
  }
  if (supports_byd_autocal_metrics(::battery)) {
    const DATALAYER_INFO_BYDATTO3& byd =
        (battery_index == 2) ? datalayer_extended.bydAtto3_2 : datalayer_extended.bydAtto3;
    doc["autocal_taper"] = byd.autocal_crit_taper;
    doc["autocal_dwell_s"] = byd.autocal_dwell_accumulated_ms / 1000u;
    doc["autocal_cooldown_ready"] = byd.autocal_crit_cooldown_ready;
    doc["autocal_soc_drift"] = byd.autocal_drift_percent;
    static const char* const charge_session_text[] = {"Idle",     "Requesting", "Ready",
                                                      "Charging", "Finishing",  "Resting"};
    doc["charge_session"] = charge_session_text[byd.charge_session_state < 6 ? byd.charge_session_state : 0];
    doc["charge_grant"] = byd.charge_grant;
    char bms_mode[5];
    snprintf(bms_mode, sizeof(bms_mode), "0x%02X", byd.contactor_feedback);
    doc["charge_bms_mode"] = bms_mode;
    doc["charge_term_cell_max"] = byd.termination_cell_max_mV;
    doc["charge_term_cell_min"] = byd.termination_cell_min_mV;
    doc["charge_term_cell_delta"] = byd.termination_cell_delta_mV;
    doc["charge_term_cell_max_num"] = byd.termination_cell_max_number;
    doc["charge_term_cell_min_num"] = byd.termination_cell_min_number;
  }
  if (supports_byd_metrics(::battery)) {
    const DATALAYER_INFO_BYDATTO3& byd =
        (battery_index == 2) ? datalayer_extended.bydAtto3_2 : datalayer_extended.bydAtto3;
    doc["min_cell_number"] = byd.BMS_min_cell_voltage_number;
    doc["max_cell_number"] = byd.BMS_max_cell_voltage_number;
  }
  if (supports_leaf_metrics(::battery)) {
    const DATALAYER_INFO_NISSAN_LEAF& leaf = (battery_index == 3)   ? datalayer_extended.nissanleaf_3
                                             : (battery_index == 2) ? datalayer_extended.nissanleaf_2
                                                                    : datalayer_extended.nissanleaf;
    // Omit until a group 1 reply with a known layout has been decoded, so HA shows "unknown"
    // instead of a false 0 % before the first poll completes.
    if (leaf.battery_HX_pptt != 0u) {
      doc["leaf_hx"] = ((float)leaf.battery_HX_pptt) / 100.0f;
    }
    if (leaf.battery_SOHraw_pptt != 0u) {
      doc["leaf_soh_raw"] = ((float)leaf.battery_SOHraw_pptt) / 100.0f;
    }
    // Same treatment for the 12 V level: omitted until the pack has reported one, so it reads
    // unknown rather than 0.00 V until the first group 1 reply comes back.
    if (leaf.VBAT_mV != 0u) {
      doc["leaf_vbat"] = ((float)leaf.VBAT_mV) / 1000.0f;
    }
    // Pack capacity as reported, and the same figure as energy at the pack's nominal voltage,
    // which differs by generation. Both omitted until a capacity has been read.
    if (leaf.CapacityCAh != 0u) {
      const float capacity_Ah = ((float)leaf.CapacityCAh) / 100.0f;
      const float nominal_V = (leaf.LEAF_gen == 2) ? 350.4f : 360.0f;
      doc["leaf_capacity_ah"] = capacity_Ah;
      doc["leaf_capacity"] = (capacity_Ah * nominal_V) / 1000.0f;
    }
  }
}

static std::vector<EventData> order_events;

// Returns the MDI icon for an info/global discovery sensor, or nullptr to leave HA's
// device-class default. Per-entity matches win over device-class matches. Compared against
// the base (un-suffixed) entity ids, so all battery variants are covered.
static const char* sensor_discovery_icon(const char* entity_id, const char* device_class) {
  if (entity_id != nullptr) {
    if (strcmp(entity_id, "balancing_active_cells") == 0 || strcmp(entity_id, "balancing_status") == 0) {
      return "mdi:fuel-cell";
    }
    if (strcmp(entity_id, "bms_status") == 0) {
      return "mdi:information-box-outline";
    }
    if (strcmp(entity_id, "insulation_resistance") == 0) {
      return "mdi:resistor";
    }
    if (strcmp(entity_id, "state_of_health") == 0 || strcmp(entity_id, "leaf_soh_raw") == 0) {
      return "mdi:battery-heart-variant";
    }
    if (strcmp(entity_id, "leaf_hx") == 0) {
      return "mdi:battery-minus-variant";
    }
    // Amp-hours have no device_class, so this one would fall back to Home Assistant's generic
    // icon next to its kWh sibling, which does get one from "energy_storage".
    if (strcmp(entity_id, "leaf_capacity_ah") == 0) {
      return "mdi:car-battery";
    }
    if (strcmp(entity_id, "charging_state") == 0) {
      return "mdi:home-battery";
    }
    if (strcmp(entity_id, "limiting_factor") == 0) {
      return "mdi:home-battery-outline";
    }
    if (strcmp(entity_id, "emulator_status") == 0 || strcmp(entity_id, "event_level") == 0) {
      return "mdi:information-outline";
    }
    if (strcmp(entity_id, "pause_status") == 0) {
      return "mdi:battery-outline";
    }
    if (strcmp(entity_id, "software_version") == 0) {
      return "mdi:tag-outline";
    }
    if (strncmp(entity_id, "heap_", 5) == 0) {
      return "mdi:memory";
    }
  }
  if (device_class != nullptr) {
    if (strcmp(device_class, "voltage") == 0)
      return "mdi:current-dc";
    if (strcmp(device_class, "current") == 0)
      return "mdi:equal";
  }
  return nullptr;
}

// Returns the MDI icon for a command button, or nullptr.
static const char* button_discovery_icon(const char* command) {
  if (strcmp(command, "RESTART") == 0)
    return "mdi:restart";
  if (strcmp(command, "BMSRESET") == 0)
    return "mdi:star-four-points-box-outline";
  if (strcmp(command, "PAUSE") == 0)
    return "mdi:battery-minus-outline";
  if (strcmp(command, "RESUME") == 0)
    return "mdi:battery-sync-outline";
  if (strcmp(command, "STOP") == 0)
    return "mdi:battery-remove-outline";
  return nullptr;
}

// Publishes the HA discovery config for one sensor. The per-battery name / entity id /
// value_template variants are generated into stack buffers here instead of being strdup()'d
// into a permanent std::list at startup: discovery is one-shot, so nothing needs to stay
// on the heap for it.
// diagnostic: set for emulator-level sensors, which describe the emulator itself rather
// than the battery it is talking to. Home Assistant then files them under the device's
// Diagnostic section instead of the main sensor list.
static bool publish_sensor_discovery(const SensorConfig& config, const char* id_suffix, const char* name_suffix,
                                     const String& state_topic, bool diagnostic = false) {
  char entity_id[64];
  char name_buf[64];
  char value_template[96];
  snprintf(entity_id, sizeof(entity_id), "%s%s", config.entity_id, id_suffix);
  snprintf(name_buf, sizeof(name_buf), "%s%s", config.name, name_suffix);
  // The state topics are per-battery, so the value_template key is the base id for every battery
  snprintf(value_template, sizeof(value_template), "{{ value_json.%s | default(none) }}", config.entity_id);

  JsonObjectWriter& doc = shared_doc;
  doc["name"] = name_buf;
  doc["state_topic"] = state_topic;
  doc["unique_id"] = topic_name + "_" + String(entity_id);
  const String default_entity_object_id = default_entity_id_prefix + String(entity_id);
  doc["default_entity_id"] = generateSensorDefaultEntityId(default_entity_object_id);
  doc["value_template"] = value_template;
  const bool capacity_sensor = strncmp(config.entity_id, "total_capacity", strlen("total_capacity")) == 0 ||
                               strncmp(config.entity_id, "remaining_capacity", strlen("remaining_capacity")) == 0;
  const bool lifetime_energy =
      strcmp(config.entity_id, "charged_energy") == 0 || strcmp(config.entity_id, "discharged_energy") == 0;
  if (config.unit != nullptr && strlen(config.unit) > 0) {
    doc["unit_of_measurement"] = config.unit;
  }
  if (config.device_class != nullptr && strlen(config.device_class) > 0) {
    doc["device_class"] = capacity_sensor ? "energy_storage" : config.device_class;
    doc["state_class"] = lifetime_energy ? "total_increasing" : "measurement";
  }
  // "balancing_active_cells" is a numeric count with no device_class, so it misses the
  // state_class assignment above. Mark it as a measurement explicitly so Home Assistant
  // records long-term statistics for it.
  if (strcmp(config.entity_id, "balancing_active_cells") == 0) {
    doc["state_class"] = "measurement";
  }
  // "insulation_resistance" is a numeric value with no device_class, so it also misses the
  // state_class assignment above. Mark it as a measurement and display it as a whole
  // number of kOhm.
  if (strcmp(config.entity_id, "insulation_resistance") == 0) {
    doc["state_class"] = "measurement";
    doc["suggested_display_precision"] = 0;
  }
  // The state of health figures are percentages carried with no device_class, so they miss the
  // state_class assignment above too. Mark them as measurements and show two decimals, like
  // LeafSpy does.
  if (strcmp(config.entity_id, "leaf_hx") == 0 || strcmp(config.entity_id, "leaf_soh_raw") == 0 ||
      strcmp(config.entity_id, "state_of_health") == 0) {
    doc["state_class"] = "measurement";
    doc["suggested_display_precision"] = 2;
  }
  // "heap_fragmentation" is a percentage with no matching device_class either. Mark it as a
  // measurement and show one decimal, like the ESPHome debug sensor does.
  if (strcmp(config.entity_id, "heap_fragmentation") == 0) {
    doc["state_class"] = "measurement";
    doc["suggested_display_precision"] = 1;
  }
  // "energy" device_class is only valid with state_class total / total_increasing, never
  // "measurement" — HA rejects the combination. The capacity sensors represent a current
  // stored amount, so use "energy_storage" (compatible with "measurement") instead. The
  // charged/discharged sensors are genuine lifetime totals, so keep "energy" but use
  // "total_increasing".
  // Cell min/max voltages: show 3 decimals in HA so they don't round to the same integer
  // on display. Precision is intentionally not applied to battery_voltage. (Icons are
  // handled centrally below.)
  if (strcmp(config.entity_id, "cell_max_voltage") == 0 || strcmp(config.entity_id, "cell_min_voltage") == 0) {
    doc["suggested_display_precision"] = 3;
  }
  // The 12 V level is a small voltage where the second decimal carries the information, so it
  // gets the same treatment as the cell voltages above rather than the pack-voltage default.
  if (strcmp(config.entity_id, "leaf_vbat") == 0) {
    doc["suggested_display_precision"] = 2;
  }
  // Amp-hours have no matching device_class in Home Assistant, so the capacity in Ah misses the
  // state_class assignment above. Both capacity sensors are shown to two decimals: degradation
  // moves them slowly enough that the second decimal is the interesting part.
  if (strcmp(config.entity_id, "leaf_capacity_ah") == 0) {
    doc["state_class"] = "measurement";
  }
  if (strcmp(config.entity_id, "leaf_capacity_ah") == 0 || strcmp(config.entity_id, "leaf_capacity") == 0) {
    doc["suggested_display_precision"] = 2;
  }
  // Battery current, CPU temp and both SOC sensors: show 1 decimal in HA.
  if (strcmp(config.entity_id, "battery_current") == 0 || strcmp(config.entity_id, "cpu_temp") == 0 ||
      strncmp(config.entity_id, "SOC", strlen("SOC")) == 0) {
    doc["suggested_display_precision"] = 1;
  }
  // Entity icons (centralized): status sensors by entity id, all voltage/current sensors
  // by device_class. This also covers the balancing and cell min/max entities above.
  {
    const char* icon = sensor_discovery_icon(config.entity_id, config.device_class);
    if (icon != nullptr) {
      doc["icon"] = icon;
    }
  }
  if (diagnostic) {
    doc["entity_category"] = "diagnostic";
  }
  if (!set_common_discovery_attributes(doc) || !serialize_mqtt_json(doc)) {
    return false;
  }
  bool ok = mqtt_publish(generateCommonInfoAutoConfigTopic(entity_id).c_str(), mqtt_msg, true);
  doc.clear();
  return ok;
}

static bool publish_common_info(void) {

  if (ha_autodiscovery_enabled && !ha_common_info_published) {
    DocClearGuard guard(shared_doc);
    // Battery sensors: one discovery config per template entry per present battery.
    for (const auto& target : battery_targets) {
      Battery* bat = *target.bat;
      if (bat == nullptr) {
        continue;
      }
      for (const auto& config : batterySensorConfigTemplate) {
        if (!config.condition(bat)) {
          continue;
        }
        if (!publish_sensor_discovery(config, target.id_suffix, display_name_suffix(target),
                                      info_topics[target.index - 1])) {
          return false;
        }
      }
    }
    // The installation-level entities, only where there is an installation to speak of.
    if (datalayer.system.info.configured_batteries > 1) {
      for (const auto& config : aggregateSensorConfigTemplate) {
        // No single Battery to ask about an installation-wide entity; the conditions here take
        // nullptr and look at the configured packs themselves.
        if (!config.condition(nullptr)) {
          continue;
        }
        if (!publish_sensor_discovery(config, "_multi", "", aggregate_topic)) {
          return false;
        }
      }
    }
    // Global (emulator-level) sensors stay on battery #1's "/info" topic. They all describe
    // the emulator rather than the battery, so they are published as diagnostic entities.
    for (const auto& config : globalSensorConfigTemplate) {
      // Emulator-level sensors have no battery instance; the condition only gates on settings.
      if (!config.condition(nullptr)) {
        continue;
      }
      if (!publish_sensor_discovery(config, "", "", info_topics[0], true)) {
        return false;
      }
    }
    ha_common_info_published = true;

  } else {
    // State publishing: each battery gets its own topic with identical un-suffixed keys.
    // "<name>/info" carries the global emulator values plus battery #1, exactly as before,
    // so single-battery setups and raw-topic consumers of battery #1 see no change.
    {
      DocClearGuard guard(shared_doc);
      JsonObjectWriter& doc = shared_doc;
      doc["bms_status"] = getBMSStatus(datalayer.system.status.system_status);
      doc["pause_status"] = get_emulator_pause_status();

      //only publish these values once the battery was actually seen on CAN (battery_detected)
      //and we are still communicating with it. CAN_battery_still_alive alone is not enough:
      //it starts as a nonzero countdown at boot, so for up to ~60 s it is truthy before the
      //first frame ever arrived - publishing datalayer defaults (SOC 0%, 0.0 V, SOH 99%)
      //as if they were real. Gating on detection makes HA show "unknown" until data exists.
      if (battery_detected && datalayer.battery.status.CAN_battery_still_alive && allowed_to_send_CAN &&
          esp32hal->system_booted_up()) {
        set_battery_attributes(doc, datalayer.battery, 1, battery->supports_charged_energy());
      }

      doc["event_level"] = get_event_level_string(get_event_level());
      doc["emulator_status"] = get_emulator_status_string(get_emulator_status());
      // Static identity of the running binary. Published on every cycle (the topic is not
      // retained, so a single publish would be lost on a Home Assistant restart) and stored
      // zero-copy, both being const char* literals.
      doc["hardware"] = esp32hal->name();
      doc["software_version"] = version_number;
      if (datalayer.system.info.CPU_measurement_enabled) {
        doc["cpu_temp"] = datalayer.system.info.CPU_temperature;
      }
      doc["emulator_uptime"] = millis64() / 1000;
      doc["espnow_running"] = espnow_is_running() ? 1 : 0;

      // Internal-RAM heap diagnostics. Same sources and fragmentation formula as the ESPHome
      // debug component, so the values are directly comparable with an ESPHome node's.
      if (mqtt_publish_heap_metrics) {
        const uint32_t heap_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        const uint32_t heap_max_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        doc["heap_free"] = heap_free;
        doc["heap_max_block"] = heap_max_block;
        doc["heap_min_free"] = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        // Share of the free heap that is not reachable as one contiguous block. Guarded
        // against a zero free heap so no NaN is ever published.
        if (heap_free > 0u) {
          doc["heap_fragmentation"] = 100.0f - (100.0f * (float)heap_max_block / (float)heap_free);
        }
      }

      if (!serialize_mqtt_json(doc)) {
        return false;
      }
      if (mqtt_publish(info_topics[0].c_str(), mqtt_msg, false) == false) {
        log_publish_failure("Common info");
        return false;
      }
    }

    // The installation on "/info_multi". Nothing to aggregate with a single pack.
    if (datalayer.system.info.configured_batteries > 1) {
      DocClearGuard guard(shared_doc);
      set_aggregate_attributes(shared_doc);
      if (!serialize_mqtt_json(shared_doc)) {
        return false;
      }
      if (mqtt_publish(aggregate_topic.c_str(), mqtt_msg, false) == false) {
        log_publish_failure("Aggregate info");
        return false;
      }
    }

    // Batteries #2 and #3 on "/info_2" and "/info_3".
    for (const auto& target : battery_targets) {
      Battery* bat = *target.bat;
      if (target.index == 1 || bat == nullptr) {
        continue;
      }
      //only publish once this battery was actually seen on CAN and is still communicating
      //(see the battery #1 comment above for why detection matters at boot)
      if (*target.detected && target.data->status.CAN_battery_still_alive && allowed_to_send_CAN &&
          esp32hal->system_booted_up()) {
        DocClearGuard guard(shared_doc);
        set_battery_attributes(shared_doc, *target.data, target.index, bat->supports_charged_energy());
        if (!serialize_mqtt_json(shared_doc)) {
          return false;
        }
        if (mqtt_publish(info_topics[target.index - 1].c_str(), mqtt_msg, false) == false) {
          log_publish_failure("Common info");
          return false;
        }
      }
    }
  }
  return true;
}

// --- Manual JSON serialization for the large, flat array payloads -------------------------
//
// The cell voltage and balancing state payloads are the largest recurring messages (up to
// ~100+ elements per battery) but structurally trivial: {"key":[...]}. Serializing them
// with snprintf straight into mqtt_msg avoids extra work beyond the output buffer.

// Both arrays are the same length, sourced from the same snapshot and published on the
// same cadence, so they go out as one message on the spec_data topic:
//   {"cell_voltages":[...],"cell_balancing":[...]}
// This halves the number of publishes for the largest recurring payload. Home Assistant
// discovery is unaffected: the per-cell entities read value_json.cell_voltages[N] from
// this same topic, and no discovery config ever referenced balancing_data.
static bool publish_cell_data_state(const DATALAYER_BATTERY_TYPE& battery_data, const String& state_topic) {
  if (battery_data.info.number_of_cells == 0u) {
    return true;  // nothing populated yet, not an error
  }
  // Cell voltages are only included once the BMS has actually filled them in; the
  // balancing flags are valid as soon as the cell count is known.
  const bool voltages_valid = battery_data.status.cell_voltages_mV[battery_data.info.number_of_cells - 1] != 0u;

  size_t len = snprintf(mqtt_msg, sizeof(mqtt_msg), "{");

  if (voltages_valid) {
    len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "\"cell_voltages\":[");
    for (size_t i = 0; i < battery_data.info.number_of_cells; ++i) {
      if (len >= sizeof(mqtt_msg) - 32) {  // headroom for this element plus the closing "]}"
        logging.println("Cell data MQTT msg too large for buffer");
        return true;  // skip this payload, don't abort the publish cycle
      }
      // A zero here means the BMS returned no reading for that cell, which is not the same as
      // 0.000 V. Sent as JSON null so the per-cell HA entity goes unknown and a chart drawn from
      // this array leaves a gap rather than a spike to the bottom of the scale.
      if (battery_data.status.cell_voltages_mV[i] == 0u) {
        len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "%snull", (i != 0u) ? "," : "");
      } else {
        len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "%s%.3f", (i != 0u) ? "," : "",
                        ((float)battery_data.status.cell_voltages_mV[i]) / 1000.0f);
      }
    }
    len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "],");
  }

  len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "\"cell_balancing\":[");
  for (size_t i = 0; i < battery_data.info.number_of_cells; ++i) {
    if (len >= sizeof(mqtt_msg) - 32) {  // headroom for this element plus the closing "]}"
      logging.println("Cell data MQTT msg too large for buffer");
      return true;  // skip this payload, don't abort the publish cycle
    }
    len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "%s%s", (i != 0u) ? "," : "",
                    battery_data.status.cell_balancing_status[i] ? "true" : "false");
  }
  len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "]}");

  if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
    log_publish_failure("Cell data");
    return false;
  }
  return true;
}

// Publishes the per-cell HA discovery configs for one battery. Returns false on publish
// failure. ready is set to false if the battery's cell count is not initialized yet, so
// the caller retries discovery on a later cycle.
static bool publish_cell_voltage_discovery(const DATALAYER_BATTERY_TYPE& battery_data, const String& state_topic,
                                           const String& entity_prefix, const String& battery_name_suffix,
                                           const char* topic_suffix, bool& ready) {
  // If the cell voltage number isn't initialized...
  if (battery_data.info.number_of_cells == 0u) {
    ready = false;
    return true;
  }

  JsonObjectWriter& doc = shared_doc;
  for (int i = 0; i < battery_data.info.number_of_cells; i++) {
    int cellNumber = i + 1;
    set_battery_voltage_attributes(doc, i, cellNumber, state_topic, entity_prefix, battery_name_suffix);
    if (!set_common_discovery_attributes(doc)) {
      return false;
    }

    if (!serialize_mqtt_json(doc)) {
      return false;
    }
    const bool published =
        mqtt_publish(generateCellVoltageAutoConfigTopic(cellNumber, topic_suffix).c_str(), mqtt_msg, true);
    doc.clear();
    if (!published) {
      return false;
    }
  }
  doc.clear();  // clear after sending autoconfig
  return true;
}

static bool publish_cell_voltages(void) {
  static String state_topic = topic_name + "/spec_data";
  static String state_topic_2 = topic_name + "/spec_data_2";
  static String state_topic_3 = topic_name + "/spec_data_3";

  if (ha_autodiscovery_enabled && !ha_cell_voltages_published) {
    DocClearGuard guard(shared_doc);
    bool all_ready = true;

    const String first_pack_name_suffix = (datalayer.system.info.configured_batteries > 1) ? " 1" : "";
    if (!publish_cell_voltage_discovery(datalayer.battery, state_topic, default_entity_id_prefix,
                                        first_pack_name_suffix, "", all_ready)) {
      return false;
    }
    if (battery2) {
      if (!publish_cell_voltage_discovery(datalayer.battery2, state_topic_2, default_entity_id_prefix + "2_", " 2",
                                          "_2_", all_ready)) {
        return false;
      }
    }
    if (battery3) {
      if (!publish_cell_voltage_discovery(datalayer.battery3, state_topic_3, default_entity_id_prefix + "3_", " 3",
                                          "_3_", all_ready)) {
        return false;
      }
    }
    if (all_ready) {
      ha_cell_voltages_published = true;
    }
  }

  // State payloads only on the 60 s cadence; discovery above retries on every cycle
  // until complete, so HA entities still appear promptly after boot.
  if (!cell_data_due) {
    return true;
  }

  if (!publish_cell_data_state(datalayer.battery, state_topic)) {
    return false;
  }
  if (battery2 && !publish_cell_data_state(datalayer.battery2, state_topic_2)) {
    return false;
  }
  if (battery3 && !publish_cell_data_state(datalayer.battery3, state_topic_3)) {
    return false;
  }
  // All batteries published: done until the timer next elapses. On any failure above the
  // flag stays set, so the whole round is retried on the next publish cycle instead of
  // waiting a full minute.
  cell_data_due = false;
  return true;
}

bool publish_events() {
  static String state_topic = topic_name + "/events";
  if (ha_autodiscovery_enabled && !ha_events_published) {
    DocClearGuard guard(shared_doc);
    JsonObjectWriter& doc = shared_doc;

    doc["name"] = "Event";
    doc["state_topic"] = state_topic;
    doc["unique_id"] = topic_name + "_event";
    doc["default_entity_id"] = generateSensorDefaultEntityId(default_entity_id_prefix + "event");
    doc["value_template"] =
        "{{ value_json.event_type ~ ' (c:' ~ value_json.count ~ ',m:' ~  value_json.millis ~ ') ' ~ value_json.message "
        "}}";
    doc["json_attributes_topic"] = state_topic;
    doc["json_attributes_template"] = "{{ value_json | tojson }}";
    doc["icon"] = "mdi:information-outline";
    doc["entity_category"] = "diagnostic";
    if (!set_common_discovery_attributes(doc) || !serialize_mqtt_json(doc)) {
      return false;
    }
    if (mqtt_publish(generateEventsAutoConfigTopic("event").c_str(), mqtt_msg, true)) {
      ha_events_published = true;
    } else {
      return false;
    }
  } else {
    DocClearGuard guard(shared_doc);
    JsonObjectWriter& doc = shared_doc;
    const EVENTS_STRUCT_TYPE* event_pointer;

    //clear the vector
    order_events.clear();
    // Collect all events
    for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
      event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
      if (event_pointer->occurences > 0 && !event_pointer->MQTTpublished) {
        order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
      }
    }
    // Sort events by timestamp
    std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampAsc);

    for (const auto& event : order_events) {

      EVENTS_ENUM_TYPE event_handle = event.event_handle;
      event_pointer = event.event_pointer;

      // get_event_enum_string / get_event_level_string return const char*: assign directly
      // (stored by pointer, payload unchanged) instead of wrapping in a temporary String.
      // count/data/millis intentionally stay String-typed so the published JSON is
      // byte-identical to previous releases.
      doc["event_type"] = get_event_enum_string(event_handle);
      doc["severity"] = get_event_level_string(event_handle);
      doc["count"] = String(event_pointer->occurences);
      doc["data"] = String(event_pointer->data);
      doc["message"] = get_event_message_string(event_handle);
      doc["millis"] = String(event_pointer->timestamp);

      if (!serialize_mqtt_json(doc)) {
        return false;
      }
      if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
        log_publish_failure("Event");
        return false;
      } else {
        set_event_MQTTpublished(event_handle);
      }
      doc.clear();
    }
    // Clearing must happen AFTER the loop: the previous in-loop clear invalidated the
    // iterators of the vector being iterated (undefined behavior with >1 pending event).
    order_events.clear();
  }
  return true;
}

static bool publish_buttons_discovery(void) {
  if (ha_autodiscovery_enabled) {
    if (ha_buttons_published == false) {
      DocClearGuard guard(shared_doc);
      JsonObjectWriter& doc = shared_doc;
      for (int i = 0; i < sizeof(buttonConfigs) / sizeof(buttonConfigs[0]); i++) {
        const SensorConfig& config = buttonConfigs[i];
        doc["name"] = config.name;
        doc["unique_id"] = default_entity_id_prefix + config.entity_id;
        doc["command_topic"] = generateButtonTopic(config.entity_id);
        {
          const char* icon = button_discovery_icon(config.entity_id);
          if (icon != nullptr) {
            doc["icon"] = icon;
          }
        }
        // Rebooting the emulator is a maintenance action on the emulator itself, not a
        // battery control like the pause/resume/stop buttons.
        if (strcmp(config.entity_id, "RESTART") == 0) {
          doc["entity_category"] = "diagnostic";
        }
        if (!set_common_discovery_attributes(doc) || !serialize_mqtt_json(doc)) {
          return false;
        }
        if (!mqtt_publish(generateButtonAutoConfigTopic(config.entity_id).c_str(), mqtt_msg, true)) {
          return false;
        }
        doc.clear();
      }
      // Only mark done once ALL button configs went out. The old code set the flag on the
      // first successful publish, so a mid-loop failure left the remaining buttons
      // undiscovered forever.
      ha_buttons_published = true;
    }
  }
  return true;
}

static void subscribe() {
  esp_mqtt_client_subscribe(client, (topic_name + "/command/+").c_str(), 1);
}

struct MqttJsonNumber {
  bool is_number = false;
  bool is_integer = false;
  bool is_int = false;
  bool is_float = false;
  float value = 0.0f;
  int value_as_int = 0;
};

class MqttCommandJsonParser {
 public:
  MqttCommandJsonParser(const char* input, size_t length) : input_(input), length_(length) {}

  bool parse() {
    skipWhitespace();
    if (peek() == '{') {
      if (!parseObject(0, true)) {
        return false;
      }
    } else if (!skipValue(0)) {
      return false;
    }
    skipWhitespace();
    return position_ == length_;
  }

  MqttJsonNumber max_charge;
  MqttJsonNumber max_discharge;
  MqttJsonNumber timeout;
  MqttJsonNumber max_pct;
  MqttJsonNumber min_pct;

 private:
  char peek() const { return position_ < length_ ? input_[position_] : '\0'; }

  void skipWhitespace() {
    while (position_ < length_ && (input_[position_] == ' ' || input_[position_] == '\t' || input_[position_] == '\n' ||
                                   input_[position_] == '\r')) {
      ++position_;
    }
  }

  bool consume(char expected) {
    if (peek() != expected) {
      return false;
    }
    ++position_;
    return true;
  }

  bool parseString(char* decoded, size_t decoded_capacity) {
    if (!consume('"')) {
      return false;
    }
    size_t decoded_length = 0;
    bool decoded_overflow = false;
    while (position_ < length_) {
      unsigned char current = static_cast<unsigned char>(input_[position_++]);
      if (current == '"') {
        if (decoded != nullptr && decoded_capacity != 0) {
          decoded[decoded_length] = '\0';
        }
        return true;
      }
      if (current < 0x20) {
        return false;
      }
      if (current == '\\') {
        if (position_ >= length_) {
          return false;
        }
        const char escape = input_[position_++];
        switch (escape) {
          case '"':
          case '\\':
          case '/':
            current = static_cast<unsigned char>(escape);
            break;
          case 'b':
            current = '\b';
            break;
          case 'f':
            current = '\f';
            break;
          case 'n':
            current = '\n';
            break;
          case 'r':
            current = '\r';
            break;
          case 't':
            current = '\t';
            break;
          case 'u': {
            uint16_t codepoint;
            if (!parseHexCodepoint(codepoint)) {
              return false;
            }
            if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
              if (position_ + 2 > length_ || input_[position_] != '\\' || input_[position_ + 1] != 'u') {
                return false;
              }
              position_ += 2;
              uint16_t low;
              if (!parseHexCodepoint(low) || low < 0xdc00 || low > 0xdfff) {
                return false;
              }
              codepoint = '?';
            } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) {
              return false;
            }
            current = codepoint <= 0x7f ? static_cast<unsigned char>(codepoint) : '?';
            break;
          }
          default:
            return false;
        }
      }
      if (decoded != nullptr && decoded_capacity != 0) {
        if (current == '\0') {
          current = '?';
        }
        if (decoded_length + 1 < decoded_capacity) {
          decoded[decoded_length++] = static_cast<char>(current);
        } else {
          decoded_overflow = true;
        }
      }
    }
    (void)decoded_overflow;
    return false;
  }

  bool parseHexCodepoint(uint16_t& codepoint) {
    if (position_ + 4 > length_) {
      return false;
    }
    codepoint = 0;
    for (int i = 0; i < 4; ++i) {
      const char digit = input_[position_++];
      uint8_t value;
      if (digit >= '0' && digit <= '9') {
        value = static_cast<uint8_t>(digit - '0');
      } else if (digit >= 'a' && digit <= 'f') {
        value = static_cast<uint8_t>(digit - 'a' + 10);
      } else if (digit >= 'A' && digit <= 'F') {
        value = static_cast<uint8_t>(digit - 'A' + 10);
      } else {
        return false;
      }
      codepoint = static_cast<uint16_t>((codepoint << 4) | value);
    }
    return true;
  }

  MqttJsonNumber* fieldForKey(const char* key) {
    if (strcmp(key, "max_charge") == 0) {
      return &max_charge;
    }
    if (strcmp(key, "max_discharge") == 0) {
      return &max_discharge;
    }
    if (strcmp(key, "timeout") == 0) {
      return &timeout;
    }
    if (strcmp(key, "max_pct") == 0) {
      return &max_pct;
    }
    if (strcmp(key, "min_pct") == 0) {
      return &min_pct;
    }
    return nullptr;
  }

  bool parseNumber(MqttJsonNumber* result) {
    const size_t start = position_;
    consume('-');
    if (consume('0')) {
      if (peek() >= '0' && peek() <= '9') {
        return false;
      }
    } else {
      if (peek() < '1' || peek() > '9') {
        return false;
      }
      do {
        ++position_;
      } while (peek() >= '0' && peek() <= '9');
    }

    bool integer = true;
    if (consume('.')) {
      integer = false;
      if (peek() < '0' || peek() > '9') {
        return false;
      }
      do {
        ++position_;
      } while (peek() >= '0' && peek() <= '9');
    }
    if (peek() == 'e' || peek() == 'E') {
      integer = false;
      ++position_;
      if (peek() == '+' || peek() == '-') {
        ++position_;
      }
      if (peek() < '0' || peek() > '9') {
        return false;
      }
      do {
        ++position_;
      } while (peek() >= '0' && peek() <= '9');
    }

    if (result == nullptr) {
      return true;
    }
    result->is_number = true;
    result->is_integer = integer;
    const size_t token_length = position_ - start;
    if (token_length >= 64) {
      return true;
    }
    char token[64];
    memcpy(token, input_ + start, token_length);
    token[token_length] = '\0';
    char* end = nullptr;
    const double parsed = strtod(token, &end);
    if (end == token + token_length && isfinite(parsed) && parsed <= FLT_MAX && parsed >= -FLT_MAX) {
      result->value = static_cast<float>(parsed);
      result->is_float = true;
    }
    if (integer) {
      char* int_end = nullptr;
      const long long integer_value = strtoll(token, &int_end, 10);
      if (int_end == token + token_length && integer_value >= INT_MIN && integer_value <= INT_MAX) {
        result->is_int = true;
        result->value_as_int = static_cast<int>(integer_value);
      }
    }
    return true;
  }

  bool parseObject(unsigned int depth, bool extract_fields) {
    if (depth > 16 || !consume('{')) {
      return false;
    }
    skipWhitespace();
    if (consume('}')) {
      return true;
    }
    while (position_ < length_) {
      char key[32] = {};
      if (!parseString(key, sizeof(key))) {
        return false;
      }
      skipWhitespace();
      if (!consume(':')) {
        return false;
      }
      skipWhitespace();
      MqttJsonNumber* field = extract_fields ? fieldForKey(key) : nullptr;
      if (field != nullptr) {
        *field = MqttJsonNumber();
      }
      if (peek() == '-' || (peek() >= '0' && peek() <= '9')) {
        if (!parseNumber(field)) {
          return false;
        }
      } else if (!skipValue(depth + 1)) {
        return false;
      }
      skipWhitespace();
      if (consume('}')) {
        return true;
      }
      if (!consume(',')) {
        return false;
      }
      skipWhitespace();
    }
    return false;
  }

  bool skipValue(unsigned int depth) {
    if (depth > 16) {
      return false;
    }
    skipWhitespace();
    if (peek() == '"') {
      return parseString(nullptr, 0);
    }
    if (peek() == '{') {
      return parseObject(depth + 1, false);
    }
    if (consume('[')) {
      skipWhitespace();
      if (consume(']')) {
        return true;
      }
      while (position_ < length_) {
        if (!skipValue(depth + 1)) {
          return false;
        }
        skipWhitespace();
        if (consume(']')) {
          return true;
        }
        if (!consume(',')) {
          return false;
        }
        skipWhitespace();
      }
      return false;
    }
    if (peek() == '-' || (peek() >= '0' && peek() <= '9')) {
      return parseNumber(nullptr);
    }
    if (matchLiteral("true") || matchLiteral("false") || matchLiteral("null")) {
      return true;
    }
    return false;
  }

  bool matchLiteral(const char* literal) {
    const size_t literal_length = strlen(literal);
    if (position_ + literal_length > length_ || memcmp(input_ + position_, literal, literal_length) != 0) {
      return false;
    }
    position_ += literal_length;
    return true;
  }

  const char* input_;
  size_t length_;
  size_t position_ = 0;
};

void mqtt_message_received(char* topic_raw, int topic_len, char* data, int data_len) {

  char* topic = strndup(topic_raw, topic_len);

  logging.printf("MQTT message arrived: [%.*s]\n", topic_len, topic);

  if (remote_bms_reset) {
    if (strcmp(topic, button_command_topics[BTN_BMSRESET].c_str()) == 0) {
      logging.println("Triggering BMS reset");
      start_bms_reset();
    }
  }

  if (strcmp(topic, button_command_topics[BTN_PAUSE].c_str()) == 0) {
    setBatteryPause(true, false);
  }

  if (strcmp(topic, button_command_topics[BTN_RESUME].c_str()) == 0) {
    setBatteryPause(false, false, EquipmentStop::RESUME);
  }

  if (strcmp(topic, button_command_topics[BTN_RESTART].c_str()) == 0) {
    hold_pins_across_reset();
    graceful_restart();
  }

  if (strcmp(topic, button_command_topics[BTN_STOP].c_str()) == 0) {
    setBatteryPause(true, false, EquipmentStop::STOP);
  }

  // "1" starts ESP-NOW if it is not running, "0" stops it if it is. Runtime only: the
  // "Start ESPNow at boot" setting stored in NVS is not changed.
  if (strcmp(topic, button_command_topics[BTN_ESPNOW_RUN].c_str()) == 0) {
    int start = 0;
    while (start < data_len && isspace((unsigned char)data[start])) {
      start++;
    }
    int end = data_len;
    while (end > start && isspace((unsigned char)data[end - 1])) {
      end--;
    }
    if (end - start == 1 && data[start] == '1') {
      logging.println("MQTT: starting ESPNow");
      request_espnow_running(true);
    } else if (end - start == 1 && data[start] == '0') {
      logging.println("MQTT: stopping ESPNow");
      request_espnow_running(false);
    } else {
      logging.printf("MQTT: invalid ESPNOW_RUN payload [%.*s], expected 1 or 0\n", data_len, data);
    }
  }

  if (strcmp(topic, button_command_topics[BTN_SET_LIMITS].c_str()) == 0) {
    MqttCommandJsonParser parser(data, data_len > 0 ? static_cast<size_t>(data_len) : 0u);
    const bool valid_json = data_len >= 0 && parser.parse();
    if (!valid_json) {
      logging.printf("MQTT: SET_LIMITS has invalid JSON payload [%.*s]\n", data_len, data);
    }

    if (valid_json && parser.max_charge.is_int) {
      datalayer.battery_settings.max_remote_set_charge_dA = parser.max_charge.value_as_int;
      datalayer.battery_settings.remote_settings_limit_charge = true;
    } else {
      datalayer.battery_settings.max_remote_set_charge_dA = 0;
      datalayer.battery_settings.remote_settings_limit_charge = false;
    }

    if (valid_json && parser.max_discharge.is_int) {
      datalayer.battery_settings.max_remote_set_discharge_dA = parser.max_discharge.value_as_int;
      datalayer.battery_settings.remote_settings_limit_discharge = true;
    } else {
      datalayer.battery_settings.max_remote_set_discharge_dA = 0;
      datalayer.battery_settings.remote_settings_limit_discharge = false;
    }

    if (valid_json && parser.timeout.is_int) {
      datalayer.battery_settings.remote_set_timeout = static_cast<uint32_t>(parser.timeout.value_as_int) * 1000u;
    } else {
      datalayer.battery_settings.remote_set_timeout = 30000;
    }

    datalayer.battery_settings.remote_set_timestamp = millis();
  }

  // Runtime change of the SOC rescale limits. Payload: {"max_pct": 50.0-100.0, "min_pct": -10.0-50.0}.
  // Either key may be omitted to leave that limit unchanged. Only the live datalayer values are
  // touched (nothing is written to NVS), so a reboot restores the saved settings.
  if (strcmp(topic, button_command_topics[BTN_SET_SCALESOC].c_str()) == 0) {
    if (!datalayer.battery_settings.soc_scaling_active) {
      // Limits have no effect without "Rescale SOC", so the command is ignored.
    } else {
      MqttCommandJsonParser parser(data, data_len > 0 ? static_cast<size_t>(data_len) : 0u);
      const bool valid_json = data_len >= 0 && parser.parse();
      if (!valid_json) {
        logging.printf("MQTT: SET_SCALESOC has invalid JSON payload [%.*s]\n", data_len, data);
      } else {
        // The datalayer stores these in 0.01 % units (8000 = 80.0 %), hence the *100.
        if (parser.max_pct.is_float) {
          float max_pct = parser.max_pct.value;
          if (max_pct >= 50.0f && max_pct <= 100.0f) {
            datalayer.battery_settings.max_percentage = (uint16_t)lroundf(max_pct * 100.0f);
          } else {
            logging.printf("MQTT: SET_SCALESOC max_pct %.1f out of range (50.0-100.0), ignored\n", max_pct);
          }
        }
        if (parser.min_pct.is_float) {
          float min_pct = parser.min_pct.value;
          if (min_pct >= -10.0f && min_pct <= 50.0f) {
            datalayer.battery_settings.min_percentage = (int16_t)lroundf(min_pct * 100.0f);
          } else {
            logging.printf("MQTT: SET_SCALESOC min_pct %.1f out of range (-10.0-50.0), ignored\n", min_pct);
          }
        }
      }
    }
  }

  free(topic);
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      clear_event(EVENT_MQTT_DISCONNECT);
      set_event(EVENT_MQTT_CONNECT, 0);

      // Standard LWT pattern: announce availability once, retained, on connect. The
      // broker serves it to late subscribers and replaces it with the retained
      // "offline" last-will when the session drops — no per-cycle re-publish needed.
      mqtt_publish(lwt_topic.c_str(), "online", true);

      // Handed to the MQTT task instead of published here, see pending_buttons_discovery.
      pending_buttons_discovery = true;
      subscribe();
      break;
    case MQTT_EVENT_DISCONNECTED:
      set_event(EVENT_MQTT_DISCONNECT, 0);  // also printing a log entry
      break;
    case MQTT_EVENT_DATA:
      mqtt_message_received(event->topic, event->topic_len, event->data, event->data_len);
      break;
    case MQTT_EVENT_ERROR:
      // logging.println("MQTT_ERROR");
      // logging.print("reported from esp-tls");
      // logging.println(event->error_handle->esp_tls_last_esp_err);
      // logging.print("reported from tls stack");
      // logging.println(event->error_handle->esp_tls_stack_err);
      // logging.print("captured as transport's socket errno");
      // logging.println(strerror(event->error_handle->esp_transport_sock_errno));
      break;
    case MQTT_EVENT_SUBSCRIBED:
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      break;
    case MQTT_EVENT_PUBLISHED:
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      break;
    case MQTT_EVENT_DELETED:
      break;
    case MQTT_USER_EVENT:
      break;
    case MQTT_EVENT_ANY:
      break;
  }
}

bool init_mqtt(void) {

  if (battery == nullptr) {
    logging.println("ERROR: No battery selected. Aborting MQTT initialization");
    return false;
  }

  String hostname = active_hostname();
  topic_name = hostname;
  default_entity_id_prefix = hostname + "_";
  device_name = hostname;
  device_id = hostname;

  // Precompute the per-battery state topics and the command topics once, so the publish
  // and receive paths don't rebuild them as temporary Strings on every cycle / message.
  for (const auto& target : battery_targets) {
    info_topics[target.index - 1] = topic_name + "/info" + target.id_suffix;
  }
  aggregate_topic = topic_name + "/info_multi";
  for (int i = 0; i < BTN_COUNT; i++) {
    button_command_topics[i] = generateButtonTopic(button_commands[i]);
  }

  String clientId = String("BatteryEmulatorClient-") + hostname;

  mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
  mqtt_cfg.broker.address.hostname = mqtt_server.c_str();
  mqtt_cfg.broker.address.port = mqtt_port;
  mqtt_cfg.credentials.client_id = clientId.c_str();
  mqtt_cfg.credentials.username = mqtt_user.c_str();
  mqtt_cfg.credentials.authentication.password = mqtt_password.c_str();
  lwt_topic = topic_name + "/status";
  mqtt_cfg.session.last_will.topic = lwt_topic.c_str();
  mqtt_cfg.session.last_will.qos = 1;
  mqtt_cfg.session.last_will.retain = true;
  mqtt_cfg.session.last_will.msg = "offline";
  mqtt_cfg.session.last_will.msg_len = strlen(mqtt_cfg.session.last_will.msg);
  // Broker declares the session dead at ~1.5x keepalive, so this sets how fast the
  // retained "offline" last will reaches subscribers after an unexpected disconnect.
  mqtt_cfg.session.keepalive = 30;
  mqtt_cfg.network.timeout_ms = mqtt_timeout_ms;
  // Bound heap growth of the esp-mqtt outbox when the broker is unreachable.
  // Task stack size is deliberately left at the esp-mqtt default; shrink only after
  // measuring the high-water mark with uxTaskGetStackHighWaterMark() on real hardware.
  mqtt_cfg.outbox.limit = MQTT_OUTBOX_LIMIT_BYTES;
  client = esp_mqtt_client_init(&mqtt_cfg);

  if (client == nullptr) {
    return false;
  }

  if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client) != ESP_OK) {
    return false;
  }

  return true;
}

void mqtt_client_loop(void) {
  // Only attempt to publish/reconnect MQTT if network is connected and checkTimmer is elapsed
  if (check_global_timer.elapsed() && network_connected()) {

    if (client_started == false) {
      // Configure timer with the loaded interval on first use
      publish_global_timer = MyTimer(mqtt_publish_interval_ms);
      esp_mqtt_client_start(client);
      client_started = true;
      logging.println("MQTT client started, connecting to broker...");
      return;
    }

    // Requested by the MQTT_EVENT_CONNECTED handler, published here so that shared_doc and
    // mqtt_msg stay single-threaded. Retried on the next pass if the publish fails.
    if (pending_buttons_discovery && !ota_active && publish_buttons_discovery()) {
      pending_buttons_discovery = false;
    }

    // Skip publishing if OTA update is in progress to avoid interference
    if (publish_global_timer.elapsed() && !ota_active) {
      publish_values();

      // One-shot autodiscovery: as soon as every applicable config is out and retained at
      // the broker, clear the setting so it is not republished on every boot.
      if (ha_autodiscovery_enabled && autodiscovery_complete()) {
        store_autodiscovery_done();
      }
    }
  }
}

bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain) {
  int msg_id = esp_mqtt_client_publish(client, topic, mqtt_msg, strlen(mqtt_msg), MQTT_QOS, retain);
  return msg_id > -1;
}
