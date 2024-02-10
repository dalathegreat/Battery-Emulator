#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include "../../../USER_SETTINGS.h"
#include "../../battery/BATTERIES.h"
#include "../../lib/knolleary-pubsubclient/PubSubClient.h"
#include "../utils/timer.h"

const char* mqtt_subscriptions[] = MQTT_SUBSCRIPTIONS;
const size_t mqtt_nof_subscriptions = sizeof(mqtt_subscriptions) / sizeof(mqtt_subscriptions[0]);

WiFiClient espClient;
PubSubClient client(espClient);
char mqtt_msg[MQTT_MSG_BUFFER_SIZE];
int value = 0;
static unsigned long previousMillisUpdateVal;
MyTimer publish_global_timer(5000);

static void publish_common_info(void);
static void publish_cell_voltages(void);

/** Publish global values and call callbacks for specific modules */
static void publish_values(void) {
  publish_common_info();
  publish_cell_voltages();
}

static void publish_cell_voltages(void) {
  static bool mqtt_first_transmission = true;

  // If the cell voltage number isn't initialized...
  if (nof_cellvoltages == 0u) {
    return;
  }
  // At startup, re-post the discovery message for home assistant
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;

    // Base topic for any cell voltage "sensor"
    String topic = "homeassistant/sensor/battery-emulator/cell_voltage";
    for (int i = 0; i < nof_cellvoltages; i++) {
      // Build JSON message with device configuration for each cell voltage
      // Probably shouldn't be BatteryEmulator here, instead "LeafBattery"
      // or similar but hey, it works.
      // mqtt_msg is a global buffer, should be fine since we run too much
      // in a single thread :)
      snprintf(mqtt_msg, sizeof(mqtt_msg),
               "{"
               "\"device\": {"
               "\"identifiers\": ["
               "\"battery-emulator\""
               "],"
               "\"manufacturer\": \"DalaTech\","
               "\"model\": \"BatteryEmulator\","
               "\"name\": \"BatteryEmulator\""
               "},"
               "\"device_class\": \"voltage\","
               "\"enabled_by_default\": true,"
               "\"object_id\": \"sensor_battery_voltage_cell%d\","
               "\"origin\": {"
               "\"name\": \"BatteryEmulator\","
               "\"sw\": \"%s-mqtt\","
               "\"url\": \"https://github.com/dalathegreat/Battery-Emulator\""
               "},"
               "\"state_class\": \"measurement\","
               "\"name\": \"Battery Cell Voltage %d\","
               "\"state_topic\": \"battery-emulator/spec_data\","
               "\"unique_id\": \"battery-emulator_battery_voltage_cell%d\","
               "\"unit_of_measurement\": \"V\","
               "\"value_template\": \"{{ value_json.cell_voltages[%d] }}\""
               "}",
               i + 1, version_number, i + 1, i + 1, i);
      // End each discovery topic with cell number and '/config'
      String cell_topic = topic + String(i + 1) + "/config";
      mqtt_publish_retain(cell_topic.c_str());
    }
  } else {
    // Every 5-ish seconds, build the JSON payload for the state topic. This requires
    // some annoying formatting due to C++ not having nice Python-like string formatting.
    // msg_length is a cumulative variable to track start position (param 1) and for
    // modifying the maxiumum amount of characters to write (param 2). The third parameter
    // is the string content

    // If cell voltages haven't been populated...
    if (cellvoltages[0] == 0u / 1000) {  //cell voltage is in mV and homeassistant expects V
      return;
    }

    size_t msg_length = snprintf(mqtt_msg, sizeof(mqtt_msg), "{\n\"cell_voltages\":[");
    for (size_t i = 0; i < nof_cellvoltages; ++i) {
      msg_length +=
          snprintf(mqtt_msg + msg_length, sizeof(mqtt_msg) - msg_length, "%s%d", (i == 0) ? "" : ", ", cellvoltages[i]);
    }
    snprintf(mqtt_msg + msg_length, sizeof(mqtt_msg) - msg_length, "]\n}\n");

    // Publish and print error if not OK
    if (mqtt_publish_retain("battery-emulator/spec_data") == false) {
      Serial.println("Cell voltage MQTT msg could not be sent");
    }
  }
}

struct SensorConfig {
  const char* object_id;
  const char* topic;
  const char* name;
  const char* value_template;
  const char* unit;
  const char* device_class;
};

