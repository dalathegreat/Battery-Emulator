#ifndef INCLUDE_H_
#define INCLUDE_H_

#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include "../USER_SETTINGS.h"
#include "system_settings.h"

#include "devboard/hal/hal.h"
#include "devboard/safety/safety.h"
#include "devboard/utils/logging.h"
#include "devboard/utils/time_meas.h"
#include "devboard/utils/types.h"

#include "battery/BATTERIES.h"
#include "charger/CHARGERS.h"
#include "inverter/INVERTERS.h"

/* - ERROR CHECKS BELOW, DON'T TOUCH - */

#if !defined(HW_CONFIGURED)
#error You must select a target hardware in the USER_SETTINGS.h file!
#endif

#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
#if !defined(CANFD_ADDON)
// Check that user did not try to use classic CAN over FD, without FD component
#error PLEASE ENABLE CANFD_ADDON TO USE CLASSIC CAN OVER CANFD INTERFACE
#endif
#endif

#ifdef HW_LILYGO
#if defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)
#if defined(CAN_ADDON) || defined(CANFD_ADDON) || defined(CHADEMO_BATTERY)
//Check that BMS reset is not used at the same time as Chademo and CAN addons
#error BMS RESET CANNOT BE USED AT SAME TIME AS CAN-ADDONS / CHADMEO! NOT ENOUGH GPIO!
#endif
#endif
#endif

#if defined(LOG_CAN_TO_SD) || defined(LOG_TO_SD)
#if !defined(HW_LILYGO)
#error The SD card logging feature is only available on LilyGo hardware
#endif
#endif

#endif
