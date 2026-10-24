#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "driver/spi_common.h"  // spi_host_device_t
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "soc/spi_struct.h"

/* AsyncSpiBus - SPI controller wrapper for interrupt-driven SPI device drivers.

One singleton instance per SPI host, which gets claimed and set up on first use
(when `get()` is called). Multiple devices can share the same bus instance (with
their own CS and INT pins).

The interrupts are IRAM-safe, so devices keep being serviced while the flash
cache is disabled (eg, during flash erases). Everything they touch must be in
IRAM/DRAM - step functions must be IRAM_ATTR, and must not access flash (no
const data without DRAM_ATTR, or switch jump tables).

*/

#define ASYNC_SPI_BUS_MAX_DEVICES 4

// Certain hot functions get greater optimization than the default.
#define ASYNC_SPI_BUS_HOT __attribute__((optimize("O2")))

// Optional counters for measuring CPU use (build with -DASYNC_SPI_BUS_STATS).
#ifdef ASYNC_SPI_BUS_STATS
extern volatile uint32_t async_spi_bus_stat_entries;
extern volatile uint32_t async_spi_bus_stat_cycles;
#endif

class AsyncSpiBus;

/* AsyncSpiDevice - Represents a device on an AsyncSpiBus. 

Should be subclassed by SPI device drivers.

*/

class AsyncSpiDevice {
 public:
  // Perform a single step of the device's operation. Each step is usually
  // triggered by an interrupt in response to something (eg, tx or rx
  // completion). This is a function pointer (to an IRAM_ATTR function) rather
  // than a virtual method, as vtables live in flash.
  typedef bool (*StepFn)(AsyncSpiDevice* dev);
  explicit AsyncSpiDevice(StepFn step) : step(step) {}
  StepFn const step;

  // Devices are always allocated in internal RAM, as external RAM is also
  // inaccessible while the flash cache is disabled.
  static void* operator new(size_t size) {
    void* p = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!p) {
      abort();
    }
    return p;
  }
  static void operator delete(void* p) { heap_caps_free(p); }

  AsyncSpiBus* bus = nullptr;
  uint8_t cs = 0;
  // The current state of the device's operation (which `step()` advances through)
  uint8_t state = 0;
  // Wants to be stepped immediately (still has data waiting to process)
  volatile bool pending = false;
  int8_t int_pin = -1;
  uint32_t clock_reg = 0;
};

class AsyncSpiBus {
 public:
  // Returns the singleton instance for the given SPI host, or nullptr if it
  // can't be claimed.
  static AsyncSpiBus* get(spi_host_device_t host, int sck, int mosi, int miso);

  // Add a device to the bus
  bool add(AsyncSpiDevice* dev, int cs, uint32_t clock_hz);
  // Remove a device from the bus
  void remove(AsyncSpiDevice* dev);
  // Set the SPI clock for a device on the bus (can vary between devices)
  void setClock(AsyncSpiDevice* dev, uint32_t clock_hz);

  // Attach an interrupt handler for the device's INT pin (active-low).
  bool attachInt(AsyncSpiDevice* dev, int int_pin);
  void detachInt(AsyncSpiDevice* dev);

  // Request a step for `dev`.
  void requestStep(AsyncSpiDevice* dev);

  // Hold stops new runs and waits for the current one to end. Can be called
  // multiple times - stepping resumes when every hold(true) has been undone.
  void hold(bool on);

  // Perform an in-place transfer to/from buf to the device, waiting for it to
  // complete. Useful for configuring the device, regular transfers should be
  // asynchronous.
  void transferBlocking(AsyncSpiDevice* dev, uint8_t* buf, size_t len);

  // Start an asynchronous transfer on the current device (max 64 bytes). Copies
  // tx before returning, rx is written in the interrupt. If keep_cs is true, CS
  // stays asserted for the next transfer.
  void startTransfer(const uint8_t* tx, uint8_t* rx, uint8_t len, bool keep_cs = false);

 private:
  spi_dev_t* _hw = nullptr;
  AsyncSpiDevice* _devs[ASYNC_SPI_BUS_MAX_DEVICES] = {};
  // Currently active device
  AsyncSpiDevice* _cur = nullptr;
  // Previously active device index (so we can pick the next round-robin)
  uint8_t _last = 0;
  // Current transfer buffer and length
  uint8_t* _rx = nullptr;
  uint8_t _len = 0;
  // Whether to keep the CS line asserted after the current transfer
  bool _keep_cs = false;
  uint32_t _clock_reg = 0;
  // A run has started or is active
  volatile bool _busy = false;
  // Number of active holds
  volatile uint8_t _holds = 0;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  // Lock for transferBlocking calls
  SemaphoreHandle_t _transfer_lock = nullptr;

  void applyClock(const AsyncSpiDevice* dev);
  void startChunk(const uint8_t* tx, size_t n);
  void readChunk(uint8_t* rx, size_t n);
  void run();
  static void isr(void* arg);
  static void intIsr(void* arg);
};
