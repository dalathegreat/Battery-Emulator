#ifndef DISPLAY_MODES_H
#define DISPLAY_MODES_H

#include <stdint.h>

// OLED page selected in Settings. Stored as NVM uint8 DISPMODE.
// 0 Status (today's tall SOC and rotating pages), 1 Health, 2 Cell bars.
// A missing key or any other value is Status.
enum class DisplayMode : uint8_t { Status = 0, Health = 1, CellBars = 2 };

inline DisplayMode display_mode_from_raw(uint32_t raw) {
  if (raw == static_cast<uint32_t>(DisplayMode::Health)) {
    return DisplayMode::Health;
  }
  if (raw == static_cast<uint32_t>(DisplayMode::CellBars)) {
    return DisplayMode::CellBars;
  }
  return DisplayMode::Status;
}

// Boot-log token. Status keeps the name the startup diagnostics already used.
inline const char* display_mode_name(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::Health:
      return "health";
    case DisplayMode::CellBars:
      return "cell-bars";
    default:
      return "status-pages";
  }
}

// Loaded from DISPMODE in init_stored_settings(). Default Status.
extern DisplayMode user_selected_display_mode;

#endif  // DISPLAY_MODES_H
