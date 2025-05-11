#ifndef _PRECHARGE_CONTROL_H_
#define _PRECHARGE_CONTROL_H_

#include "../../include.h"

#include "../../devboard/utils/events.h"

/**
 * @brief Contactor initialization
 *
 * @param[in] void
 *
 * @return void
 */
void init_precharge_control();

/**
 * @brief Handle contactors
 *
 * @param[in] unsigned long currentMillis
 *
 * @return void
 */
void handle_precharge_control(unsigned long currentMillis);

/**
 * @brief Handle contactors of battery 2
 *
 * @param[in] void
 *
 * @return void
 */
void handle_contactors_battery2();

#endif  // _PRECHARGE_CONTROL_H_
