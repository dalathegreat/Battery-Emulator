// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ERROR
#endif

#ifndef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL LOG_LEVEL
#endif

// The following needs to be defined only once
#ifndef _MODBUS_LOGGING
#define _MODBUS_LOGGING
#include "options.h"

#define LOG_LEVEL_NONE (0)
#define LOG_LEVEL_CRITICAL (1)
#define LOG_LEVEL_ERROR (2)
#define LOG_LEVEL_WARNING (3)
#define LOG_LEVEL_INFO (4)
#define LOG_LEVEL_DEBUG (5)
#define LOG_LEVEL_VERBOSE (6)

#define LL_RED "\e[1;31m"
#define LL_GREEN "\e[32m"
#define LL_YELLOW "\e[1;33m"
#define LL_BLUE "\e[34m"
#define LL_MAGENTA "\e[35m"
#define LL_CYAN "\e[36m"
#define LL_NORM "\e[0m"

#define LOG_HEADER(x) "[" #x "] %lu| %-20s [%4d] %s: "

constexpr const char* str_end(const char *str) {
    return *str ? str_end(str + 1) : str;
}

constexpr bool str_slant(const char *str) {
    return ((*str == '/') || (*str == '\\')) ? true : (*str ? str_slant(str + 1) : false);
}

constexpr const char* r_slant(const char* str) {
    return ((*str == '/') || (*str == '\\')) ? (str + 1) : r_slant(str - 1);
}
constexpr const char* file_name(const char* str) {
    return str_slant(str) ? r_slant(str_end(str)) : str;
}

#if IS_LINUX
void logHexDump(const char *letter, const char *label, const uint8_t *data, const size_t length);
#else
extern Print *LOGDEVICE;
void logHexDump(Print *output, const char *letter, const char *label, const uint8_t *data, const size_t length);
#endif
extern int MBUlogLvl;
#endif  // _MODBUS_LOGGING

// The remainder may need to be redefined if LOCAL_LOG_LEVEL was set differently before
#ifdef LOG_LINE_T
#undef LOG_LINE_C
#undef LOG_LINE_E
#undef LOG_LINE_T
#undef LOG_RAW_C
#undef LOG_RAW_E
#undef LOG_RAW_T
#undef HEX_DUMP_T
#undef LOG_N
#undef LOG_C
#undef LOG_E
#undef LOG_W
#undef LOG_I
#undef LOG_D
#undef LOG_V
#undef LOGRAW_N
#undef LOGRAW_C
#undef LOGRAW_E
#undef LOGRAW_W
#undef LOGRAW_I
#undef LOGRAW_D
#undef LOGRAW_V
#undef HEXDUMP_N
#undef HEXDUMP_C
#undef HEXDUMP_E
#undef HEXDUMP_W
#undef HEXDUMP_I
#undef HEXDUMP_D
#undef HEXDUMP_V
#endif

