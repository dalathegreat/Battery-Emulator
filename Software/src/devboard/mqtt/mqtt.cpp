#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include "../../../USER_SECRETS.h"
#include "../../../USER_SETTINGS.h"
#include "../../battery/BATTERIES.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../datalayer/datalayer.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../utils/events.h"
#include "../utils/timer.h"
#include "mqtt_client.h"

esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t client;
char mqtt_msg[MQTT_MSG_BUFFER_SIZE];
MyTimer publish_global_timer(5000);  //publish timer
MyTimer check_global_timer(800);     // check timmer - low-priority MQTT checks, where responsiveness is not critical.

static String topic_name = "";
static String object_id_prefix = "";
static String device_name = "";
static String device_id = "";

static void publish_common_info(void);
static void publish_cell_voltages(void);
static void publish_events(void);

/** Publish global values and call callbacks for specific modules */
static void publish_values(void) {
  publish_events();
  publish_common_info();
  publish_cell_voltages();
}

#ifdef HA_AUTODISCOVERY
struct SensorConfig {
  const char* object_id;
  const char* name;
  const char* value_template;
  const char* unit;
  const char* device_class;
};

SensorConfig sensorConfigs[] = {
    {"SOC", "SOC (scaled)", "{{ value_json.SOC }}", "%", "battery"},
    {"SOC_real", "SOC (real)", "{{ value_json.SOC_real }}", "%", "battery"},
    {"state_of_health", "State Of Health", "{{ value_json.state_of_health }}", "%", "battery"},
    {"temperature_min", "Temperature Min", "{{ value_json.temperature_min }}", "°C", "temperature"},
    {"temperature_max", "Temperature Max", "{{ value_json.temperature_max }}", "°C", "temperature"},
    {"stat_batt_power", "Stat Batt Power", "{{ value_json.stat_batt_power }}", "W", "power"},
    {"battery_current", "Battery Current", "{{ value_json.battery_current }}", "A", "current"},
    {"cell_max_voltage", "Cell Max Voltage", "{{ value_json.cell_max_voltage }}", "V", "voltage"},
    {"cell_min_voltage", "Cell Min Voltage", "{{ value_json.cell_min_voltage }}", "V", "voltage"},
    {"battery_voltage", "Battery Voltage", "{{ value_json.battery_voltage }}", "V", "voltage"},
    {"total_capacity", "Battery Total Capacity", "{{ value_json.total_capacity }}", "Wh", "energy"},
    {"remaining_capacity", "Battery Remaining Capacity (scaled)", "{{ value_json.remaining_capacity }}", "Wh",
     "energy"},
    {"remaining_capacity_real", "Battery Remaining Capacity (real)", "{{ value_json.remaining_capacity_real }}", "Wh",
     "energy"},
    {"max_discharge_power", "Battery Max Discharge Power", "{{ value_json.max_discharge_power }}", "W", "power"},
    {"max_charge_power", "Battery Max Charge Power", "{{ value_json.max_charge_power }}", "W", "power"},
    {"bms_status", "BMS Status", "{{ value_json.bms_status }}", "", ""},
    {"pause_status", "Pause Status", "{{ value_json.pause_status }}", "", ""},
#ifdef DOUBLE_BATTERY
    {"SOC_2", "SOC 2 (scaled)", "{{ value_json.SOC_2 }}", "%", "battery"},
    {"SOC_real_2", "SOC 2 (real)", "{{ value_json.SOC_real_2 }}", "%", "battery"},
    {"state_of_health_2", "State Of Health 2", "{{ value_json.state_of_health_2 }}", "%", "battery"},
    {"temperature_min_2", "Temperature Min 2", "{{ value_json.temperature_min_2 }}", "°C", "temperature"},
    {"temperature_max_2", "Temperature Max 2", "{{ value_json.temperature_max_2 }}", "°C", "temperature"},
    {"stat_batt_power_2", "Stat Batt Power 2", "{{ value_json.stat_batt_power_2 }}", "W", "power"},
    {"battery_current_2", "Battery 2 Current", "{{ value_json.battery_current_2 }}", "A", "current"},
    {"cell_max_voltage_2", "Cell Max Voltage 2", "{{ value_json.cell_max_voltage_2 }}", "V", "voltage"},
    {"cell_min_voltage_2", "Cell Min Voltage 2", "{{ value_json.cell_min_voltage_2 }}", "V", "voltage"},
    {"battery_voltage_2", "Battery 2 Voltage", "{{ value_json.battery_voltage_2 }}", "V", "voltage"},
    {"total_capacity_2", "Battery 2 Total Capacity", "{{ value_json.total_capacity_2 }}", "Wh", "energy"},
    {"remaining_capacity_2", "Battery 2 Remaining Capacity (scaled)", "{{ value_json.remaining_capacity_2 }}", "Wh",
     "energy"},
    {"remaining_capacity_real_2", "Battery 2 Remaining Capacity (real)", "{{ value_json.remaining_capacity_real_2 }}",
     "Wh", "energy"},
    {"max_discharge_power_2", "Battery 2 Max Discharge Power", "{{ value_json.max_discharge_power_2 }}", "W", "power"},
    {"max_charge_power_2", "Battery 2 Max Charge Power", "{{ value_json.max_charge_power_2 }}", "W", "power"},
    {"bms_status_2", "BMS 2 Status", "{{ value_json.bms_status_2 }}", "", ""},
    {"pause_status_2", "Pause Status 2", "{{ value_json.pause_status_2 }}", "", ""},
#endif  // DOUBLE_BATTERY
};

