#include "restart_watchdog.h"

#include "../utils/logging.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "hal/wdt_hal.h"

/* esp_restart() ends in esp_restart_noos(), which gives the RTC watchdog one second to see the bootloader
   take over: it sets the stage timeouts and turns on flash-boot mode, in which this watchdog counts even
   while disabled. It does not feed it, so the count goes on from wherever startup left it. Startup fed it
   in esp_clk_init() and switched it off in init_disable_rtc_wdt(), the last step before app_main(), and in
   between Arduino's PSRAM init tests all of the PSRAM, 8 MB on the Waveshare board. When what is left of
   the second is not enough for the ROM and the start of the bootloader, the watchdog resets the whole chip,
   RTC included: the board comes back as RESET_WDT instead of RESET_SW, with whatever the RTC domain held
   gone, and after an OTA still on the new image, as the bootloader had not got far enough to mark it tried.

   So right before that, the counter is taken back to zero: started with every stage off, so it cannot act,
   fed while it runs, and stopped again. That works whether or not the counter takes a feed while stopped. */

static void restart_watchdog_feed() {
  wdt_hal_context_t rtc_wdt;
  wdt_hal_init(&rtc_wdt, WDT_RWDT, 0, false);  // Stopped, every stage off
  wdt_hal_write_protect_disable(&rtc_wdt);
  wdt_hal_enable(&rtc_wdt);  // Feeds it, then starts it
  wdt_hal_feed(&rtc_wdt);
  esp_rom_delay_us(100);  // A dozen cycles of the ~136 kHz clock it counts, for the feed to get through
  wdt_hal_disable(&rtc_wdt);
  wdt_hal_write_protect_enable(&rtc_wdt);
}

void register_restart_watchdog_feed() {
  if (esp_register_shutdown_handler(restart_watchdog_feed) != ESP_OK) {
    logging.println("Could not register the restart watchdog feed, restarts may come back as RESET_WDT");
  }
}
