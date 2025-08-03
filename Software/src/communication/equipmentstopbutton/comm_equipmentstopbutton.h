#ifndef _COMM_EQUIPMENTSTOPBUTTON_H_
#define _COMM_EQUIPMENTSTOPBUTTON_H_

/**
 * @brief Initialization of equipment stop button
 *
 * @param[in] void
 *
 * @return void
 */
bool init_equipment_stop_button();

/**
 * @brief Monitor equipment stop button
 *
 * @param[in] void
 *
 * @return void
 */
void monitor_equipment_stop_button();

enum class STOP_BUTTON_BEHAVIOR { NOT_CONNECTED = 0, LATCHING_SWITCH = 1, MOMENTARY_SWITCH = 2, Highest };

extern STOP_BUTTON_BEHAVIOR equipment_stop_behavior;

#endif
