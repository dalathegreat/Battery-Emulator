#include "restart_core_reset.h"

#include <Arduino.h>
#include "../utils/logging.h"

#if CONFIG_IDF_TARGET_ESP32S3

#include "esp_cpu.h"
#include "esp_newlib.h"           // esp_sync_timekeeping_timers()
#include "esp_private/rtc_clk.h"  // rtc_clk_cpu_set_to_default_config()
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/wdt_hal.h"
#include "soc/rtc.h"

/* After esp_restart_noos()'s CPU reset, the ROM on the Waveshare board often read wrong data from flash
   while loading the bootloader: segment headers of zeros, then a checksum failure, or a StoreProhibited
   crash in ets_run_flash_bootloader(). A second later the RTC watchdog reset the whole chip, RTC included,
   and the board came back as RESET_WDT, with what the RTC domain kept gone, and after an OTA still on the
   new image. A CPU reset leaves the rest of the digital system as the app left it, and esp_restart_noos()
   resets only part of it again. A core reset resets all of it but the RTC domain, so RTC memory and the
   RTC counter survive it just as they survive a CPU reset.

   So the restart ends in a core reset, the way IDF's esp_restart_noos_dig() does it on the ESP32 (IDF
   builds that one for no other chip), after the RTC watchdog is armed exactly as esp_restart_noos() arms
   it, so a boot that fails all the same is still caught. Not inlined, so it stays in IRAM. */
[[noreturn]] static void __attribute__((noinline)) IRAM_ATTR restart_with_core_reset() {
  esp_cpu_intr_disable(0xFFFFFFFF);

  wdt_hal_context_t rtc_wdt;
  wdt_hal_init(&rtc_wdt, WDT_RWDT, 0, false);
  const uint32_t one_second = rtc_clk_slow_freq_get_hz();
  wdt_hal_write_protect_disable(&rtc_wdt);
  wdt_hal_config_stage(&rtc_wdt, WDT_STAGE0, one_second, WDT_STAGE_ACTION_RESET_SYSTEM);
  wdt_hal_config_stage(&rtc_wdt, WDT_STAGE1, one_second, WDT_STAGE_ACTION_RESET_RTC);
  wdt_hal_set_flashboot_en(&rtc_wdt, true);
  wdt_hal_write_protect_enable(&rtc_wdt);

#if CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM != -1
  esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM);
#endif
  rtc_clk_cpu_set_to_default_config();  // CPU back on XTAL
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  // The stall bits are in the RTC domain, so a core reset would keep them
  esp_cpu_unstall(esp_cpu_get_core_id() == 0 ? 1 : 0);
#endif
  esp_rom_software_reset_system();
  while (true) {}
}

static void restart_core_reset_handler() {
  esp_sync_timekeeping_timers();  // The one handler esp_restart() would still have run after this one
  vTaskSuspendAll();              // As esp_restart() does before esp_restart_noos()
  restart_with_core_reset();
}

void register_restart_core_reset() {
  if (esp_register_shutdown_handler(restart_core_reset_handler) != ESP_OK) {
    logging.println("Could not register the core reset restart, restarts may come back as RESET_WDT");
  }
}

#else

void register_restart_core_reset() {}

#endif
