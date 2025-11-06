// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _EMODBUS_OPTIONS_H
#define _EMODBUS_OPTIONS_H

/* === ESP32 DEFINITIONS AND MACROS === */
#if defined(ESP32) 
#include <Arduino.h>
#define USE_MUTEX 1
#define HAS_FREERTOS 1
#define HAS_ETHERNET 1
#define IS_LINUX 0
const unsigned int SERVER_TASK_STACK = 4096;
const unsigned int CLIENT_TASK_STACK = 4096;

/* === ESP8266 DEFINITIONS AND MACROS === */
#elif defined(ESP8266)
#include <Arduino.h>
#define USE_MUTEX 0
#define HAS_FREERTOS 0
#define HAS_ETHERNET 0
#define IS_LINUX 0

/* === LINUX DEFINITIONS AND MACROS === */
#elif defined(__linux__)
#define USE_MUTEX 1
#define HAS_FREERTOS 0
#define HAS_ETHERNET 0
#define IS_LINUX 1
#include <cstdio>  // for printf()
#include <cstring> // for memcpy(), strlen() etc.
#include <cinttypes> // for uint32_t etc.
#if IS_RASPBERRY
#include <wiringPi.h>
#else
#include <chrono>  // NOLINT
// Use nanosleep() to avoid problems with pthreads (std::this_thread::sleep_for would interfere!)
#define delay(x)  nanosleep((const struct timespec[]){{x/1000, (x%1000)*1000000L}}, NULL);
typedef std::chrono::steady_clock clk;
#define millis() std::chrono::duration_cast<std::chrono::milliseconds>(clk::now().time_since_epoch()).count()
#define micros() std::chrono::duration_cast<std::chrono::microseconds>(clk::now().time_since_epoch()).count()
#endif

/* === INVALID TARGET === */
#else
#error Define target in options.h
#endif

/* === COMMON MACROS === */
#if USE_MUTEX
#define LOCK_GUARD(x,y) std::lock_guard<std::mutex> x(y);
#else
#define LOCK_GUARD(x,y)
#endif

#endif // _EMODBUS_OPTIONS_H
