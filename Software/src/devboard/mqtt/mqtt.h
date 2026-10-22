/**
 * MQTT add-on for the battery emulator
 * 
 * Usage:
 * 
 * Subscription - Add topics to MQTT_SUBSCRIPTIONS in USER_SETTINGS.h and handle the messages in mqtt.cpp:callback()
 * 
 * Publishing - See example in mqtt.cpp:publish_values() for constructing the payload
 * 
 * Home assistant - See below for an example, and the official documentation is quite good (https://www.home-assistant.io/integrations/sensor.mqtt/)
 * in configuration.yaml:
 * mqtt: !include mqtt.yaml
 * 
 * in mqtt.yaml:
 * sensor:
 *   - name: "Cell max"
 *       state_topic: "battery/info"
 *       unit_of_measurement: "mV"
 *       value_template: "{{ value_json.cell_max_voltage | int }}"
 *   - name: "Cell min"
 *       state_topic: "battery/info"
 *       unit_of_measurement: "mV"
 *       value_template: "{{ value_json.cell_min_voltage | int }}"
 *   - name: "Temperature max"
 *       state_topic: "battery/info"
 *       unit_of_measurement: "C"
 *       value_template: "{{ value_json.temperature_max | float }}"
 *   - name: "Temperature min"
 *       state_topic: "battery/info"
 *       unit_of_measurement: "C"
 *       value_template: "{{ value_json.temperature_min | float }}"
 */

#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <string>
#include <vector>

#define MQTT_MSG_BUFFER_SIZE (4096)

// Accepted range of the "MQTT timeout ms" setting (esp-mqtt network timeout). Below 2 s a busy
// but healthy link can miss the deadline on a write or the CONNACK and drop the connection.
// Above 20 s a stalled write, which can take about twice the timeout, can trip the 60 s MQTT
// task watchdog and reboot the emulator.
#define MQTT_TIMEOUT_MIN_MS 2000
#define MQTT_TIMEOUT_MAX_MS 20000

// Caps a stored or submitted timeout to the accepted range.
static inline uint16_t mqtt_clamp_timeout_ms(int64_t ms) {
  if (ms < MQTT_TIMEOUT_MIN_MS) {
    return MQTT_TIMEOUT_MIN_MS;
  }
  if (ms > MQTT_TIMEOUT_MAX_MS) {
    return MQTT_TIMEOUT_MAX_MS;
  }
  return (uint16_t)ms;
}

extern const char* version_number;  // The current software version, used for mqtt

extern bool mqtt_enabled;
extern bool mqtt_transmit_all_cellvoltages;
extern bool mqtt_publish_heap_metrics;
extern uint16_t mqtt_timeout_ms;
extern uint16_t mqtt_publish_interval_ms;
extern bool ha_autodiscovery_enabled;
extern std::string ha_autodiscovery_topic;
extern std::string mqtt_server;
extern std::string mqtt_user;
extern std::string mqtt_password;
extern int mqtt_port;
extern const char* mqtt_topic_name;
extern const char* mqtt_object_id_prefix;
extern const char* mqtt_device_name;
extern const char* ha_device_id;

extern char mqtt_msg[MQTT_MSG_BUFFER_SIZE];

uint32_t mqtt_firmware_signature(void);
bool init_mqtt(void);
void mqtt_client_loop(void);
bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain);

#endif