SensorConfig buttonConfigs[] = {{"BMSRESET", "Reset BMS", "", "", ""}};

static String generateCommonInfoAutoConfigTopic(const char* object_id) {
  return "homeassistant/sensor/" + topic_name + "/" + String(object_id) + "/config";
}

static String generateCellVoltageAutoConfigTopic(int cell_number, String battery_suffix) {
  return "homeassistant/sensor/" + topic_name + "/cell_voltage" + battery_suffix + String(cell_number) + "/config";
}

static String generateEventsAutoConfigTopic(const char* object_id) {
  return "homeassistant/sensor/" + topic_name + "/" + String(object_id) + "/config";
}

static String generateButtonTopic(const char* subtype) {
  return "homeassistant/button/" + topic_name + "/" + String(subtype);
}

static String generateButtonAutoConfigTopic(const char* subtype) {
  return generateButtonTopic(subtype) + "/config";
}

#endif  // HA_AUTODISCOVERY

static std::vector<EventData> order_events;

static void publish_common_info(void) {
  static JsonDocument doc;
#ifdef HA_AUTODISCOVERY
  static bool mqtt_first_transmission = true;
#endif  // HA_AUTODISCOVERY
  static String state_topic = topic_name + "/info";
#ifdef HA_AUTODISCOVERY
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;
    for (int i = 0; i < sizeof(sensorConfigs) / sizeof(sensorConfigs[0]); i++) {
      SensorConfig& config = sensorConfigs[i];
      doc["name"] = config.name;
      doc["state_topic"] = state_topic;
      doc["unique_id"] = topic_name + "_" + String(config.object_id);
      doc["object_id"] = object_id_prefix + String(config.object_id);
      doc["value_template"] = config.value_template;
      if (config.unit != nullptr && strlen(config.unit) > 0)
        doc["unit_of_measurement"] = config.unit;
      if (config.device_class != nullptr && strlen(config.device_class) > 0) {
        doc["device_class"] = config.device_class;
        doc["state_class"] = "measurement";
      }
      doc["enabled_by_default"] = true;
      doc["expire_after"] = 240;
      doc["device"]["identifiers"][0] = ha_device_id;
      doc["device"]["manufacturer"] = "DalaTech";
      doc["device"]["model"] = "BatteryEmulator";
      doc["device"]["name"] = device_name;
      doc["origin"]["name"] = "BatteryEmulator";
      doc["origin"]["sw"] = String(version_number) + "-mqtt";
      doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";
      serializeJson(doc, mqtt_msg);
      mqtt_publish(generateCommonInfoAutoConfigTopic(config.object_id).c_str(), mqtt_msg, true);
      doc.clear();
    }

  } else {
#endif  // HA_AUTODISCOVERY
    doc["bms_status"] = getBMSStatus(datalayer.battery.status.bms_status);
    doc["pause_status"] = get_emulator_pause_status();

    //only publish these values if BMS is active and we are comunication  with the battery (can send CAN messages to the battery)
    if (datalayer.battery.status.CAN_battery_still_alive && allowed_to_send_CAN && millis() > BOOTUP_TIME) {
      doc["SOC"] = ((float)datalayer.battery.status.reported_soc) / 100.0;
      doc["SOC_real"] = ((float)datalayer.battery.status.real_soc) / 100.0;
      doc["state_of_health"] = ((float)datalayer.battery.status.soh_pptt) / 100.0;
      doc["temperature_min"] = ((float)((int16_t)datalayer.battery.status.temperature_min_dC)) / 10.0;
      doc["temperature_max"] = ((float)((int16_t)datalayer.battery.status.temperature_max_dC)) / 10.0;
      doc["stat_batt_power"] = ((float)((int32_t)datalayer.battery.status.active_power_W));
      doc["battery_current"] = ((float)((int16_t)datalayer.battery.status.current_dA)) / 10.0;
      doc["battery_voltage"] = ((float)datalayer.battery.status.voltage_dV) / 10.0;
      // publish only if cell voltages have been populated...
      if (datalayer.battery.info.number_of_cells != 0u &&
          datalayer.battery.status.cell_voltages_mV[datalayer.battery.info.number_of_cells - 1] != 0u) {
        doc["cell_max_voltage"] = ((float)datalayer.battery.status.cell_max_voltage_mV) / 1000.0;
        doc["cell_min_voltage"] = ((float)datalayer.battery.status.cell_min_voltage_mV) / 1000.0;
      }
      doc["total_capacity"] = ((float)datalayer.battery.info.total_capacity_Wh);
      doc["remaining_capacity_real"] = ((float)datalayer.battery.status.remaining_capacity_Wh);
      doc["remaining_capacity"] = ((float)datalayer.battery.status.reported_remaining_capacity_Wh);
      doc["max_discharge_power"] = ((float)datalayer.battery.status.max_discharge_power_W);
      doc["max_charge_power"] = ((float)datalayer.battery.status.max_charge_power_W);
    }
#ifdef DOUBLE_BATTERY
    //only publish these values if BMS is active and we are comunication  with the battery (can send CAN messages to the battery)
    if (datalayer.battery2.status.CAN_battery_still_alive && allowed_to_send_CAN && millis() > BOOTUP_TIME) {
      doc["SOC_2"] = ((float)datalayer.battery2.status.reported_soc) / 100.0;
      doc["SOC_real_2"] = ((float)datalayer.battery2.status.real_soc) / 100.0;
      doc["state_of_health_2"] = ((float)datalayer.battery2.status.soh_pptt) / 100.0;
      doc["temperature_min_2"] = ((float)((int16_t)datalayer.battery2.status.temperature_min_dC)) / 10.0;
      doc["temperature_max_2"] = ((float)((int16_t)datalayer.battery2.status.temperature_max_dC)) / 10.0;
      doc["stat_batt_power_2"] = ((float)((int32_t)datalayer.battery2.status.active_power_W));
      doc["battery_current_2"] = ((float)((int16_t)datalayer.battery2.status.current_dA)) / 10.0;
      doc["battery_voltage_2"] = ((float)datalayer.battery2.status.voltage_dV) / 10.0;
      // publish only if cell voltages have been populated...
      if (datalayer.battery2.info.number_of_cells != 0u &&
          datalayer.battery2.status.cell_voltages_mV[datalayer.battery2.info.number_of_cells - 1] != 0u) {
        doc["cell_max_voltage_2"] = ((float)datalayer.battery2.status.cell_max_voltage_mV) / 1000.0;
        doc["cell_min_voltage_2"] = ((float)datalayer.battery2.status.cell_min_voltage_mV) / 1000.0;
      }
      doc["total_capacity_2"] = ((float)datalayer.battery2.info.total_capacity_Wh);
      doc["remaining_capacity_real_2"] = ((float)datalayer.battery2.status.remaining_capacity_Wh);
      doc["remaining_capacity_2"] = ((float)datalayer.battery2.status.reported_remaining_capacity_Wh);
      doc["max_discharge_power_2"] = ((float)datalayer.battery2.status.max_discharge_power_W);
      doc["max_charge_power_2"] = ((float)datalayer.battery2.status.max_charge_power_W);
    }
#endif  // DOUBLE_BATTERY
    serializeJson(doc, mqtt_msg);
    if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_LOG
      logging.println("Common info MQTT msg could not be sent");
#endif  // DEBUG_LOG
    }
    doc.clear();
