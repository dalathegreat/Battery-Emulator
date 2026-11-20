#ifndef DISPLAY_MODES_H
#define DISPLAY_MODES_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// OLED page selected in Settings. Stored as NVM uint8 DISPMODE.
// 0 Status (today's tall SOC and rotating pages), 1 Health, 2 Cell bars, 3 Cycle.
// A missing key uses DISPLAY_MODE_DEFAULT_RAW. Any other stored value is Status too.
// 0 is Status: it is a real choice, not the unset marker.
enum class DisplayMode : uint8_t { Status = 0, Health = 1, CellBars = 2, Cycle = 3 };

// getUInt default when DISPMODE has never been stored.
static constexpr uint32_t DISPLAY_MODE_DEFAULT_RAW = static_cast<uint32_t>(DisplayMode::Status);

inline DisplayMode display_mode_from_raw(uint32_t raw) {
  if (raw > static_cast<uint32_t>(DisplayMode::Cycle)) {
    return DisplayMode::Status;
  }
  return static_cast<DisplayMode>(raw);
}

// How long Cycle leaves each page up: Status, then Health, then Cell bars.
static constexpr uint32_t DISPLAY_CYCLE_PAGE_MS = 4000;

// Page for a Cycle run that has been going for elapsed_ms. Unsigned elapsed wraps safely.
// 0 Status, 4000 Health, 8000 Cell bars, 12000 Status again.
inline DisplayMode display_cycle_page(uint32_t elapsed_ms) {
  static constexpr DisplayMode pages[] = {DisplayMode::Status, DisplayMode::Health, DisplayMode::CellBars};
  const uint32_t index = (elapsed_ms / DISPLAY_CYCLE_PAGE_MS) % (sizeof(pages) / sizeof(pages[0]));
  return pages[index];
}

// Boot-log token. Status keeps the name the startup diagnostics already used.
inline const char* display_mode_name(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::Health:
      return "health";
    case DisplayMode::CellBars:
      return "cell-bars";
    case DisplayMode::Cycle:
      return "cycle";
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
    health_pad(rows.line[1], "CAPACITY --");
  } else {
    snprintf(tmp, sizeof tmp, "CAPACITY %u.%ukWh", (unsigned)(in.capacity_x10 / 10u),
             (unsigned)(in.capacity_x10 % 10u));
    health_pad(rows.line[1], strlen(tmp) <= 21 ? tmp : "CAPACITY --");
  }

  if (!in.soh || in.soh_current_x10 == 0 || in.soh_new_x10 == 0) {
    health_pad(rows.line[2], "SOH --");
  } else {
    char current_kwh[8];
    char new_kwh[8];
    format_kwh_token(current_kwh, sizeof current_kwh, in.soh_current_x10);
    format_kwh_token(new_kwh, sizeof new_kwh, in.soh_new_x10);
    const unsigned pct = (unsigned)round_half_up((uint32_t)in.soh_current_x10 * 100u, in.soh_new_x10);
    snprintf(tmp, sizeof tmp, "SOH %u%% (%s/%skWh)", pct, current_kwh, new_kwh);
    if (strlen(tmp) > 21) {
      snprintf(tmp, sizeof tmp, "SOH %u%% %s/%s", pct, current_kwh, new_kwh);
    }
    if (strlen(tmp) > 21) {
      snprintf(tmp, sizeof tmp, "SOH %u%% %u/%u", pct, (unsigned)round_half_up(in.soh_current_x10, 10),
               (unsigned)round_half_up(in.soh_new_x10, 10));
    }
    health_pad(rows.line[2], strlen(tmp) <= 21 ? tmp : "SOH --");
  }

  if (!in.imbalance) {
    health_pad(rows.line[3], "CELL IMBAL --");
  } else {
    snprintf(tmp, sizeof tmp, "CELL IMBAL %umV", (unsigned)in.imbalance_mV);
    health_pad(rows.line[3], strlen(tmp) <= 21 ? tmp : "CELL IMBAL --");
  }

  if (!in.cac || in.cac_max_x10 < in.cac_min_x10) {
    health_pad(rows.line[4], "CAC --");
    health_pad(rows.line[5], "CAC IMBAL --");
  } else {
    const unsigned lo = (unsigned)round_half_up(in.cac_min_x10, 10);
    const unsigned hi = (unsigned)round_half_up(in.cac_max_x10, 10);
    snprintf(tmp, sizeof tmp, "CAC %u - %uAh", lo, hi);
    health_pad(rows.line[4], strlen(tmp) <= 21 ? tmp : "CAC --");

    const uint32_t delta_x10 = (uint32_t)in.cac_max_x10 - in.cac_min_x10;
    const unsigned delta_ah = (unsigned)round_half_up(delta_x10, 10);
    if (in.cac_avg_x10 == 0) {
      snprintf(tmp, sizeof tmp, "CAC IMBAL %uAh --%%", delta_ah);
    } else {
      const unsigned pct = (unsigned)round_half_up(delta_x10 * 100u, in.cac_avg_x10);
      snprintf(tmp, sizeof tmp, "CAC IMBAL %uAh  %u%%", delta_ah, pct);
    }
    health_pad(rows.line[5], strlen(tmp) <= 21 ? tmp : "CAC IMBAL --");
  }

  if (!in.socimb || in.soc_max_x10 < in.soc_min_x10 || in.soc_max_x10 > 1000) {
    health_pad(rows.line[6], "SOC IMBAL --");
  } else {
    const unsigned pct = (unsigned)round_half_up((uint32_t)in.soc_max_x10 - in.soc_min_x10, 10);
    snprintf(tmp, sizeof tmp, "SOC IMBAL %u%%", pct);
    health_pad(rows.line[6], strlen(tmp) <= 21 ? tmp : "SOC IMBAL --");
  }

  health_pad(rows.line[7], "");
}

