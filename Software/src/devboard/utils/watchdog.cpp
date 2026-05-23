#include "watchdog.h"
#include "logging.h"

#include <Arduino.h>

void Watchdog::update() {
  last_update_time_ms = millis();
}

void Watchdog::panic_if_exceeded_ms(uint32_t timeout_ms, const char* message) {
  unsigned long current_time_ms = millis();
  unsigned long last_time_ms = last_update_time_ms.load();
  if (last_time_ms > 0 && (current_time_ms - last_time_ms) > timeout_ms) {
    logging.println(message);
    esp_system_abort(message);
  }
}
