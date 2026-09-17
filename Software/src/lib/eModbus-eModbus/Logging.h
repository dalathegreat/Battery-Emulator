// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
//
// Battery-Emulator vendored eModbus is size-trimmed: the server sources had all logging removed by
// hand. The added master (ModbusClient*) and TCP-server (ModbusServerTCPtemp) sources are kept
// byte-identical to upstream so they stay easy to diff/merge; this no-op shim compiles their
// LOG_*/LOGRAW_*/HEXDUMP_* calls out to nothing (zero flash, --gc-sections drops the rest).
// If you ever want real logging, replace this file with the upstream Logging.{h,cpp}.

#ifndef _MODBUS_LOGGING
#define _MODBUS_LOGGING

// Log-level constants (kept so references like LOG_LEVEL_VERBOSE resolve)
#define LOG_LEVEL_NONE (0)
#define LOG_LEVEL_CRITICAL (1)
#define LOG_LEVEL_ERROR (2)
#define LOG_LEVEL_WARNING (3)
#define LOG_LEVEL_INFO (4)
#define LOG_LEVEL_DEBUG (5)
#define LOG_LEVEL_VERBOSE (6)

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_NONE
#endif
#ifndef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL LOG_LEVEL
#endif

// All logging macros expand to nothing.
#define LOG_C(...) do {} while (0)
#define LOG_E(...) do {} while (0)
#define LOG_W(...) do {} while (0)
#define LOG_N(...) do {} while (0)
#define LOG_I(...) do {} while (0)
#define LOG_D(...) do {} while (0)
#define LOG_V(...) do {} while (0)

#define LOGRAW_C(...) do {} while (0)
#define LOGRAW_E(...) do {} while (0)
#define LOGRAW_W(...) do {} while (0)
#define LOGRAW_N(...) do {} while (0)
#define LOGRAW_I(...) do {} while (0)
#define LOGRAW_D(...) do {} while (0)
#define LOGRAW_V(...) do {} while (0)

#define HEXDUMP_C(...) do {} while (0)
#define HEXDUMP_E(...) do {} while (0)
#define HEXDUMP_W(...) do {} while (0)
#define HEXDUMP_N(...) do {} while (0)
#define HEXDUMP_I(...) do {} while (0)
#define HEXDUMP_D(...) do {} while (0)
#define HEXDUMP_V(...) do {} while (0)

#endif  // _MODBUS_LOGGING
