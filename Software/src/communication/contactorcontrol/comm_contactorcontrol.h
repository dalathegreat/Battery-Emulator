#ifndef _COMM_CONTACTORCONTROL_H_
#define _COMM_CONTACTORCONTROL_H_

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"

// Settings that can be changed at run-time
extern bool contactor_control_enabled;
extern bool contactor_control_inverted_logic;
extern bool contactor_control_enabled_double_battery;
extern bool contactor_control_enabled_triple_battery;
extern bool pwm_contactor_control;
extern bool periodic_bms_reset;
// Interval between periodic BMS resets, in hours. Only 24 and 48 are offered in the UI.
extern uint16_t periodic_bms_reset_interval_h;
// Guards for the periodic reset. Low SOC defers the reset until SOC recovers, balancing
// costs it a single period. Neither ever applies to a remote (MQTT) triggered reset.
extern bool periodic_bms_reset_defer_low_soc;
extern bool periodic_bms_reset_skip_balancing;
extern bool remote_bms_reset;
extern uint16_t precharge_time_ms;
extern uint16_t pwm_frequency;
extern uint16_t pwm_hold_duty;

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
 * @return true if contactor init was successful, false otherwise.
 */
bool init_contactors();

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

/**
 * @brief Handle contactors of battery 3
 *
 * @param[in] void
 *
 * @return void
 */
void handle_contactors_battery3();

// True when init_contactors() drives BMS_POWER (i.e. the pin is actively controlled).
bool bms_power_is_active();

// Latch/unlatch reset-hold pins (see Esp32Hal::reset_hold_pins()).
// hold: only pins currently driven by the firmware are latched.
// release: every candidate pin is released unconditionally (clears any stale hold).
void hold_pins_across_reset();
void release_pins_across_reset();

#endif
