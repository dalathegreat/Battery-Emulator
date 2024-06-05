#ifndef INCLUDE_H_
#define INCLUDE_H_

#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include "../USER_SETTINGS.h"
#include "system_settings.h"

#include "devboard/hal/hal.h"
#include "devboard/safety/safety.h"
#include "devboard/utils/time_meas.h"
#include "devboard/utils/types.h"

#include "battery/BATTERIES.h"
#include "charger/CHARGERS.h"
#include "inverter/INVERTERS.h"

/* - ERROR CHECKS BELOW, DON'T TOUCH - */

#if !defined(HW_CONFIGURED)
#error You must select a HW to run on!
#endif

#if defined(DUAL_CAN) && defined(CAN_FD)
// Check that user did not try to use dual can and fd-can on same hardware pins
#error CAN-FD AND DUAL-CAN CANNOT BE USED SIMULTANEOUSLY
#endif

#ifdef MODBUS_INVERTER_SELECTED
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
// Check that Dual LilyGo via RS485 option isn't enabled, this collides with Modbus!
#error MODBUS CANNOT BE USED IN DOUBLE LILYGO SETUPS! CHECK USER SETTINGS!
#endif
#endif

#ifndef BATTERY_SELECTED
#error No battery selected! Choose one from the USER_SETTINGS.h file
#endif

#ifdef KIA_E_GMP_BATTERY
#ifndef CAN_FD
#error KIA HYUNDAI EGMP BATTERIES CANNOT BE USED WITHOUT CAN FD
#endif
#endif

#endif
