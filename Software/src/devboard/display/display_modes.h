#ifndef DISPLAY_MODES_H
#define DISPLAY_MODES_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

// Last good reading stays on the glass this long after it stops being live, then the row is "--".
static constexpr uint32_t DISPLAY_VALUE_HOLD_MS = 30UL * 60UL * 1000UL;

// live updates the stamp. Otherwise the previous reading is kept until the hold expires.
inline bool display_hold_current(bool live, bool* known, uint32_t* stamp_ms, uint32_t now_ms) {
  if (live) {
    *known = true;
    *stamp_ms = now_ms;
    return true;
  }
  if (*known && (uint32_t)(now_ms - *stamp_ms) <= DISPLAY_VALUE_HOLD_MS) {
    return true;
  }
  *known = false;
  return false;
}

// Nonnegative half-up division. 3.2 Ah in tenths (32/10) is 3; 1.50 (150/100) is 2; 1.49 is 1.
inline uint32_t round_half_up(uint32_t numerator, uint32_t denominator) {
  return (numerator + denominator / 2u) / denominator;
}

// One health frame. Flags are independent: a missing CAC does not blank SOC.
// Capacity and the SOH operands are 0.1 kWh. CAC is 0.1 Ah. SOC is 0.01 %. Brick SOC is 0.1 %.
struct HealthInputs {
  bool soc = false;
  uint16_t soc_pptt = 0;
  bool capacity = false;
  uint16_t capacity_x10 = 0;
  bool soh = false;
  uint16_t soh_current_x10 = 0;
  uint16_t soh_new_x10 = 0;
  bool imbalance = false;
  uint16_t imbalance_mV = 0;
  bool cac = false;
  uint16_t cac_min_x10 = 0;
  uint16_t cac_max_x10 = 0;
  uint16_t cac_avg_x10 = 0;
  bool socimb = false;
  uint16_t soc_min_x10 = 0;
  uint16_t soc_max_x10 = 0;
};

struct HealthHold {
  bool soc_known = false;
  bool capacity_known = false;
  bool soh_known = false;
  bool imbalance_known = false;
  bool cac_known = false;
  bool socimb_known = false;
  uint32_t soc_ms = 0;
  uint32_t capacity_ms = 0;
  uint32_t soh_ms = 0;
  uint32_t imbalance_ms = 0;
  uint32_t cac_ms = 0;
  uint32_t socimb_ms = 0;
  HealthInputs saved{};
};

struct HealthRows {
  char line[8][22];
};

inline void health_pad(char dest[22], const char* text) {
  memset(dest, ' ', 21);
  dest[21] = '\0';
  size_t n = strlen(text);
  if (n > 21) {
    n = 21;
  }
  memcpy(dest, text, n);
}

inline HealthInputs health_hold_step(HealthHold& hold, const HealthInputs& live, uint32_t now_ms) {
  if (live.soc) {
    hold.saved.soc_pptt = live.soc_pptt;
  }
  if (live.capacity) {
    hold.saved.capacity_x10 = live.capacity_x10;
  }
  if (live.soh) {
    hold.saved.soh_current_x10 = live.soh_current_x10;
    hold.saved.soh_new_x10 = live.soh_new_x10;
  }
  if (live.imbalance) {
    hold.saved.imbalance_mV = live.imbalance_mV;
  }
  if (live.cac) {
    hold.saved.cac_min_x10 = live.cac_min_x10;
    hold.saved.cac_max_x10 = live.cac_max_x10;
    hold.saved.cac_avg_x10 = live.cac_avg_x10;
  }
  if (live.socimb) {
    hold.saved.soc_min_x10 = live.soc_min_x10;
    hold.saved.soc_max_x10 = live.soc_max_x10;
  }

  HealthInputs shown{};
  shown.soc = display_hold_current(live.soc, &hold.soc_known, &hold.soc_ms, now_ms);
  shown.soc_pptt = hold.saved.soc_pptt;
  shown.capacity = display_hold_current(live.capacity, &hold.capacity_known, &hold.capacity_ms, now_ms);
  shown.capacity_x10 = hold.saved.capacity_x10;
  shown.soh = display_hold_current(live.soh, &hold.soh_known, &hold.soh_ms, now_ms);
  shown.soh_current_x10 = hold.saved.soh_current_x10;
  shown.soh_new_x10 = hold.saved.soh_new_x10;
  shown.imbalance = display_hold_current(live.imbalance, &hold.imbalance_known, &hold.imbalance_ms, now_ms);
  shown.imbalance_mV = hold.saved.imbalance_mV;
  shown.cac = display_hold_current(live.cac, &hold.cac_known, &hold.cac_ms, now_ms);
  shown.cac_min_x10 = hold.saved.cac_min_x10;
  shown.cac_max_x10 = hold.saved.cac_max_x10;
  shown.cac_avg_x10 = hold.saved.cac_avg_x10;
  shown.socimb = display_hold_current(live.socimb, &hold.socimb_known, &hold.socimb_ms, now_ms);
  shown.soc_min_x10 = hold.saved.soc_min_x10;
  shown.soc_max_x10 = hold.saved.soc_max_x10;
  return shown;
}

