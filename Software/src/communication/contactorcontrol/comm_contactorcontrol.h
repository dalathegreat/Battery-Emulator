#ifndef _COMM_CONTACTORCONTROL_H_
#define _COMM_CONTACTORCONTROL_H_

#include "../../include.h"

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"

/**
 * @brief Handle BMS power output
 *
 * @param[in] void
 *
 * @return void
 */
void handle_BMSpower();

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
