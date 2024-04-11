#ifndef SYSTEM_SETTINGS_H_
#define SYSTEM_SETTINGS_H_
/** TASKS
 * 
 * Parameter: TASK_CORE_PRIO
 * Description:
 * Defines the priority of core functionality (CAN, Modbus, etc)
 * 
 * Parameter: TASK_CONNECTIVITY_PRIO
 * Description:
 * Defines the priority of various wireless functionality (TCP, MQTT, etc)
*/
#define TASK_CORE_PRIO 4
#define TASK_CONNECTIVITY_PRIO 3

/** MAX AMOUNT OF CELLS
 * 
 * Parameter: MAX_AMOUNT_CELLS
 * Description:
 * Basically the length of the array used to hold individual cell voltages
*/
#define MAX_AMOUNT_CELLS 192

/** LED 
 * 
 * Parameter: LED_MODE_DEFAULT
 * Description:
 * The default LED mode. Available modes:
 * CLASSIC   - slow up/down ramp
 * FLOW      - slow ramp up or down depending on flow of energy
 * HEARTBEAT - Heartbeat-like LED pattern that reacts to the system state with color and BPM
 * 
 * Parameter: LED_PERIOD_MS
 * Description:
 * The period of whatever LED mode is active. If CLASSIC, then a ramp up and ramp down will finish in
 * LED_PERIOD_MS milliseconds
*/
#define LED_MODE_DEFAULT FLOW
#define LED_PERIOD_MS 3000
#define LED_EXECUTION_FREQUENCY 50

#endif