#ifdef HA_AUTODISCOVERY
  }
#endif  // HA_AUTODISCOVERY
}

static void publish_cell_voltages(void) {
#ifdef HA_AUTODISCOVERY
  static bool mqtt_first_transmission = true;
#endif  // HA_AUTODISCOVERY
  static JsonDocument doc;
  static String state_topic = topic_name + "/spec_data";
#ifdef DOUBLE_BATTERY
  static String state_topic_2 = topic_name + "/spec_data_2";

#endif  // DOUBLE_BATTERY

#ifdef HA_AUTODISCOVERY
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;

    // If the cell voltage number isn't initialized...
    if (datalayer.battery.info.number_of_cells != 0u) {

      for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
        int cellNumber = i + 1;
        doc["name"] = "Battery Cell Voltage " + String(cellNumber);
        doc["object_id"] = object_id_prefix + "battery_voltage_cell" + String(cellNumber);
        doc["unique_id"] = topic_name + "_battery_voltage_cell" + String(cellNumber);
        doc["device_class"] = "voltage";
        doc["state_class"] = "measurement";
        doc["state_topic"] = state_topic;
        doc["unit_of_measurement"] = "V";
        doc["enabled_by_default"] = true;
        doc["expire_after"] = 240;
        doc["value_template"] = "{{ value_json.cell_voltages[" + String(i) + "] }}";
        doc["device"]["identifiers"][0] = ha_device_id;
        doc["device"]["manufacturer"] = "DalaTech";
        doc["device"]["model"] = "BatteryEmulator";
        doc["device"]["name"] = device_name;
        doc["origin"]["name"] = "BatteryEmulator";
        doc["origin"]["sw"] = String(version_number) + "-mqtt";
        doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";

        serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
        mqtt_publish(generateCellVoltageAutoConfigTopic(cellNumber, "").c_str(), mqtt_msg, true);
      }
      doc.clear();  // clear after sending autoconfig
    }
