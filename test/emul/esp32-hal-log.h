#ifndef ESP32_HAL_LOG_H
#define ESP32_HAL_LOG_H

void log_printf(const char* format, ...);

#define log_d(format, ...) log_printf(format, ##__VA_ARGS__)
#define log_e(format, ...) log_printf(format, ##__VA_ARGS__)
#define log_i(format, ...) log_printf(format, ##__VA_ARGS__)
#define log_w(format, ...) log_printf(format, ##__VA_ARGS__)
#define log_v(format, ...) log_printf(format, ##__VA_ARGS__)

#endif
