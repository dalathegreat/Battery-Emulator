#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <src/communication/nvm/comm_nvm.h>
#include "../../battery/BATTERIES.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/safety/safety.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
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
}

static bool ha_common_info_published = false;
static bool ha_cell_voltages_published = false;
static bool ha_events_published = false;
static bool ha_buttons_published = false;

// One JsonDocument shared by all publish functions. They are only ever called sequentially
// from publish_values() / the MQTT event handler, never concurrently, so sharing is safe
// and caps the retained ArduinoJson pool to the single largest payload instead of one pool
// per publish function.
static JsonDocument shared_doc;

// RAII guard: clears the shared document on scope entry and exit, so every early return
// (e.g. a failed publish mid-loop) releases the document memory instead of keeping a full
// payload allocated until the next successful cycle.
struct DocClearGuard {
  JsonDocument& doc;
  explicit DocClearGuard(JsonDocument& d) : doc(d) { doc.clear(); }
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
static bool supports_charged(Battery* b) {
  return b->supports_charged_energy();
}
static bool supports_tesla_dcdc_metrics(Battery* b) {
  return b != nullptr && (user_selected_battery_type == BatteryType::TeslaModel3Y ||
                          user_selected_battery_type == BatteryType::TeslaModelSX);
}
static bool supports_byd_autocal_metrics(Battery* b) {
  return b != nullptr && user_selected_battery_type == BatteryType::BydAtto3;
}

static const SensorConfig batterySensorConfigTemplate[] = {
    {"SOC", "SOC (Scaled)", "%", "battery", always},
    {"SOC_real", "SOC (real)", "%", "battery", always},
    {"state_of_health", "State of Health", "%", "battery", always},
    {"temperature_min", "Temperature Min", "°C", "temperature", always},
    {"temperature_max", "Temperature Max", "°C", "temperature", always},
    {"stat_batt_power", "Battery Power", "W", "power", always},
    {"battery_current", "Battery Current", "A", "current", always},
    {"cell_max_voltage", "Cell Max Voltage", "V", "voltage", always},
    {"cell_min_voltage", "Cell Min Voltage", "V", "voltage", always},
    {"cell_voltage_delta", "Cell Voltage Delta", "mV", "voltage", always},
    {"battery_voltage", "Battery Voltage", "V", "voltage", always},
    {"total_capacity", "Total Capacity", "Wh", "energy", always},
    {"remaining_capacity", "Remaining Capacity (scaled)", "Wh", "energy", always},
    {"remaining_capacity_real", "Remaining Capacity (real)", "Wh", "energy", always},
    {"max_discharge_power", "Max Discharge Power", "W", "power", always},
    {"max_charge_power", "Max Charge Power", "W", "power", always},
    {"charged_energy", "Battery Charged Energy", "Wh", "energy", supports_charged},
    {"discharged_energy", "Battery Discharged Energy", "Wh", "energy", supports_charged},
    {"balancing_active_cells", "Balancing Active Cells", "", "", always},
    {"balancing_status", "Balancing Status", "", "", always},
    {"charging_state", "Charging State", "", "", always},
    {"limiting_factor", "Limiting Factor", "", "", always},
    {"dc_dc_current", "DC-DC Current", "A", "current", supports_tesla_dcdc_metrics},
    {"dc_dc_voltage", "DC-DC Voltage", "V", "voltage", supports_tesla_dcdc_metrics},
    {"autocal_taper", "BYD Auto-cal: In Taper", "", "", supports_byd_autocal_metrics},
    {"autocal_dwell_s", "BYD Auto-cal: Dwell Time", "s", "duration", supports_byd_autocal_metrics},
    {"autocal_cooldown_ready", "BYD Auto-cal: Cooldown Ready", "", "", supports_byd_autocal_metrics},
    {"autocal_soc_drift", "BYD Auto-cal: SOC Drift", "%", "battery", supports_byd_autocal_metrics}};

static const SensorConfig globalSensorConfigTemplate[] = {
    {"bms_status", "BMS Status", "", "", always},
    {"pause_status", "Pause Status", "", "", always},
    {"event_level", "Event Level", "", "", always},
    {"emulator_status", "Emulator Status", "", "", always},
    {"emulator_uptime", "Emulator Uptime", "s", "duration", always},
    {"cpu_temp", "CPU Temperature", "°C", "temperature", always}};

// The battery instances the MQTT module publishes for. Battery #1 keeps the historical
// un-suffixed topic ("<name>/info") and entity ids, so single-battery setups see no change.
struct BatteryTarget {
  Battery** bat;                       // global battery instance pointer (may point to nullptr)
  const DATALAYER_BATTERY_TYPE* data;  // matching datalayer entry
  int index;                           // 1-based battery number
  const char* id_suffix;               // suffix for entity ids / unique ids ("", "_2", "_3")
  const char* name_suffix;             // suffix for display names ("", " 2", " 3")
};

static const BatteryTarget battery_targets[] = {
    {&battery, &datalayer.battery, 1, "", ""},
    {&battery2, &datalayer.battery2, 2, "_2", " 2"},
    {&battery3, &datalayer.battery3, 3, "_3", " 3"},
};

// Per-battery state topics: "<name>/info", "<name>/info_2", "<name>/info_3".
// Built once in init_mqtt(). Publishing each battery to its own topic (with identical,
// un-suffixed JSON keys) keeps the JSON document and payload size constant per battery
// instead of growing with the battery count, and lets ArduinoJson store all keys as
// zero-copy const char* literals.
static String info_topics[3];

static const SensorConfig buttonConfigs[] = {{"BMSRESET", "Reset BMS", nullptr, nullptr, nullptr},
                                             {"PAUSE", "Pause charge/discharge", nullptr, nullptr, nullptr},
                                             {"RESUME", "Resume charge/discharge", nullptr, nullptr, nullptr},
                                             {"RESTART", "Restart Battery Emulator", nullptr, nullptr, nullptr},
                                             {"STOP", "Open Contactors", nullptr, nullptr, nullptr}};

// All commands the emulator subscribes to. The matching topics are precomputed once in
// init_mqtt() so that mqtt_message_received() does not rebuild six temporary Strings on
// every received message.
enum ButtonCommand { BTN_BMSRESET = 0, BTN_PAUSE, BTN_RESUME, BTN_RESTART, BTN_STOP, BTN_SET_LIMITS, BTN_COUNT };
static const char* button_commands[BTN_COUNT] = {"BMSRESET", "PAUSE", "RESUME", "RESTART", "STOP", "SET_LIMITS"};
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

void set_common_discovery_attributes(JsonDocument& doc) {
  doc["device"]["identifiers"][0] = device_id;
  doc["device"]["model"] = "Battery Emulator";
  doc["device"]["manufacturer"] = "FOSS";
  doc["device"]["name"] = device_name;
  doc["device"]["configuration_url"] = "http://" + WiFi.localIP().toString();
  doc["availability"][0]["topic"] = lwt_topic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["enabled_by_default"] = true;
}

void set_battery_voltage_attributes(JsonDocument& doc, int i, int cellNumber, const String& state_topic,
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
      return "Blocked";
    default:
      return "Unknown";
  }
}