#ifdef DOUBLE_BATTERY
    // If the cell voltage number isn't initialized...
    if (datalayer.battery2.info.number_of_cells != 0u) {

      for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
        int cellNumber = i + 1;
        doc["name"] = "Battery 2 Cell Voltage " + String(cellNumber);
        doc["object_id"] = object_id_prefix + "battery_2_voltage_cell" + String(cellNumber);
        doc["unique_id"] = topic_name + "_battery_2_voltage_cell" + String(cellNumber);
        doc["device_class"] = "voltage";
        doc["state_class"] = "measurement";
        doc["state_topic"] = state_topic_2;
        doc["unit_of_measurement"] = "V";
        doc["enabled_by_default"] = true;
        doc["expire_after"] = 240;
        doc["value_template"] = "{{ value_json.cell_voltages[" + String(i) + "] }}";
        doc["device"]["identifiers"][0] = ha_device_id;
        doc["device"]["manufacturer"] = "DalaTech";
        doc["device"]["model"] = "BatteryEmulator";
        doc["device"]["name"] = device_name;
        doc["origin"]["name"] = "BatteryEmulator";
        doc["origin"]["sw"] = String(version_number) + "-mqtt";
        doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";

        serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
        mqtt_publish(generateCellVoltageAutoConfigTopic(cellNumber, "_2_").c_str(), mqtt_msg, true);
      }
      doc.clear();  // clear after sending autoconfig
    }
#endif  // DOUBLE_BATTERY
  }
#endif  // HA_AUTODISCOVERY

  // If cell voltages have been populated...
  if (datalayer.battery.info.number_of_cells != 0u &&
      datalayer.battery.status.cell_voltages_mV[datalayer.battery.info.number_of_cells - 1] != 0u) {

    JsonArray cell_voltages = doc["cell_voltages"].to<JsonArray>();
    for (size_t i = 0; i < datalayer.battery.info.number_of_cells; ++i) {
      cell_voltages.add(((float)datalayer.battery.status.cell_voltages_mV[i]) / 1000.0);
    }

    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));

    if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_LOG
      logging.println("Cell voltage MQTT msg could not be sent");