inline void format_kwh_token(char* dest, size_t n, uint16_t x10) {
  if ((x10 % 10u) == 0) {
    snprintf(dest, n, "%u", (unsigned)(x10 / 10u));
  } else {
    snprintf(dest, n, "%u.%u", (unsigned)(x10 / 10u), (unsigned)(x10 % 10u));
  }
}

inline void format_health(const HealthInputs& in, HealthRows& rows) {
  char tmp[40];

  if (!in.soc || in.soc_pptt > 10000) {
    health_pad(rows.line[0], "SOC --");
  } else {
    snprintf(tmp, sizeof tmp, "SOC %u%%", (unsigned)round_half_up(in.soc_pptt, 100));
    health_pad(rows.line[0], tmp);
  }

  if (!in.capacity) {
    health_pad(rows.line[1], "Capacity --");
  } else {
    snprintf(tmp, sizeof tmp, "Capacity %u.%ukWh", (unsigned)(in.capacity_x10 / 10u),
             (unsigned)(in.capacity_x10 % 10u));
    health_pad(rows.line[1], strlen(tmp) <= 21 ? tmp : "Capacity --");
  }

  if (!in.soh || in.soh_current_x10 == 0 || in.soh_new_x10 == 0) {
    health_pad(rows.line[2], "SOH --");
  } else {
    char current_kwh[8];
    char new_kwh[8];
    format_kwh_token(current_kwh, sizeof current_kwh, in.soh_current_x10);
    format_kwh_token(new_kwh, sizeof new_kwh, in.soh_new_x10);
    const unsigned pct =
        (unsigned)round_half_up((uint32_t)in.soh_current_x10 * 100u, in.soh_new_x10);
    snprintf(tmp, sizeof tmp, "SOH %u%% (%s/%skWh)", pct, current_kwh, new_kwh);
    health_pad(rows.line[2], strlen(tmp) <= 21 ? tmp : "SOH --");
  }

  if (!in.imbalance) {
    health_pad(rows.line[3], "Imbalance --");
  } else {
    snprintf(tmp, sizeof tmp, "Imbalance %umV", (unsigned)in.imbalance_mV);
    health_pad(rows.line[3], strlen(tmp) <= 21 ? tmp : "Imbalance --");
  }

  if (!in.cac || in.cac_max_x10 < in.cac_min_x10) {
    health_pad(rows.line[4], "CAC --");
    health_pad(rows.line[5], "CACimb --");
  } else {
    const unsigned lo = (unsigned)round_half_up(in.cac_min_x10, 10);
    const unsigned hi = (unsigned)round_half_up(in.cac_max_x10, 10);
    snprintf(tmp, sizeof tmp, "CAC %u - %uAh", lo, hi);
    health_pad(rows.line[4], strlen(tmp) <= 21 ? tmp : "CAC --");

    const uint32_t delta_x10 = (uint32_t)in.cac_max_x10 - in.cac_min_x10;
    const unsigned delta_ah = (unsigned)round_half_up(delta_x10, 10);
    if (in.cac_avg_x10 == 0) {
      snprintf(tmp, sizeof tmp, "CACimb %uAh --%%", delta_ah);
    } else {
      const unsigned pct = (unsigned)round_half_up(delta_x10 * 100u, in.cac_avg_x10);
      snprintf(tmp, sizeof tmp, "CACimb %uAh  %u%%", delta_ah, pct);
    }
    health_pad(rows.line[5], strlen(tmp) <= 21 ? tmp : "CACimb --");
  }

  if (!in.socimb || in.soc_max_x10 < in.soc_min_x10 || in.soc_max_x10 > 1000) {
    health_pad(rows.line[6], "SOCimb --");
  } else {
    const unsigned pct = (unsigned)round_half_up((uint32_t)in.soc_max_x10 - in.soc_min_x10, 10);
    snprintf(tmp, sizeof tmp, "SOCimb %u%%", pct);
    health_pad(rows.line[6], strlen(tmp) <= 21 ? tmp : "SOCimb --");
  }

  health_pad(rows.line[7], "");
}

#endif  // DISPLAY_MODES_H
