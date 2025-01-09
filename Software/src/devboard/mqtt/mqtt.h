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
#include <vector>
#include "../../include.h"

#define MQTT_MSG_BUFFER_SIZE (1024)

extern const char* version_number;  // The current software version, used for mqtt

extern const char* mqtt_user;
extern const char* mqtt_password;
extern const char* mqtt_topic_name;
extern const char* mqtt_object_id_prefix;
extern const char* mqtt_device_name;
extern const char* ha_device_id;

extern char mqtt_msg[MQTT_MSG_BUFFER_SIZE];

void init_mqtt(void);
void mqtt_loop(void);
bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain);

#endif
