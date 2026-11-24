#include "restart_trace.h"

#include <Arduino.h>
#include "../utils/logging.h"

#if CONFIG_IDF_TARGET_ESP32S3 && !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE

#include "esp32s3/rom/cache.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_memory_utils.h"
#include "esp_newlib.h"                    // esp_sync_timekeeping_timers()
#include "esp_private/rtc_clk.h"           // rtc_clk_cpu_set_to_default_config()
#include "esp_private/startup_internal.h"  // ESP_SYSTEM_INIT_FN
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/uart_ll.h"
#include "hal/wdt_hal.h"
#include "soc/gpio_reg.h"
#include "soc/rtc.h"
#include "soc/syscon_reg.h"  // SYSTEM_CORE_RST_EN_REG and its bits
#include "soc/system_reg.h"
#include "soc/timer_group_struct.h"

enum : uint32_t {
  STEP_NONE = 0,
  STEP_ENTERED,     // The shutdown handlers before this one ran: Wi-Fi stopped, watchdog counter zeroed
  STEP_RWDT_ARMED,  // The RTC watchdog's one second runs from here
  STEP_MWDT_OFF,
  STEP_OTHER_CPU_STALLED,
  STEP_ICACHE_OFF,
  STEP_DCACHE_OFF,
  STEP_UART0_FLUSH,  // Waiting for UART0 to send what is left in its FIFO
  STEP_UART1_FLUSH,
  STEP_UART2_FLUSH,
  STEP_MODULES_RESET,
  STEP_CPU_ON_XTAL,
  STEP_CPUS_RESET,  // From here on it is the ROM, then the bootloader
  STEP_COUNT
};

static const char* const STEP_NAMES[STEP_COUNT] = {
    "none",          "entered",     "RTC watchdog armed", "timer watchdogs off", "other CPU stalled",
    "ICache off",    "DCache off",  "flushing UART0",     "flushing UART1",      "flushing UART2",
    "modules reset", "CPU on XTAL", "CPUs reset"};

