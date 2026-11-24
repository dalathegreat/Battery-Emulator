#include "restart_quiesce.h"

#include <Arduino.h>  // psramFound()
#include "../utils/logging.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

/* esp_restart() ends in esp_restart_noos(), which arms the RTC watchdog, then resets and stalls the
   other CPU wherever it happens to be, and only then disables the caches. If that CPU had a PSRAM
   access in flight, the restart can hang right there. About a second later the RTC watchdog resets
   the whole chip, RTC included: the board comes back as RESET_WDT instead of RESET_SW, and whatever
   was kept in the RTC domain is gone. The restart runs in core_loop on core 1, so the CPU caught is
   core 0, the one running Wi-Fi, lwIP, AsyncTCP and MQTT, whose bigger buffers live in PSRAM.

   So as the last step before esp_restart_noos(), that CPU is handed to a task that runs from
   internal RAM with its interrupts off. When the reset reaches it, nothing is in flight.

   A task rather than an IPC ISR stall: a task only gets the CPU between critical sections, so the
   parked CPU holds no spinlock the restarting one could then wait on. Should a task there have been
   holding a mutex that the timekeeping shutdown handler still takes, the interrupt watchdog ends it
   as RESET_INT_WDT a fraction of a second later, rather than a hang. */

static constexpr uint32_t PARK_STACK_BYTES = 2048;  // Allocated at restart only
static constexpr int64_t PARK_WAIT_US = 20000;      // Past this, the restart goes ahead as it always did

static volatile bool other_cpu_parked = false;

static void IRAM_ATTR park_task(void*) {
  portDISABLE_INTERRUPTS();
  other_cpu_parked = true;
  while (true) {}
}

static void park_other_cpu() {
  const BaseType_t other_cpu = (xPortGetCoreID() == 0) ? 1 : 0;
  /* Stack and TCB explicitly in internal RAM: this build lets task stacks live in PSRAM, which is
     exactly what the parked CPU must not touch. Nothing logs from here on: a task on the parked CPU
     may hold the log lock. */
  if (xTaskCreatePinnedToCoreWithCaps(park_task, "restart_park", PARK_STACK_BYTES, nullptr, configMAX_PRIORITIES - 1,
                                      nullptr, other_cpu, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS) {
    return;
  }
  const int64_t deadline = esp_timer_get_time() + PARK_WAIT_US;
  while (!other_cpu_parked && esp_timer_get_time() < deadline) {}
}

void register_restart_quiesce() {
#if !CONFIG_FREERTOS_UNICORE
  if (!psramFound()) {
    return;  // Nothing in PSRAM to be caught in the middle of
  }
  if (esp_register_shutdown_handler(park_other_cpu) != ESP_OK) {
    logging.println("Could not register the restart quiesce handler, restarts run with both CPUs live");
  }
#endif
}