// Cell bars. One column per cell on the 56px under the text row, centered in 128.
// A valid bar is at least 2px. A missing cell (0 mV) is a 1px dot, never a bar.
static constexpr uint16_t CELL_BAR_MIN_SPAN_MV = 20;
static constexpr uint8_t CELL_BAR_PX = 56;
static constexpr uint8_t CELL_BAR_DOT_PX = 1;
static constexpr uint8_t CELL_BAR_MIN_PX = 2;
static constexpr uint16_t CELL_BAR_MAX_CELLS = 128;

// Legacy S/X modules are 6 bricks. 96 and 84 are exact module counts; 83 is not.
// 3/Y 96 is either 4x24 or 23+25+25+23, and 108 LFP has no single verified module
// order, so a 3/Y cell count alone does not get gaps. Plaid 110 is not grouped.
enum class CellPackFamily : uint8_t { Unknown = 0, TeslaLegacySX = 1, Tesla3Y = 2 };

inline uint8_t equal_module_bricks(CellPackFamily family, uint16_t cells) {
  if (family == CellPackFamily::TeslaLegacySX && (cells == 96 || cells == 84)) {
    return 6;
  }
  return 0;
}

struct CellScale {
  bool have = false;
  uint16_t min_mV = 0;
  uint16_t max_mV = 0;
  uint16_t lo_mV = 0;
  uint16_t span_mV = CELL_BAR_MIN_SPAN_MV;
};

inline CellScale cell_voltage_scale(const uint16_t* mv, uint16_t count) {
  CellScale scale;
  if (mv == nullptr || count == 0) {
    return scale;
  }
  const uint16_t n = count > CELL_BAR_MAX_CELLS ? CELL_BAR_MAX_CELLS : count;
  bool any = false;
  uint16_t min_mV = 0;
  uint16_t max_mV = 0;
  for (uint16_t i = 0; i < n; i++) {
    if (mv[i] == 0) {
      continue;
    }
    if (!any || mv[i] < min_mV) {
      min_mV = mv[i];
    }
    if (!any || mv[i] > max_mV) {
      max_mV = mv[i];
    }
    any = true;
  }
  if (!any) {
    return scale;
  }
  scale.have = true;
  scale.min_mV = min_mV;
  scale.max_mV = max_mV;
  const uint16_t delta = (uint16_t)(max_mV - min_mV);
  scale.span_mV = delta < CELL_BAR_MIN_SPAN_MV ? CELL_BAR_MIN_SPAN_MV : delta;
  const uint16_t pad = (uint16_t)((scale.span_mV - delta) / 2u);
  scale.lo_mV = pad > min_mV ? 0 : (uint16_t)(min_mV - pad);
  return scale;
}

