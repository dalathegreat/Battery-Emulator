#ifndef _COMM_CONTACTORCONTROL_H_
#define _COMM_CONTACTORCONTROL_H_

#include "../../include.h"

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"

// Settings that can be changed at run-time
extern bool contactor_control_enabled;
extern bool contactor_control_enabled_double_battery;
extern bool pwm_contactor_control;
extern bool periodic_bms_reset;
extern bool remote_bms_reset;

/**
 * @brief Handle BMS power output
 *
 * @param[in] void
 *
 * @return void
 */
void handle_BMSpower();

/**
 * @brief Start BMS reset sequence
 *
 * @param[in] void
 *
 * @return void
 */
void start_bms_reset();

/**
 * @brief Contactor initialization
 *
 * @param[in] void
 *
 * @return void
 */
void init_contactors();

/**
 * @brief Handle contactors
 *
 * @param[in] void
 *
 * @return void
 */
void handle_contactors();

/**
 * @brief Handle contactors of battery 2
 *
 * @param[in] void
 *
 * @return void
 */
void handle_contactors_battery2();

#endif