SensorConfig sensorConfigs[] = {
    {"SOC", "homeassistant/sensor/battery-emulator/SOC/config", "Battery Emulator SOC", "{{ value_json.SOC }}", "%",
     "battery"},
    {"state_of_health", "homeassistant/sensor/battery-emulator/state_of_health/config",
     "Battery Emulator State Of Health", "{{ value_json.state_of_health }}", "%", "battery"},
    {"temperature_min", "homeassistant/sensor/battery-emulator/temperature_min/config",
     "Battery Emulator Temperature Min", "{{ value_json.temperature_min }}", "°C", "temperature"},
    {"temperature_max", "homeassistant/sensor/battery-emulator/temperature_max/config",
     "Battery Emulator Temperature Max", "{{ value_json.temperature_max }}", "°C", "temperature"},
    {"stat_batt_power", "homeassistant/sensor/battery-emulator/stat_batt_power/config",
     "Battery Emulator Stat Batt Power", "{{ value_json.stat_batt_power }}", "W", "power"},
    {"battery_current", "homeassistant/sensor/battery-emulator/battery_current/config",
     "Battery Emulator Battery Current", "{{ value_json.battery_current }}", "A", "current"},
    {"cell_max_voltage", "homeassistant/sensor/battery-emulator/cell_max_voltage/config",
     "Battery Emulator Cell Max Voltage", "{{ value_json.cell_max_voltage }}", "V", "voltage"},
    {"cell_min_voltage", "homeassistant/sensor/battery-emulator/cell_min_voltage/config",
     "Battery Emulator Cell Min Voltage", "{{ value_json.cell_min_voltage }}", "V", "voltage"},
    {"battery_voltage", "homeassistant/sensor/battery-emulator/battery_voltage/config",
     "Battery Emulator Battery Voltage", "{{ value_json.battery_voltage }}", "V", "voltage"},
};

static void publish_common_info(void) {
  static bool mqtt_first_transmission = true;
  static char* state_topic = "battery-emulator/info";
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;
    for (int i = 0; i < sizeof(sensorConfigs) / sizeof(sensorConfigs[0]); i++) {
      SensorConfig& config = sensorConfigs[i];
      snprintf(mqtt_msg, sizeof(mqtt_msg),
               "{"
               "\"name\": \"%s\","
               "\"state_topic\": \"%s\","
               "\"unique_id\": \"battery-emulator_%s\","
               "\"object_id\": \"sensor_battery_%s\","
               "\"device\": {"
               "\"identifiers\": ["
               "\"battery-emulator\""
               "],"
               "\"manufacturer\": \"DalaTech\","
               "\"model\": \"BatteryEmulator\","
               "\"name\": \"BatteryEmulator\""
               "},"
               "\"origin\": {"
               "\"name\": \"BatteryEmulator\","
               "\"sw\": \"%s-mqtt\","
               "\"url\": \"https://github.com/dalathegreat/Battery-Emulator\""
               "},"
               "\"value_template\": \"%s\","
               "\"unit_of_measurement\": \"%s\","
               "\"device_class\": \"%s\","
               "\"enabled_by_default\": true,"
               "\"state_class\": \"measurement\""
               "}",
               config.name, state_topic, config.object_id, config.object_id, version_number, config.value_template,
               config.unit, config.device_class);
      mqtt_publish_retain(config.topic);
    }
  } else {
    snprintf(mqtt_msg, sizeof(mqtt_msg),
             "{\n"
             "  \"SOC\": %.3f,\n"
             "  \"state_of_health\": %.3f,\n"
             "  \"temperature_min\": %.3f,\n"
             "  \"temperature_max\": %.3f,\n"
             "  \"stat_batt_power\": %.3f,\n"
             "  \"battery_current\": %.3f,\n"
             "  \"cell_max_voltage\": %d,\n"
             "  \"cell_min_voltage\": %d,\n"
             "  \"battery_voltage\": %d\n"
             "}\n",
             ((float)SOC) / 100.0, ((float)StateOfHealth) / 100.0, ((float)((int16_t)temperature_min)) / 10.0,
             ((float)((int16_t)temperature_max)) / 10.0, ((float)((int16_t)stat_batt_power)),
             ((float)((int16_t)battery_current)) / 10.0, cell_max_voltage / 1000, cell_min_voltage / 1000,
             battery_voltage / 10.0);
    bool result = client.publish(state_topic, mqtt_msg, true);
  }

  //Serial.println(mqtt_msg);  // Uncomment to print the payload on serial
}

/* This is called whenever a subscribed topic changes (hopefully) */
static void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/* If we lose the connection, get it back and re-sub */
static void reconnect() {
  // attempt one reconnection
  Serial.print("Attempting MQTT connection... ");
  const char* hostname = WiFi.getHostname();
  String clientId = "LilyGoClient-" + String(hostname);
  // Attempt to connect
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
    Serial.println("connected");

    for (int i = 0; i < mqtt_nof_subscriptions; i++) {
      client.subscribe(mqtt_subscriptions[i]);
      Serial.print("Subscribed to: ");
      Serial.println(mqtt_subscriptions[i]);
    }
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
  }
}

void init_mqtt(void) {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  Serial.println("MQTT initialized");

  previousMillisUpdateVal = millis();
  reconnect();
}

void mqtt_loop(void) {
  if (client.connected()) {
    client.loop();
    if (publish_global_timer.elapsed() == true)  // Every 5s
    {
      publish_values();  // Update values heading towards inverter. Prepare for sending on CAN, or write directly to Modbus.
    }
  } else {
    if (millis() - previousMillisUpdateVal >= 5000)  // Every 5s
    {
      previousMillisUpdateVal = millis();
      reconnect();  // Update values heading towards inverter. Prepare for sending on CAN, or write directly to Modbus.
    }
  }
}

bool mqtt_publish_retain(const char* topic) {
  if (client.connected() == true) {
    return client.publish(topic, mqtt_msg, true);
  }
  return false;
}
