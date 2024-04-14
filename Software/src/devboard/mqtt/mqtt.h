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
#include "../../include.h"

#define MQTT_MSG_BUFFER_SIZE (1024)

extern const char* version_number;  // The current software version, used for mqtt

extern uint16_t system_scaled_SOC_pptt;                    //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;                      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000 , Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;                     //Total number of cell voltages, set by each battery

extern const char* mqtt_user;
extern const char* mqtt_password;

extern char mqtt_msg[MQTT_MSG_BUFFER_SIZE];

void init_mqtt(void);
void mqtt_loop(void);
bool mqtt_publish(const char* topic, const char* mqtt_msg, bool retain);

#endif