#endif  // DEBUG_LOG
    }
    doc.clear();
  }

#ifdef DOUBLE_BATTERY
  // If cell voltages have been populated...
  if (datalayer.battery2.info.number_of_cells != 0u &&
      datalayer.battery2.status.cell_voltages_mV[datalayer.battery2.info.number_of_cells - 1] != 0u) {

    JsonArray cell_voltages = doc["cell_voltages"].to<JsonArray>();
    for (size_t i = 0; i < datalayer.battery2.info.number_of_cells; ++i) {
      cell_voltages.add(((float)datalayer.battery2.status.cell_voltages_mV[i]) / 1000.0);
    }

    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));

    if (!mqtt_publish(state_topic_2.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_LOG
      logging.println("Cell voltage MQTT msg could not be sent");
#endif  // DEBUG_LOG
    }
    doc.clear();
  }
#endif  // DOUBLE_BATTERY
}

void publish_events() {

  static JsonDocument doc;
#ifdef HA_AUTODISCOVERY
  static bool mqtt_first_transmission = true;
#endif  // HA_AUTODISCOVERY
  static String state_topic = topic_name + "/events";
#ifdef HA_AUTODISCOVERY
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;

    doc["name"] = "Event";
    doc["state_topic"] = state_topic;
    doc["unique_id"] = topic_name + "_event";
    doc["object_id"] = object_id_prefix + "event";
    doc["value_template"] =
        "{{ value_json.event_type ~ ' (c:' ~ value_json.count ~ ',m:' ~  value_json.millis ~ ') ' ~ value_json.message "
        "}}";
    doc["json_attributes_topic"] = state_topic;
    doc["json_attributes_template"] = "{{ value_json | tojson }}";
    doc["enabled_by_default"] = true;
    doc["device"]["identifiers"][0] = ha_device_id;
    doc["device"]["manufacturer"] = "DalaTech";
    doc["device"]["model"] = "BatteryEmulator";
    doc["device"]["name"] = device_name;
    doc["origin"]["name"] = "BatteryEmulator";
    doc["origin"]["sw"] = String(version_number) + "-mqtt";
    doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";
    serializeJson(doc, mqtt_msg);
    mqtt_publish(generateEventsAutoConfigTopic("event").c_str(), mqtt_msg, true);

    doc.clear();
  } else {
#endif  // HA_AUTODISCOVERY

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

      doc["event_type"] = String(get_event_enum_string(event_handle));
      doc["severity"] = String(get_event_level_string(event_handle));
      doc["count"] = String(event_pointer->occurences);
      doc["data"] = String(event_pointer->data);
      doc["message"] = String(get_event_message_string(event_handle));
      doc["millis"] = String(event_pointer->timestamp);

      serializeJson(doc, mqtt_msg);
      if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_LOG
        logging.println("Common info MQTT msg could not be sent");
#endif  // DEBUG_LOG
      } else {
        set_event_MQTTpublished(event_handle);
      }
      doc.clear();
      //clear the vector
      order_events.clear();
    }
#ifdef HA_AUTODISCOVERY
  }
#endif  // HA_AUTODISCOVERY
}

static void publish_buttons_discovery(void) {
#ifdef HA_AUTODISCOVERY
  static bool mqtt_first_transmission = true;
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;

#ifdef DEBUG_LOG
    logging.println("Publishing buttons discovery");
#endif  // DEBUG_LOG

    static JsonDocument doc;
    for (int i = 0; i < sizeof(buttonConfigs) / sizeof(buttonConfigs[0]); i++) {
      SensorConfig& config = buttonConfigs[i];
      doc["name"] = config.name;
      doc["unique_id"] = config.object_id;
      doc["command_topic"] = generateButtonTopic(config.object_id);
      doc["enabled_by_default"] = true;
      doc["expire_after"] = 240;
      doc["device"]["identifiers"][0] = ha_device_id;
      doc["device"]["manufacturer"] = "DalaTech";
      doc["device"]["model"] = "BatteryEmulator";
      doc["device"]["name"] = device_name;
      doc["origin"]["name"] = "BatteryEmulator";
      doc["origin"]["sw"] = String(version_number) + "-mqtt";
      doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";
      serializeJson(doc, mqtt_msg);
      mqtt_publish(generateButtonAutoConfigTopic(config.object_id).c_str(), mqtt_msg, true);
      doc.clear();
    }
  }
