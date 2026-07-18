#ifndef _PRECHARGE_CONTROL_H_
#define _PRECHARGE_CONTROL_H_

#include <stdint.h>

#include "../../devboard/utils/events.h"

// TODO: Ensure valid values at run-time
// User can update all these values via Settings page
enum class PrechargeControlMode : uint8_t {
  Disabled = 0,
  Hia4v1Pwm = 1,
  I2cG05 = 2,
};

extern PrechargeControlMode precharge_control_mode;

bool is_precharge_control_enabled();
bool is_precharge_control_i2c_g05_enabled();
extern bool precharge_inverter_normally_open_contactor;
extern uint16_t precharge_max_precharge_time_before_fault;
extern uint16_t Precharge_max_PWM_Freq;
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