inline uint8_t cell_bar_height(uint16_t mv, const CellScale& scale) {
  if (!scale.have || mv == 0) {
    return CELL_BAR_DOT_PX;
  }
  const uint16_t above = mv > scale.lo_mV ? (uint16_t)(mv - scale.lo_mV) : 0;
  uint32_t height = (uint32_t)above * (CELL_BAR_PX - CELL_BAR_MIN_PX) / scale.span_mV + CELL_BAR_MIN_PX;
  if (height < CELL_BAR_MIN_PX) {
    height = CELL_BAR_MIN_PX;
  }
  if (height > CELL_BAR_PX) {
    height = CELL_BAR_PX;
  }
  return (uint8_t)height;
}

// column[x] is 0 (empty, including a module gap), 1 (missing cell) or 2..56 (bar).
// False when there is no valid cell to scale against.
inline bool layout_cell_bars(CellPackFamily family, const uint16_t* mv, uint16_t count, uint8_t column[128]) {
  memset(column, 0, 128);
  if (mv == nullptr || count == 0) {
    return false;
  }
  const uint16_t n = count > CELL_BAR_MAX_CELLS ? CELL_BAR_MAX_CELLS : count;
  const CellPackFamily use_family = count > CELL_BAR_MAX_CELLS ? CellPackFamily::Unknown : family;
  const CellScale scale = cell_voltage_scale(mv, n);
  if (!scale.have) {
    return false;
  }

  uint8_t bricks = equal_module_bricks(use_family, n);
  uint16_t gaps = 0;
  if (bricks >= 2 && (n % bricks) == 0) {
    gaps = (uint16_t)(n / bricks - 1u);
    if ((uint16_t)(n + gaps) > 128) {
      bricks = 0;
      gaps = 0;
    }
  } else {
    bricks = 0;
  }

  uint16_t x = (uint16_t)((128u - (n + gaps)) / 2u);
  for (uint16_t i = 0; i < n; i++) {
    if (bricks != 0 && i > 0 && (i % bricks) == 0) {
      x++;
    }
    column[x] = mv[i] == 0 ? CELL_BAR_DOT_PX : cell_bar_height(mv[i], scale);
    x++;
  }
  return true;
}

inline bool format_cell_header(char dest[22], uint16_t min_mV, uint16_t max_mV) {
  if (max_mV < min_mV) {
    return false;
  }
  const unsigned delta = (unsigned)(max_mV - min_mV);
  int n = snprintf(dest, 22, "%u.%03u-%u.%03uV  %umV", (unsigned)(min_mV / 1000u), (unsigned)(min_mV % 1000u),
                   (unsigned)(max_mV / 1000u), (unsigned)(max_mV % 1000u), delta);
  if (n < 0 || n > 21) {
    n = snprintf(dest, 22, "%u.%03u-%u.%03uV %umV", (unsigned)(min_mV / 1000u), (unsigned)(min_mV % 1000u),
                 (unsigned)(max_mV / 1000u), (unsigned)(max_mV % 1000u), delta);
  }
  return n >= 0 && n <= 21;
}

struct CellHold {
  bool known = false;
  uint32_t ms = 0;
  uint16_t count = 0;
  CellPackFamily family = CellPackFamily::Unknown;
  uint16_t mv[CELL_BAR_MAX_CELLS]{};
};

// A live sample needs one non-zero cell. After that the same columns stay up for 30 minutes.
inline bool cell_hold_step(CellHold& hold, bool live, CellPackFamily family, const uint16_t* mv, uint16_t count,
                           uint32_t now_ms) {
  const uint16_t n = count > CELL_BAR_MAX_CELLS ? CELL_BAR_MAX_CELLS : count;
  bool sample = false;
  if (live && mv != nullptr && n > 0) {
    for (uint16_t i = 0; i < n; i++) {
      if (mv[i] != 0) {
        sample = true;
        break;
      }
    }
  }
  if (sample) {
    hold.known = true;
    hold.ms = now_ms;
    hold.count = n;
    hold.family = count > CELL_BAR_MAX_CELLS ? CellPackFamily::Unknown : family;
    memcpy(hold.mv, mv, n * sizeof(uint16_t));
    return true;
  }
  if (hold.known && (uint32_t)(now_ms - hold.ms) <= DISPLAY_VALUE_HOLD_MS) {
    return true;
  }
  hold.known = false;
  hold.count = 0;
  return false;
}

#endif  // DISPLAY_MODES_H
