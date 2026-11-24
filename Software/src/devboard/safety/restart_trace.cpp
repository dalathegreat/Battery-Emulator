#include "restart_trace.h"

#include <Arduino.h>
#include "../utils/logging.h"

#if CONFIG_IDF_TARGET_ESP32S3 && !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_newlib.h"                    // esp_sync_timekeeping_timers()
#include "esp_private/esp_clk.h"           // esp_clk_slowclk_cal_get()
#include "esp_private/rtc_clk.h"           // rtc_clk_cpu_set_to_default_config()
#include "esp_private/startup_internal.h"  // ESP_SYSTEM_INIT_FN
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/wdt_hal.h"
#include "soc/rtc.h"

enum : uint32_t {
  STEP_NONE = 0,
  STEP_ENTERED,     // The shutdown handlers before this one ran: Wi-Fi stopped, watchdog counter zeroed
  STEP_RWDT_ARMED,  // The RTC watchdog's one second runs from here
  STEP_CORE_RESET,  // From here on it is the ROM, then the bootloader
  STEP_COUNT
};

static const char* const STEP_NAMES[STEP_COUNT] = {"none", "entered", "RTC watchdog armed", "core reset"};

struct RestartTrace {
  uint32_t magic;
  uint32_t step;
  uint32_t app_starts;   // Counted from early startup, since the restart began
  uint64_t reset_ticks;  // RTC counter as the core is reset. A core reset leaves the counter running,
  uint64_t start_ticks;  // so this, read at the first app start, tells how long the ROM and bootloader took
};
static constexpr uint32_t TRACE_MAGIC = 0x52535431;  // "RST1"

// Kept twice, to learn which survives the reset: internal RAM and RTC memory. Both stay readable with
// the caches off, and startup clears neither.
static __NOINIT_ATTR volatile RestartTrace trace_ram;
static RTC_NOINIT_ATTR volatile RestartTrace trace_rtc;

static inline void IRAM_ATTR mark(uint32_t step) {
  trace_ram.step = step;
  trace_rtc.step = step;
}

/* The serial log of a failing restart shows what goes wrong: after esp_restart_noos()'s CPU reset the ROM
   reads wrong data from flash while it loads the bootloader (load:0x00000000, a checksum failure, or a
   StoreProhibited crash in ets_run_flash_bootloader()), until the RTC watchdog resets the whole chip, and
   then it reads fine. A CPU reset leaves the rest of the digital system as the app left it, and
   esp_restart_noos() resets only part of it again. A core reset resets all of it but the RTC domain, so
   RTC memory and the RTC counter still survive it.

   So this restarts with a core reset, the way esp_restart_noos_dig() does it on the ESP32 (IDF builds that
   function for no other chip), after arming the RTC watchdog exactly as esp_restart_noos() does, so a boot
   that still fails ends the same way as before. Not inlined: inside its flash-resident caller it would lose
   IRAM_ATTR. */
[[noreturn]] static void __attribute__((noinline)) IRAM_ATTR core_reset_restart() {
  esp_cpu_intr_disable(0xFFFFFFFF);

  wdt_hal_context_t rtc_wdt;
  wdt_hal_init(&rtc_wdt, WDT_RWDT, 0, false);
  const uint32_t stage_timeout_ticks = rtc_clk_slow_freq_get_hz();  // One second
  wdt_hal_write_protect_disable(&rtc_wdt);
  wdt_hal_config_stage(&rtc_wdt, WDT_STAGE0, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
  wdt_hal_config_stage(&rtc_wdt, WDT_STAGE1, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);
  wdt_hal_set_flashboot_en(&rtc_wdt, true);
  wdt_hal_write_protect_enable(&rtc_wdt);
  mark(STEP_RWDT_ARMED);

#if CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM != -1
  esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM);
#endif
  rtc_clk_cpu_set_to_default_config();  // CPU back on XTAL
  // The stall bits are in the RTC domain, so a core reset would keep them
  esp_cpu_unstall(esp_cpu_get_core_id() == 0 ? 1 : 0);

  const uint64_t ticks = rtc_time_get();
  trace_ram.reset_ticks = ticks;
  trace_rtc.reset_ticks = ticks;
  mark(STEP_CORE_RESET);
  esp_rom_software_reset_system();
  while (true) {}
}

