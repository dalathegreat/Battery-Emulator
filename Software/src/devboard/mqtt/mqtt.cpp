#include "mqtt.h"
#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include "../../../USER_SETTINGS.h"
#include "../../lib/knolleary-pubsubclient/PubSubClient.h"
#include "../utils/timer.h"

const char* mqtt_subscriptions[] = MQTT_SUBSCRIPTIONS;
const size_t mqtt_nof_subscriptions = sizeof(mqtt_subscriptions) / sizeof(mqtt_subscriptions[0]);

WiFiClient espClient;
PubSubClient client(espClient);
char msg[MSG_BUFFER_SIZE];
int value = 0;
static unsigned long previousMillisUpdateVal;
MyTimer publish_global_timer(5000);

/** Publish global values and call callbacks for specific modules */
static void publish_values(void) {

  snprintf(msg, sizeof(msg),
           "{\n"
           "  \"SOC\": %.3f,\n"
           "  \"StateOfHealth\": %.3f,\n"
           "  \"temperature_min\": %.3f,\n"
           "  \"temperature_max\": %.3f,\n"
           "  \"cell_max_voltage\": %d,\n"
           "  \"cell_min_voltage\": %d\n"
           "}\n",
           ((float)SOC) / 100.0, ((float)StateOfHealth) / 100.0, ((float)((int16_t)temperature_min)) / 10.0,
           ((float)((int16_t)temperature_max)) / 10.0, cell_max_voltage, cell_min_voltage);
  bool result = client.publish("battery_testing/info", msg, true);
  Serial.println(msg);  // Uncomment to print the payload on serial
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
  // Create a random client ID
  String clientId = "LilyGoClient-";
  clientId += String(random(0xffff), HEX);
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

bool mqtt_publish(const String& topic, const String& payload) {
  return client.publish(topic.c_str(), payload.c_str());
}