struct RestartTrace {
  uint32_t magic;
  uint32_t step;
  uint32_t app_starts;  // Counted from early startup, since the restart began
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

// IDF 5.5.6's esp_restart_noos() for the ESP32-S3, step for step, with esp_system_reset_modules_on_exit()
// written out in it, and a mark after each step
// Not inlined: inside its flash-resident caller it would lose IRAM_ATTR, and run from flash with the caches off
[[noreturn]] static void __attribute__((noinline)) IRAM_ATTR traced_restart_noos() {
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

  wdt_hal_context_t mwdt;
  mwdt.inst = WDT_MWDT0;
  mwdt.mwdt_dev = &TIMERG0;
  wdt_hal_write_protect_disable(&mwdt);
  wdt_hal_disable(&mwdt);
  wdt_hal_write_protect_enable(&mwdt);
  mwdt.inst = WDT_MWDT1;
  mwdt.mwdt_dev = &TIMERG1;
  wdt_hal_write_protect_disable(&mwdt);
  wdt_hal_disable(&mwdt);
  wdt_hal_write_protect_enable(&mwdt);
  mark(STEP_MWDT_OFF);

  const uint32_t core_id = esp_cpu_get_core_id();
  const uint32_t other_core_id = (core_id == 0) ? 1 : 0;
  esp_rom_software_reset_cpu(other_core_id);
  esp_cpu_stall(other_core_id);
  mark(STEP_OTHER_CPU_STALLED);

  Cache_Disable_ICache();
  mark(STEP_ICACHE_OFF);
  Cache_Disable_DCache();
  mark(STEP_DCACHE_OFF);

  WRITE_PERI_REG(GPIO_FUNC0_IN_SEL_CFG_REG, 0x30);
  WRITE_PERI_REG(GPIO_FUNC1_IN_SEL_CFG_REG, 0x30);
  WRITE_PERI_REG(GPIO_FUNC2_IN_SEL_CFG_REG, 0x30);
  WRITE_PERI_REG(GPIO_FUNC3_IN_SEL_CFG_REG, 0x30);
  WRITE_PERI_REG(GPIO_FUNC4_IN_SEL_CFG_REG, 0x30);
  WRITE_PERI_REG(GPIO_FUNC5_IN_SEL_CFG_REG, 0x30);

  for (uint32_t i = 0; i < SOC_UART_HP_NUM; ++i) {
    if (uart_ll_is_enabled(i)) {
      mark(STEP_UART0_FLUSH + i);
      esp_rom_output_tx_wait_idle(i);
    }
  }

  SET_PERI_REG_MASK(SYSTEM_CORE_RST_EN_REG, SYSTEM_WIFIBB_RST | SYSTEM_FE_RST | SYSTEM_WIFIMAC_RST | SYSTEM_SDIO_RST |
                                                SYSTEM_EMAC_RST | SYSTEM_MACPWR_RST | SYSTEM_BTBB_RST |
                                                SYSTEM_BTBB_REG_RST | SYSTEM_RW_BTMAC_RST | SYSTEM_RW_BTLP_RST |
                                                SYSTEM_RW_BTMAC_REG_RST | SYSTEM_RW_BTLP_REG_RST);
  REG_WRITE(SYSTEM_CORE_RST_EN_REG, 0);
  SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_TIMERS_RST | SYSTEM_SPI01_RST | SYSTEM_UART_RST |
                                                  SYSTEM_SYSTIMER_RST | SYSTEM_PWM0_RST | SYSTEM_PWM1_RST);
  REG_WRITE(SYSTEM_PERIP_RST_EN0_REG, 0);
  SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_DMA_RST | SYSTEM_CRYPTO_AES_RST | SYSTEM_CRYPTO_DS_RST |
                                                  SYSTEM_CRYPTO_HMAC_RST | SYSTEM_CRYPTO_RSA_RST |
                                                  SYSTEM_CRYPTO_SHA_RST | SYSTEM_SDIO_HOST_RST);
  REG_WRITE(SYSTEM_PERIP_RST_EN1_REG, 0);
  SET_PERI_REG_MASK(SYSTEM_EDMA_CTRL_REG, SYSTEM_EDMA_RESET);
  CLEAR_PERI_REG_MASK(SYSTEM_EDMA_CTRL_REG, SYSTEM_EDMA_RESET);
  mark(STEP_MODULES_RESET);

  rtc_clk_cpu_set_to_default_config();
  mark(STEP_CPU_ON_XTAL);

  REG_WRITE(SYSTEM_CORE_1_CONTROL_1_REG, 0);  // Clear the APP CPU entry point
  mark(STEP_CPUS_RESET);
  if (core_id == 0) {
    esp_rom_software_reset_cpu(1);
    esp_rom_software_reset_cpu(0);
  } else {
    esp_rom_software_reset_cpu(0);
    esp_cpu_unstall(0);
    esp_rom_software_reset_cpu(1);
  }
  while (true) {}
}

// Registered ahead of Wi-Fi and of the watchdog feed, so it runs after both, and never returns: it does
// what is left of esp_restart() itself
static void restart_trace_handler() {
  if (esp_ptr_external_ram(esp_cpu_get_sp())) {
    return;  // This copy cannot move the stack out of PSRAM the way IDF's can: leave the restart to IDF
  }
  trace_ram.app_starts = 0;
  trace_rtc.app_starts = 0;
  trace_ram.magic = TRACE_MAGIC;
  trace_rtc.magic = TRACE_MAGIC;
  mark(STEP_ENTERED);
  esp_sync_timekeeping_timers();  // The one handler left after this one
  vTaskSuspendAll();              // As esp_restart() does before esp_restart_noos()
  traced_restart_noos();
}

// Early in startup, before PSRAM init: tells a stop in the ROM or the bootloader from one in the app
ESP_SYSTEM_INIT_FN(restart_trace_app_start, CORE, BIT(0), 2) {
  if (trace_ram.magic == TRACE_MAGIC) {
    trace_ram.app_starts = trace_ram.app_starts + 1;
  }
  if (trace_rtc.magic == TRACE_MAGIC) {
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
    const uint32_t step = in_ram ? trace_ram.step : trace_rtc.step;
    const uint32_t app_starts = in_ram ? trace_ram.app_starts : trace_rtc.app_starts;
    logging.printf("Last restart got to: %s (step %u), app starts since: %u (kept in RAM: %s, RTC: %s)\n",
                   step < STEP_COUNT ? STEP_NAMES[step] : "?", (unsigned)step, (unsigned)app_starts,
                   in_ram ? "yes" : "no", in_rtc ? "yes" : "no");
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