#endif  // HA_AUTODISCOVERY
}

static void subscribe() {
  for (int i = 0; i < sizeof(buttonConfigs) / sizeof(buttonConfigs[0]); i++) {
    SensorConfig& config = buttonConfigs[i];
    const char* topic = generateButtonTopic(config.object_id).c_str();
#ifdef DEBUG_LOG
    logging.printf("Subscribing to topic: [%s]\n", topic);
#endif  // DEBUG_LOG
    esp_mqtt_client_subscribe(client, topic, 0);
  }
}

void mqtt_message_received(char* topic, int topic_len, char* data, int data_len) {
#ifdef DEBUG_LOG
  logging.printf("MQTT message arrived: [%.*s]\n", topic_len, topic);
#endif  // DEBUG_LOG

#ifdef REMOTE_BMS_RESET
  const char* bmsreset_topic = generateButtonTopic("BMSRESET").c_str();
  if (strncmp(topic, bmsreset_topic, topic_len) == 0) {
#ifdef DEBUG_LOG
    logging.println("Triggering BMS reset");
#endif  // DEBUG_LOG
    start_bms_reset();
  }
#endif  // REMOTE_BMS_RESET
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      clear_event(EVENT_MQTT_DISCONNECT);
      set_event(EVENT_MQTT_CONNECT, 0);

      publish_buttons_discovery();
      subscribe();
#ifdef DEBUG_LOG
      logging.println("MQTT connected");
#endif  // DEBUG_LOG
      break;
    case MQTT_EVENT_DISCONNECTED:
      set_event(EVENT_MQTT_DISCONNECT, 0);
#ifdef DEBUG_LOG
      logging.println("MQTT disconnected!");
#endif  // DEBUG_LOG
      break;
    case MQTT_EVENT_DATA:
      mqtt_message_received(event->topic, event->topic_len, event->data, event->data_len);
      break;
  }
}

void init_mqtt(void) {

#ifdef MQTT
#ifdef MQTT_MANUAL_TOPIC_OBJECT_NAME
  // Use custom topic name, object ID prefix, and device name from user settings
  topic_name = mqtt_topic_name;
  object_id_prefix = mqtt_object_id_prefix;
  device_name = mqtt_device_name;
  device_id = ha_device_id;
#else
  // Use default naming based on WiFi hostname for topic, object ID prefix, and device name
  topic_name = "battery-emulator_" + String(WiFi.getHostname());
  object_id_prefix = String(WiFi.getHostname()) + String("_");
  device_name = "BatteryEmulator_" + String(WiFi.getHostname());
  device_id = "battery-emulator";
#endif
#endif

  char clientId[64];  // Adjust the size as needed
  snprintf(clientId, sizeof(clientId), "BatteryEmulatorClient-%s", WiFi.getHostname());
  mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
  mqtt_cfg.broker.address.hostname = MQTT_SERVER;
  mqtt_cfg.broker.address.port = MQTT_PORT;
  mqtt_cfg.credentials.client_id = clientId;
  mqtt_cfg.credentials.username = MQTT_USER;
  mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client);
  esp_mqtt_client_start(client);

#ifdef DEBUG_LOG
  logging.println("MQTT initialized");
#endif  // DEBUG_LOG
}

void mqtt_loop(void) {
  // Only attempt to publish/reconnect MQTT if Wi-Fi is connectedand checkTimmer is elapsed
  if (check_global_timer.elapsed() && WiFi.status() == WL_CONNECTED) {
    if (publish_global_timer.elapsed())  // Every 5s
    {
      publish_values();
    }
  }
}

bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain) {
  int msg_id = esp_mqtt_client_publish(client, topic, mqtt_msg, strlen(mqtt_msg), 0, retain);
  return msg_id > -1;
}