// Now we can define the macros based on LOCAL_LOG_LEVEL
#if IS_LINUX
#define LOG_LINE_C(level, x, format, ...) if (MBUlogLvl >= level) printf(LL_RED LOG_HEADER(x) format LL_NORM, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_LINE_E(level, x, format, ...) if (MBUlogLvl >= level) printf(LL_YELLOW LOG_HEADER(x) format LL_NORM, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_LINE_T(level, x, format, ...) if (MBUlogLvl >= level) printf(LOG_HEADER(x) format, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_RAW_C(level, x, format, ...) if (MBUlogLvl >= level) printf(LL_RED format LL_NORM, ##__VA_ARGS__)
#define LOG_RAW_E(level, x, format, ...) if (MBUlogLvl >= level) printf(LL_YELLOW format LL_NORM, ##__VA_ARGS__)
#define LOG_RAW_T(level, x, format, ...) if (MBUlogLvl >= level) printf(format, ##__VA_ARGS__)
#define HEX_DUMP_T(x, level, label, address, length) if (MBUlogLvl >= level) logHexDump(#x, label, address, length)
#else
#define LOG_LINE_C(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(LL_RED LOG_HEADER(x) format LL_NORM, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_LINE_E(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(LL_YELLOW LOG_HEADER(x) format LL_NORM, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_LINE_T(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(LOG_HEADER(x) format, millis(), file_name(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_RAW_C(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(LL_RED format LL_NORM, ##__VA_ARGS__)
#define LOG_RAW_E(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(LL_YELLOW format LL_NORM, ##__VA_ARGS__)
#define LOG_RAW_T(level, x, format, ...) if (MBUlogLvl >= level) LOGDEVICE->printf(format, ##__VA_ARGS__)
#define HEX_DUMP_T(x, level, label, address, length) if (MBUlogLvl >= level) logHexDump(LOGDEVICE, #x, label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_NONE
#define LOG_N(format, ...) LOG_LINE_T(LOG_LEVEL_NONE, N, format, ##__VA_ARGS__)
#define LOGRAW_N(format, ...) LOG_RAW_T(LOG_LEVEL_NONE, N, format, ##__VA_ARGS__)
#define HEXDUMP_N(label, address, length) HEX_DUMP_T(N, LOG_LEVEL_NONE, label, address, length)
#else
#define LOG_N(format, ...)
#define LOGRAW_N(format, ...)
#define HEXDUMP_N(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_CRITICAL
#define LOG_C(format, ...) LOG_LINE_C(LOG_LEVEL_CRITICAL, C, format, ##__VA_ARGS__)
#define LOGRAW_C(format, ...) LOG_RAW_C(LOG_LEVEL_CRITICAL, C, format, ##__VA_ARGS__)
#define HEXDUMP_C(label, address, length) HEX_DUMP_T(C, LOG_LEVEL_CRITICAL, label, address, length)
#else
#define LOG_C(format, ...)
#define LOGRAW_C(format, ...)
#define HEXDUMP_C(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(format, ...) LOG_LINE_E(LOG_LEVEL_ERROR, E, format, ##__VA_ARGS__)
#define LOGRAW_E(format, ...) LOG_RAW_E(LOG_LEVEL_ERROR, E, format, ##__VA_ARGS__)
#define HEXDUMP_E(label, address, length) HEX_DUMP_T(E, LOG_LEVEL_ERROR, label, address, length)
#else
#define LOG_E(format, ...)
#define LOGRAW_E(format, ...)
#define HEXDUMP_E(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_WARNING
#define LOG_W(format, ...) LOG_LINE_T(LOG_LEVEL_WARNING, W, format, ##__VA_ARGS__)
#define LOGRAW_W(format, ...) LOG_RAW_T(LOG_LEVEL_WARNING, W, format, ##__VA_ARGS__)
#define HEXDUMP_W(label, address, length) HEX_DUMP_T(W, LOG_LEVEL_WARNING, label, address, length)
#else
#define LOG_W(format, ...)
#define LOGRAW_W(format, ...)
#define HEXDUMP_W(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(format, ...) LOG_LINE_T(LOG_LEVEL_INFO, I, format, ##__VA_ARGS__)
#define LOGRAW_I(format, ...) LOG_RAW_T(LOG_LEVEL_INFO, I, format, ##__VA_ARGS__)
#define HEXDUMP_I(label, address, length) HEX_DUMP_T(I, LOG_LEVEL_INFO, label, address, length)
#else
#define LOG_I(format, ...)
#define LOGRAW_I(format, ...)
#define HEXDUMP_I(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(format, ...) LOG_LINE_T(LOG_LEVEL_DEBUG, D, format, ##__VA_ARGS__)
#define LOGRAW_D(format, ...) LOG_RAW_T(LOG_LEVEL_DEBUG, D, format, ##__VA_ARGS__)
#define HEXDUMP_D(label, address, length) HEX_DUMP_T(D, LOG_LEVEL_DEBUG, label, address, length)
#else
#define LOG_D(format, ...)
#define LOGRAW_D(format, ...)
#define HEXDUMP_D(label, address, length)
#endif

#if LOCAL_LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_V(format, ...) LOG_LINE_T(LOG_LEVEL_VERBOSE, V, format, ##__VA_ARGS__)
#define LOGRAW_V(format, ...) LOG_RAW_T(LOG_LEVEL_VERBOSE, V, format, ##__VA_ARGS__)
#define HEXDUMP_V(label, address, length) HEX_DUMP_T(V, LOG_LEVEL_VERBOSE, label, address, length)
#else
#define LOG_V(format, ...)
#define LOGRAW_V(format, ...)
#define HEXDUMP_V(label, address, length)
#endif