// Registered ahead of Wi-Fi and of the watchdog feed, so it runs after both, and never returns: it does
// what is left of esp_restart() itself
static void restart_trace_handler() {
  trace_ram.app_starts = 0;
  trace_rtc.app_starts = 0;
  trace_ram.magic = TRACE_MAGIC;
  trace_rtc.magic = TRACE_MAGIC;
  mark(STEP_ENTERED);
  esp_sync_timekeeping_timers();  // The one handler left after this one
  vTaskSuspendAll();              // As esp_restart() does before esp_restart_noos()
  core_reset_restart();
}

// Early in startup, before PSRAM init: tells a stop in the ROM or the bootloader from one in the app
ESP_SYSTEM_INIT_FN(restart_trace_app_start, CORE, BIT(0), 2) {
  const uint64_t ticks = rtc_time_get();
  if (trace_ram.magic == TRACE_MAGIC) {
    if (trace_ram.app_starts == 0) {
      trace_ram.start_ticks = ticks;
    }
    trace_ram.app_starts = trace_ram.app_starts + 1;
  }
  if (trace_rtc.magic == TRACE_MAGIC) {
    if (trace_rtc.app_starts == 0) {
      trace_rtc.start_ticks = ticks;
    }
    trace_rtc.app_starts = trace_rtc.app_starts + 1;
  }
  return ESP_OK;
}

void register_restart_trace() {
  if (esp_register_shutdown_handler(restart_trace_handler) != ESP_OK) {
    logging.println("Could not register the restart trace");
  }
}

void restart_trace_report() {
  const bool in_ram = trace_ram.magic == TRACE_MAGIC;
  const bool in_rtc = trace_rtc.magic == TRACE_MAGIC;
  if (in_ram || in_rtc) {
    const volatile RestartTrace& trace = in_ram ? trace_ram : trace_rtc;
    const uint32_t step = trace.step;
    const uint32_t app_starts = trace.app_starts;
    logging.printf("Last restart got to: %s (step %u), app starts since: %u (kept in RAM: %s, RTC: %s)\n",
                   step < STEP_COUNT ? STEP_NAMES[step] : "?", (unsigned)step, (unsigned)app_starts,
                   in_ram ? "yes" : "no", in_rtc ? "yes" : "no");
    if (step == STEP_CORE_RESET && app_starts > 0) {
      const uint64_t reset_ticks = trace.reset_ticks;
      const uint64_t start_ticks = trace.start_ticks;
      const uint32_t cal = esp_clk_slowclk_cal_get();  // RTC slow clock period in us, Q13.19
      if (start_ticks >= reset_ticks && cal) {
        logging.printf("Last restart: ROM and bootloader took %u ms, core reset to app start\n",
                       (unsigned)(((start_ticks - reset_ticks) * cal >> 19) / 1000));
      } else {
        logging.println("Last restart: the RTC counter started over, so the chip had an RTC reset in between");
      }
    }
  } else {
    logging.println("Last restart got to: no trace");
  }
  /* Ready for the next one. The magic stays, so a reset that comes while no restart is under way shows
     as "none", and only memory the reset wiped (or a power-up) as no trace. */
  trace_ram.step = STEP_NONE;
  trace_rtc.step = STEP_NONE;
  trace_ram.app_starts = 0;
  trace_rtc.app_starts = 0;
  trace_ram.magic = TRACE_MAGIC;
  trace_rtc.magic = TRACE_MAGIC;
}

#else

void register_restart_trace() {}
void restart_trace_report() {}

#endif
