#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include "../../../USER_SETTINGS.h"
#include "../../battery/BATTERIES.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
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
  static JsonDocument doc;
  static const char* hostname = WiFi.getHostname();
  if (nof_cellvoltages == 0u) {
    return;
  }

  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;
    String topic = "homeassistant/sensor/battery-emulator/cell_voltage";

    for (int i = 0; i < nof_cellvoltages; i++) {
      char cellNumber[4];                  // Buffer to hold the formatted cell number
      sprintf(cellNumber, "%03d", i + 1);  // Format the cell number with leading zeros

      doc["name"] = "Battery Cell Voltage " + String(cellNumber);
      doc["object_id"] = "battery_voltage_cell" + String(cellNumber);
      doc["unique_id"] = "battery-emulator_battery_voltage_cell" + String(cellNumber);
      doc["device_class"] = "voltage";
      doc["state_class"] = "measurement";
      doc["state_topic"] = "battery-emulator/spec_data";
      doc["unit_of_measurement"] = "V";
      doc["enabled_by_default"] = true;
      doc["expire_after"] = 240;
      doc["value_template"] = "{{ value_json.cell_voltages[" + String(i) + "] }}";
      doc["device"]["identifiers"][0] = "battery-emulator";
      doc["device"]["manufacturer"] = "DalaTech";
      doc["device"]["model"] = "BatteryEmulator";
      doc["device"]["name"] = "BatteryEmulator_" + String(hostname);
      doc["origin"]["name"] = "BatteryEmulator";
      doc["origin"]["sw"] = String(version_number) + "-mqtt";
      doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";

      serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));
      String cell_topic = topic + String(i + 1) + "/config";
      mqtt_publish(cell_topic.c_str(), mqtt_msg, true);
    }
    doc.clear();  // clear after sending autoconfig
  } else {
    if (cellvoltages[0] == 0u) {
      return;
    }

    JsonArray cell_voltages = doc["cell_voltages"].to<JsonArray>();
    for (size_t i = 0; i < nof_cellvoltages; ++i) {
      cell_voltages.add(cellvoltages[i] / 1000.0);
    }

    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));

    if (!mqtt_publish("battery-emulator/spec_data", mqtt_msg, false)) {
      Serial.println("Cell voltage MQTT msg could not be sent");
    }
    doc.clear();
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
  static JsonDocument doc;
  static bool mqtt_first_transmission = true;
  static char* state_topic = "battery-emulator/info";
  static const char* hostname = WiFi.getHostname();
  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;
    for (int i = 0; i < sizeof(sensorConfigs) / sizeof(sensorConfigs[0]); i++) {
      SensorConfig& config = sensorConfigs[i];
      doc["name"] = config.name;
      doc["state_topic"] = state_topic;
      doc["unique_id"] = "battery-emulator_" + String(hostname) + "_" + String(config.object_id);
      doc["object_id"] = String(hostname) + "_" + String(config.object_id);
      doc["value_template"] = config.value_template;
      doc["unit_of_measurement"] = config.unit;
      doc["device_class"] = config.device_class;
      doc["enabled_by_default"] = true;
      doc["state_class"] = "measurement";
      doc["expire_after"] = 240;
      doc["device"]["identifiers"][0] = "battery-emulator";
      doc["device"]["manufacturer"] = "DalaTech";
      doc["device"]["model"] = "BatteryEmulator";
      doc["device"]["name"] = "BatteryEmulator_" + String(hostname);
      doc["origin"]["name"] = "BatteryEmulator";
      doc["origin"]["sw"] = String(version_number) + "-mqtt";
      doc["origin"]["url"] = "https://github.com/dalathegreat/Battery-Emulator";
      serializeJson(doc, mqtt_msg);
      mqtt_publish(config.topic, mqtt_msg, true);
    }
    doc.clear();
  } else {
    doc["SOC"] = ((float)SOC) / 100.0;
    doc["state_of_health"] = ((float)StateOfHealth) / 100.0;
    doc["temperature_min"] = ((float)((int16_t)temperature_min)) / 10.0;
    doc["temperature_max"] = ((float)((int16_t)temperature_max)) / 10.0;
    doc["stat_batt_power"] = ((float)((int16_t)stat_batt_power));
    doc["battery_current"] = ((float)((int16_t)battery_current)) / 10.0;
    doc["cell_max_voltage"] = cell_max_voltage / 1000.0;
    doc["cell_min_voltage"] = cell_min_voltage / 1000.0;
    doc["battery_voltage"] = battery_voltage / 10.0;

    serializeJson(doc, mqtt_msg);
    bool result = mqtt_publish(state_topic, mqtt_msg, false);
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
  char clientId[64];  // Adjust the size as needed
  snprintf(clientId, sizeof(clientId), "LilyGoClient-%s", hostname);
  // Attempt to connect
  if (client.connect(clientId, mqtt_user, mqtt_password)) {
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
      publish_values();
    }
  } else {
    if (millis() - previousMillisUpdateVal >= 5000)  // Every 5s
    {
      previousMillisUpdateVal = millis();
      reconnect();
    }
  }
}

bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain) {
  if (client.connected() == true) {
    return client.publish(topic, mqtt_msg, retain);
  }
  return false;
}
