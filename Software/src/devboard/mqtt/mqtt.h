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
#include "../../../USER_SETTINGS.h"

#define MQTT_MSG_BUFFER_SIZE (1024)

extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t temperature_min;    //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t temperature_max;    //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
extern uint16_t cell_max_voltage;   //mV,   0-4350
extern uint16_t cell_min_voltage;   //mV,   0-4350
extern uint16_t cellvoltages[120];  //mV    0-4350 per cell
extern uint8_t nof_cellvoltages;    // Total number of cell voltages, set by each battery. 

extern const char* mqtt_user;
extern const char* mqtt_password;

extern char mqtt_msg[MQTT_MSG_BUFFER_SIZE];

void init_mqtt(void);
void mqtt_loop(void);
bool mqtt_publish_retain(const char* topic);

#endif
