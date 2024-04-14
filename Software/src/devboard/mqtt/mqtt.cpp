#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include "../../../USER_SETTINGS.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
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

static String generateCellVoltageAutoConfigTopic(int cell_number, const char* hostname) {
  return String("homeassistant/sensor/battery-emulator_") + String(hostname) + "/cell_voltage" + String(cell_number) +
         "/config";
}

static void publish_cell_voltages(void) {
  static bool mqtt_first_transmission = true;
  static JsonDocument doc;
  static const char* hostname = WiFi.getHostname();
  static String state_topic = String("battery-emulator_") + String(hostname) + "/spec_data";

  // If the cell voltage number isn't initialized...
  if (system_number_of_cells == 0u) {
    return;
  }

  if (mqtt_first_transmission == true) {
    mqtt_first_transmission = false;
    String topic = "homeassistant/sensor/battery-emulator/cell_voltage";

    for (int i = 0; i < system_number_of_cells; i++) {
      int cellNumber = i + 1;
      doc["name"] = "Battery Cell Voltage " + String(cellNumber);
      doc["object_id"] = "battery_voltage_cell" + String(cellNumber);
      doc["unique_id"] = "battery-emulator_" + String(hostname) + "_battery_voltage_cell" +
                         String(cellNumber);  //"battery-emulator_" + String(hostname) + "_" +
      doc["device_class"] = "voltage";
      doc["state_class"] = "measurement";
      doc["state_topic"] = state_topic;
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
      mqtt_publish(generateCellVoltageAutoConfigTopic(cellNumber, hostname).c_str(), mqtt_msg, true);
    }
    doc.clear();  // clear after sending autoconfig
  } else {
    // If cell voltages haven't been populated...
    if (system_number_of_cells == 0u) {
      return;
    }

    JsonArray cell_voltages = doc["cell_voltages"].to<JsonArray>();
    for (size_t i = 0; i < system_number_of_cells; ++i) {
      cell_voltages.add(((float)system_cellvoltages_mV[i]) / 1000.0);
    }

    serializeJson(doc, mqtt_msg, sizeof(mqtt_msg));

    if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_VIA_USB
      Serial.println("Cell voltage MQTT msg could not be sent");
#endif
    }
    doc.clear();
  }
}

struct SensorConfig {
  const char* object_id;
  const char* name;
  const char* value_template;
  const char* unit;
  const char* device_class;
};

SensorConfig sensorConfigs[] = {
    {"SOC", "Battery Emulator SOC (scaled)", "{{ value_json.SOC }}", "%", "battery"},
    {"SOC_real", "Battery Emulator SOC (real)", "{{ value_json.SOC_real }}", "%", "battery"},
    {"state_of_health", "Battery Emulator State Of Health", "{{ value_json.state_of_health }}", "%", "battery"},
    {"temperature_min", "Battery Emulator Temperature Min", "{{ value_json.temperature_min }}", "°C", "temperature"},
    {"temperature_max", "Battery Emulator Temperature Max", "{{ value_json.temperature_max }}", "°C", "temperature"},
    {"stat_batt_power", "Battery Emulator Stat Batt Power", "{{ value_json.stat_batt_power }}", "W", "power"},
    {"battery_current", "Battery Emulator Battery Current", "{{ value_json.battery_current }}", "A", "current"},
    {"cell_max_voltage", "Battery Emulator Cell Max Voltage", "{{ value_json.cell_max_voltage }}", "V", "voltage"},
    {"cell_min_voltage", "Battery Emulator Cell Min Voltage", "{{ value_json.cell_min_voltage }}", "V", "voltage"},
    {"battery_voltage", "Battery Emulator Battery Voltage", "{{ value_json.battery_voltage }}", "V", "voltage"},
};

static String generateCommonInfoAutoConfigTopic(const char* object_id, const char* hostname) {
  return String("homeassistant/sensor/battery-emulator_") + String(hostname) + "/" + String(object_id) + "/config";
}

static void publish_common_info(void) {
  static JsonDocument doc;
  static bool mqtt_first_transmission = true;
  static const char* hostname = WiFi.getHostname();
  static String state_topic = String("battery-emulator_") + String(hostname) + "/info";
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
      mqtt_publish(generateCommonInfoAutoConfigTopic(config.object_id, hostname).c_str(), mqtt_msg, true);
    }
    doc.clear();
  } else {
    doc["SOC"] = ((float)system_scaled_SOC_pptt) / 100.0;
    doc["SOC_real"] = ((float)system_real_SOC_pptt) / 100.0;
    doc["state_of_health"] = ((float)datalayer.battery.status.soh_pptt) / 100.0;
    doc["temperature_min"] = ((float)((int16_t)datalayer.battery.status.temperature_min_dC)) / 10.0;
    doc["temperature_max"] = ((float)((int16_t)datalayer.battery.status.temperature_max_dC)) / 10.0;
    doc["stat_batt_power"] = ((float)((int32_t)datalayer.battery.status.active_power_W));
    doc["battery_current"] = ((float)((int16_t)datalayer.battery.status.current_dA)) / 10.0;
    doc["cell_max_voltage"] = ((float)system_cell_max_voltage_mV) / 1000.0;
    doc["cell_min_voltage"] = ((float)system_cell_min_voltage_mV) / 1000.0;
    doc["battery_voltage"] = ((float)datalayer.battery.status.voltage_dV) / 10.0;

    serializeJson(doc, mqtt_msg);
    if (!mqtt_publish(state_topic.c_str(), mqtt_msg, false)) {
#ifdef DEBUG_VIA_USB
      Serial.println("Common info MQTT msg could not be sent");
#endif
    }
    doc.clear();
  }
}

/* This is called whenever a subscribed topic changes (hopefully) */
static void callback(char* topic, byte* payload, unsigned int length) {
#ifdef DEBUG_VIA_USB
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
#endif
}

/* If we lose the connection, get it back and re-sub */
static void reconnect() {
// attempt one reconnection
#ifdef DEBUG_VIA_USB
  Serial.print("Attempting MQTT connection... ");
#endif
  const char* hostname = WiFi.getHostname();
  char clientId[64];  // Adjust the size as needed
  snprintf(clientId, sizeof(clientId), "LilyGoClient-%s", hostname);
  // Attempt to connect
  if (client.connect(clientId, mqtt_user, mqtt_password)) {
#ifdef DEBUG_VIA_USB
    Serial.println("connected");
#endif

    for (int i = 0; i < mqtt_nof_subscriptions; i++) {
      client.subscribe(mqtt_subscriptions[i]);
#ifdef DEBUG_VIA_USB
      Serial.print("Subscribed to: ");
      Serial.println(mqtt_subscriptions[i]);
#endif
    }
  } else {
#ifdef DEBUG_VIA_USB
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
#endif
    // Wait 5 seconds before retrying
  }
}

void init_mqtt(void) {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
#ifdef DEBUG_VIA_USB
  Serial.println("MQTT initialized");
#endif

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
