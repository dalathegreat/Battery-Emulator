#ifndef _PRECHARGE_CONTROL_H_
#define _PRECHARGE_CONTROL_H_

#include "../../devboard/utils/events.h"

// TODO: Ensure valid values at run-time
// User can update all these values via Settings page
extern bool precharge_control_enabled;
extern bool precharge_inverter_normally_open_contactor;
extern uint16_t precharge_max_precharge_time_before_fault;
/**
 * @brief Contactor initialization
 *
 * @param[in] void
 *
 * @return void
 */
bool init_precharge_control();

/**
 * @brief Handle contactors
 *
 * @param[in] unsigned long currentMillis
 *
 * @return void
 */
void handle_precharge_control(unsigned long currentMillis);

#endif  // _PRECHARGE_CONTROL_H_