// Fills the document with the state values for one battery. All keys are un-suffixed
// const char* literals: ArduinoJson stores those by pointer (zero copy), whereas the old
// "key" + suffix String keys were each heap-allocated and then copied into the document
// pool — on every publish cycle, for every battery.
void set_battery_attributes(JsonDocument& doc, const DATALAYER_BATTERY_TYPE& battery_data, int battery_index,
                            bool battery_supports_charged) {
  doc["SOC"] = ((float)battery_data.status.reported_soc) / 100.0f;
  doc["SOC_real"] = ((float)battery_data.status.real_soc) / 100.0f;
  doc["state_of_health"] = ((float)battery_data.status.soh_pptt) / 100.0f;
  doc["temperature_min"] = ((float)((int16_t)battery_data.status.temperature_min_dC)) / 10.0f;
  doc["temperature_max"] = ((float)((int16_t)battery_data.status.temperature_max_dC)) / 10.0f;
  doc["stat_batt_power"] = ((float)((int32_t)battery_data.status.active_power_W));
  doc["battery_current"] = ((float)((int16_t)battery_data.status.current_dA)) / 10.0f;
  doc["battery_voltage"] = ((float)battery_data.status.voltage_dV) / 10.0f;
  if (battery_data.info.number_of_cells != 0u &&
      battery_data.status.cell_voltages_mV[battery_data.info.number_of_cells - 1] != 0u) {
    doc["cell_max_voltage"] = ((float)battery_data.status.cell_max_voltage_mV) / 1000.0f;
    doc["cell_min_voltage"] = ((float)battery_data.status.cell_min_voltage_mV) / 1000.0f;
    doc["cell_voltage_delta"] =
        ((float)battery_data.status.cell_max_voltage_mV) - ((float)battery_data.status.cell_min_voltage_mV);
  }
  doc["total_capacity"] = ((float)battery_data.info.total_capacity_Wh);
  doc["remaining_capacity_real"] = ((float)battery_data.status.remaining_capacity_Wh);
  doc["remaining_capacity"] = ((float)battery_data.status.reported_remaining_capacity_Wh);
  doc["max_discharge_power"] = ((float)battery_data.status.max_discharge_power_W);
  doc["max_charge_power"] = ((float)battery_data.status.max_charge_power_W);

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
  ChargingState charging_state = get_charging_state(battery_data.status.current_dA);
  doc["charging_state"] = charging_state_to_text(charging_state);
  doc["limiting_factor"] = limiting_factor_to_text(get_limiting_factor(
      charging_state, battery_data.settings.inverter_limits_charge, battery_data.settings.inverter_limits_discharge,
      battery_data.settings.user_settings_limit_charge, battery_data.settings.user_settings_limit_discharge));
  if (battery_index == 1 && supports_tesla_dcdc_metrics(::battery)) {
    doc["dc_dc_current"] = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent) * 0.1f;
    doc["dc_dc_voltage"] = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 0.0390625f;
  }
  if (supports_byd_autocal_metrics(::battery)) {
    const DATALAYER_INFO_BYDATTO3& byd =
        (battery_index == 2) ? datalayer_extended.bydAtto3_2 : datalayer_extended.bydAtto3;
    doc["autocal_taper"] = byd.autocal_crit_taper;
    doc["autocal_dwell_s"] = byd.autocal_dwell_accumulated_ms / 1000u;
    doc["autocal_cooldown_ready"] = byd.autocal_crit_cooldown_ready;
    doc["autocal_soc_drift"] = byd.autocal_drift_percent;
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
static bool publish_sensor_discovery(const SensorConfig& config, const char* id_suffix, const char* name_suffix,
                                     const String& state_topic) {
  char entity_id[64];
  char name_buf[64];
  char value_template[96];
  snprintf(entity_id, sizeof(entity_id), "%s%s", config.entity_id, id_suffix);
  snprintf(name_buf, sizeof(name_buf), "%s%s", config.name, name_suffix);
  // The state topics are per-battery, so the value_template key is the base id for every
  // battery — no "_2"/"_3" key variants exist anymore.
  snprintf(value_template, sizeof(value_template), "{{ value_json.%s }}", config.entity_id);

  JsonDocument& doc = shared_doc;
  doc["name"] = name_buf;
  doc["state_topic"] = state_topic;
  doc["unique_id"] = topic_name + "_" + String(entity_id);
  const String default_entity_object_id = default_entity_id_prefix + String(entity_id);
  doc["default_entity_id"] = generateSensorDefaultEntityId(default_entity_object_id);
  doc["value_template"] = value_template;
  if (config.unit != nullptr && strlen(config.unit) > 0) {
    doc["unit_of_measurement"] = config.unit;
  }
  if (config.device_class != nullptr && strlen(config.device_class) > 0) {
    doc["device_class"] = config.device_class;
    doc["state_class"] = "measurement";
  }
  // "balancing_active_cells" is a numeric count with no device_class, so it misses the
  // state_class assignment above. Mark it as a measurement explicitly so Home Assistant
  // records long-term statistics for it.
  if (strcmp(config.entity_id, "balancing_active_cells") == 0) {
    doc["state_class"] = "measurement";
  }
  // "energy" device_class is only valid with state_class total / total_increasing, never
  // "measurement" — HA rejects the combination. The capacity sensors represent a current
  // stored amount, so use "energy_storage" (compatible with "measurement") instead. The
  // charged/discharged sensors are genuine lifetime totals, so keep "energy" but use
  // "total_increasing".
  if (strncmp(config.entity_id, "total_capacity", strlen("total_capacity")) == 0 ||
      strncmp(config.entity_id, "remaining_capacity", strlen("remaining_capacity")) == 0) {
    doc["device_class"] = "energy_storage";
  } else if (strcmp(config.entity_id, "charged_energy") == 0 || strcmp(config.entity_id, "discharged_energy") == 0) {
    doc["state_class"] = "total_increasing";
  }
  // Cell min/max voltages: show 3 decimals in HA so they don't round to the same integer
  // on display. Precision is intentionally not applied to battery_voltage. (Icons are
  // handled centrally below.)
  if (strcmp(config.entity_id, "cell_max_voltage") == 0 || strcmp(config.entity_id, "cell_min_voltage") == 0) {
    doc["suggested_display_precision"] = 3;
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
  set_common_discovery_attributes(doc);
  serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
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
        if (!publish_sensor_discovery(config, target.id_suffix, target.name_suffix, info_topics[target.index - 1])) {
          return false;
        }
      }
    }
    // Global (emulator-level) sensors stay on battery #1's "/info" topic.
    for (const auto& config : globalSensorConfigTemplate) {
      if (!publish_sensor_discovery(config, "", "", info_topics[0])) {
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
      JsonDocument& doc = shared_doc;
      doc["bms_status"] = getBMSStatus(datalayer.system.status.system_status);
      doc["pause_status"] = get_emulator_pause_status();

      //only publish these values if BMS is active and we are comunication with the battery (can send CAN messages to the battery)
      if (datalayer.battery.status.CAN_battery_still_alive && allowed_to_send_CAN && esp32hal->system_booted_up()) {
        set_battery_attributes(doc, datalayer.battery, 1, battery->supports_charged_energy());
      }

      doc["event_level"] = get_event_level_string(get_event_level());
      doc["emulator_status"] = get_emulator_status_string(get_emulator_status());
      doc["cpu_temp"] = datalayer.system.info.CPU_temperature;
      doc["emulator_uptime"] = millis64() / 1000;

      serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
      if (mqtt_publish(info_topics[0].c_str(), mqtt_msg, false) == false) {
        logging.println("Common info MQTT msg could not be sent");
        return false;
      }
    }

    // Batteries #2 and #3 on "/info_2" and "/info_3".
    for (const auto& target : battery_targets) {
      Battery* bat = *target.bat;
      if (target.index == 1 || bat == nullptr) {
        continue;
      }
      //only publish these values if BMS is active and we are comunication with the battery (can send CAN messages to the battery)
      if (target.data->status.CAN_battery_still_alive && allowed_to_send_CAN && esp32hal->system_booted_up()) {
        DocClearGuard guard(shared_doc);
        set_battery_attributes(shared_doc, *target.data, target.index, bat->supports_charged_energy());
        serializeJson(shared_doc, mqtt_msg, sizeof(mqtt_msg));
        if (mqtt_publish(info_topics[target.index - 1].c_str(), mqtt_msg, false) == false) {
          logging.println("Common info MQTT msg could not be sent");
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
// with snprintf straight into mqtt_msg avoids building an ArduinoJson document for the
// biggest transient allocation of every publish cycle. Discovery (one-shot, deeply
// structured) keeps using ArduinoJson.

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
      len += snprintf(mqtt_msg + len, sizeof(mqtt_msg) - len, "%s%.3f", (i != 0u) ? "," : "",
                      ((float)battery_data.status.cell_voltages_mV[i]) / 1000.0f);
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
    logging.println("Cell data MQTT msg could not be sent");
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

  JsonDocument& doc = shared_doc;
  for (int i = 0; i < battery_data.info.number_of_cells; i++) {
    int cellNumber = i + 1;
    set_battery_voltage_attributes(doc, i, cellNumber, state_topic, entity_prefix, battery_name_suffix);
    set_common_discovery_attributes(doc);

    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
    if (mqtt_publish(generateCellVoltageAutoConfigTopic(cellNumber, topic_suffix).c_str(), mqtt_msg, true) == false) {
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

    if (!publish_cell_voltage_discovery(datalayer.battery, state_topic, default_entity_id_prefix, "", "", all_ready)) {
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
    JsonDocument& doc = shared_doc;

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
    set_common_discovery_attributes(doc);
    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
    if (mqtt_publish(generateEventsAutoConfigTopic("event").c_str(), mqtt_msg, true)) {
      ha_events_published = true;
    } else {
      return false;
    }
  } else {
    DocClearGuard guard(shared_doc);
    JsonDocument& doc = shared_doc;
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

      serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
      if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
        logging.println("Common info MQTT msg could not be sent");
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
      logging.println("Publishing buttons discovery");

      DocClearGuard guard(shared_doc);
      JsonDocument& doc = shared_doc;
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
        set_common_discovery_attributes(doc);
        serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
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

  if (strcmp(topic, button_command_topics[BTN_SET_LIMITS].c_str()) == 0) {
    JsonDocument doc;
    char* data_str = strndup(data, data_len);
    deserializeJson(doc, data_str);

    if (doc["max_charge"].is<int>()) {
      datalayer.battery.settings.max_remote_set_charge_dA = doc["max_charge"];
      datalayer.battery.settings.remote_settings_limit_charge = true;
    } else {
      datalayer.battery.settings.max_remote_set_charge_dA = 0;
      datalayer.battery.settings.remote_settings_limit_charge = false;
    }

    if (doc["max_discharge"].is<int>()) {
      datalayer.battery.settings.max_remote_set_discharge_dA = doc["max_discharge"];
      datalayer.battery.settings.remote_settings_limit_discharge = true;
    } else {
      datalayer.battery.settings.max_remote_set_discharge_dA = 0;
      datalayer.battery.settings.remote_settings_limit_discharge = false;
    }

    if (doc["timeout"].is<int>()) {
      datalayer.battery.settings.remote_set_timeout = doc["timeout"].as<int>() * 1000;
    } else {
      datalayer.battery.settings.remote_set_timeout = 30000;
    }

    datalayer.battery.settings.remote_set_timestamp = millis();

    free(data_str);
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

      publish_buttons_discovery();
      subscribe();
      break;
    case MQTT_EVENT_DISCONNECTED:
      set_event(EVENT_MQTT_DISCONNECT, 0);
      logging.println("MQTT disconnected!");
      break;
    case MQTT_EVENT_DATA:
      mqtt_message_received(event->topic, event->topic_len, event->data, event->data_len);
      break;
    case MQTT_EVENT_ERROR:
      logging.println("MQTT_ERROR");
      logging.print("reported from esp-tls");
      logging.println(event->error_handle->esp_tls_last_esp_err);
      logging.print("reported from tls stack");
      logging.println(event->error_handle->esp_tls_stack_err);
      logging.print("captured as transport's socket errno");
      logging.println(strerror(event->error_handle->esp_transport_sock_errno));
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

  String hostname = String(WiFi.getHostname());
  topic_name = hostname;
  default_entity_id_prefix = hostname + "_";
  device_name = hostname;
  device_id = hostname;

  // Precompute the per-battery state topics and the command topics once, so the publish
  // and receive paths don't rebuild them as temporary Strings on every cycle / message.
  for (const auto& target : battery_targets) {
    info_topics[target.index - 1] = topic_name + "/info" + target.id_suffix;
  }
  for (int i = 0; i < BTN_COUNT; i++) {
    button_command_topics[i] = generateButtonTopic(button_commands[i]);
  }

  String clientId = String("BatteryEmulatorClient-") + WiFi.getHostname();

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
  // Only attempt to publish/reconnect MQTT if Wi-Fi is connected and checkTimmer is elapsed
  if (check_global_timer.elapsed() && WiFi.status() == WL_CONNECTED) {

    if (client_started == false) {
      // Configure timer with the loaded interval on first use
      publish_global_timer = MyTimer(mqtt_publish_interval_ms);
      esp_mqtt_client_start(client);
      client_started = true;
      logging.println("MQTT client started, connecting to broker...");
      return;
    }

    // Skip publishing if OTA update is in progress to avoid interference
    if (publish_global_timer.elapsed() && !ota_active) {
      publish_values();
    }
  }
}

bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain) {
  int msg_id = esp_mqtt_client_publish(client, topic, mqtt_msg, strlen(mqtt_msg), MQTT_QOS, retain);
  return msg_id > -1;
}
