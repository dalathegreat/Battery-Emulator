#include "async_spi_bus.h"
#include "async_spi_clock.h"

#include "driver/gpio.h"
#include "esp_cpu.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "hal/gpio_ll.h"
#include "esp_private/spi_share_hw_ctrl.h"
#include "esp_rom_gpio.h"
#include "hal/spi_ll.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include "soc/spi_periph.h"

#ifdef ASYNC_SPI_BUS_STATS
volatile uint32_t async_spi_bus_stat_entries;
volatile uint32_t async_spi_bus_stat_cycles;

// Counts the cycles until the end of the scope
struct StatScope {
  uint32_t start = esp_cpu_get_cycle_count();
  ~StatScope() {
    async_spi_bus_stat_cycles = async_spi_bus_stat_cycles + (esp_cpu_get_cycle_count() - start);
    async_spi_bus_stat_entries = async_spi_bus_stat_entries + 1;
  }
};
#define STAT_SCOPE() StatScope stat_scope_
#else
#define STAT_SCOPE()
#endif

// GPIO pin register fields
#define PIN_INT_TYPE_M (7u << 7)
#define PIN_INT_TYPE_NEGEDGE (2u << 7)
#define PIN_INT_ENA_S 13
#define PIN_INT_ENA_M (0x1Fu << PIN_INT_ENA_S)

// INT pins (all buses)
static AsyncSpiDevice* s_int_devs[2 * ASYNC_SPI_BUS_MAX_DEVICES];
static intr_handle_t s_int_handle;
static int s_int_core;
// The NMI source's status for s_int_core (pins 0-31, 32+)
static volatile uint32_t* s_int_status[2];

static void routeOutput(int pin, uint32_t signal) {
  esp_rom_gpio_pad_select_gpio(pin);
  gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
  esp_rom_gpio_connect_out_signal(pin, signal, false, false);
}

AsyncSpiBus* AsyncSpiBus::get(spi_host_device_t host, int sck, int mosi, int miso) {
  static AsyncSpiBus* buses[SOC_SPI_PERIPH_NUM];
  if (host < SPI2_HOST || host >= SOC_SPI_PERIPH_NUM) {
    return nullptr;
  }
  if (buses[host]) {
    return buses[host];
  }
  // Enables and resets the peripheral, and stops anyone else using it
  if (!spicommon_periph_claim(host, "async_spi_bus")) {
    return nullptr;
  }

  spi_dev_t* hw = SPI_LL_GET_HW(host);
  spi_ll_master_init(hw);
  spi_ll_master_set_mode(hw, 0);
  hw->user.val = 0;
  hw->user.usr_mosi = 1;
  hw->user.usr_miso = 1;
  // Full duplex mode, with received bytes replacing sent ones in the data buffer
  hw->user.doutdin = 1;

  const spi_signal_conn_t* sig = &spi_periph_signal[host];
  routeOutput(sck, sig->spiclk_out);
  routeOutput(mosi, sig->spid_out);
  esp_rom_gpio_pad_select_gpio(miso);
  gpio_set_direction((gpio_num_t)miso, GPIO_MODE_INPUT);
  esp_rom_gpio_connect_in_signal(miso, sig->spiq_in, false);

  AsyncSpiBus* b = new AsyncSpiBus();
  b->_hw = hw;
  b->_transfer_lock = xSemaphoreCreateMutex();
  spi_ll_clear_int_stat(hw);
  spi_ll_enable_int(hw);
  if (!b->_transfer_lock || esp_intr_alloc(sig->irq, ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM, isr, b, nullptr) != ESP_OK) {
    if (b->_transfer_lock) {
      vSemaphoreDelete(b->_transfer_lock);
    }
    delete b;
    spicommon_periph_free(host);
    return nullptr;
  }
  buses[host] = b;
  return b;
}

bool AsyncSpiBus::add(AsyncSpiDevice* dev, int cs, uint32_t clock_hz) {
  gpio_set_level((gpio_num_t)cs, 1);  // Idle high before it becomes an output
  gpio_set_direction((gpio_num_t)cs, GPIO_MODE_OUTPUT);
  dev->cs = cs;
  dev->bus = this;
  setClock(dev, clock_hz);
  bool added = false;
  portENTER_CRITICAL(&_mux);
  for (AsyncSpiDevice*& d : _devs) {
    if (!d) {
      d = dev;
      added = true;
      break;
    }
  }
  portEXIT_CRITICAL(&_mux);
  return added;
}

void AsyncSpiBus::remove(AsyncSpiDevice* dev) {
  hold(true);
  for (AsyncSpiDevice*& d : _devs) {
    if (d == dev) {
      d = nullptr;
    }
  }
  hold(false);
}

// Set the SPI clock for a device, derived from the 80MHz APB clock.
void AsyncSpiBus::setClock(AsyncSpiDevice* dev, uint32_t clock_hz) {
  dev->clock_reg = asyncSpiClockReg(APB_CLK_FREQ, clock_hz);  // Gets applied from its next transfer
}

// The bus must be idle when attaching an interrupt to a device.
bool AsyncSpiBus::attachInt(AsyncSpiDevice* dev, int int_pin) {
  if (!s_int_handle) {
    s_int_core = xPortGetCoreID();
#ifdef CONFIG_IDF_TARGET_ESP32
    s_int_status[0] = (volatile uint32_t*)(s_int_core ? GPIO_ACPU_NMI_INT_REG : GPIO_PCPU_NMI_INT_REG);
    s_int_status[1] = (volatile uint32_t*)(s_int_core ? GPIO_ACPU_NMI_INT1_REG : GPIO_PCPU_NMI_INT1_REG);
#else
    s_int_status[0] = (volatile uint32_t*)GPIO_PCPU_NMI_INT_REG;  // Both cores, on ESP32-S3
    s_int_status[1] = (volatile uint32_t*)GPIO_PCPU_NMI_INT1_REG;
#endif
    // Allocate an interrupt, any level is fine.
    if (esp_intr_alloc(ETS_GPIO_NMI_SOURCE, ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM, intIsr, nullptr,
                       &s_int_handle) != ESP_OK) {
      s_int_handle = nullptr;
      return false;
    }
  }
  bool added = false;
  portENTER_CRITICAL(&_mux);
  for (AsyncSpiDevice*& d : s_int_devs) {
    if (!d || d == dev) {
      d = dev;
      added = true;
      break;
    }
  }
  portEXIT_CRITICAL(&_mux);
  if (!added) {
    return false;
  }
  dev->int_pin = int_pin;
  gpio_set_direction((gpio_num_t)int_pin, GPIO_MODE_INPUT);
  gpio_pullup_en((gpio_num_t)int_pin);
#ifdef CONFIG_IDF_TARGET_ESP32
  const uint32_t nmi_ena = s_int_core ? (1u << 1) : (1u << 3);  // APP / PRO CPU NMI
#else
  const uint32_t nmi_ena = 1u << 1;
#endif
  volatile uint32_t* pin_reg = (volatile uint32_t*)(GPIO_PIN0_REG + 4 * int_pin);
  *pin_reg = (*pin_reg & ~(PIN_INT_TYPE_M | PIN_INT_ENA_M)) | PIN_INT_TYPE_NEGEDGE | (nmi_ena << PIN_INT_ENA_S);
  return true;
}

void AsyncSpiBus::detachInt(AsyncSpiDevice* dev) {
  if (dev->int_pin < 0) {
    return;
  }
  volatile uint32_t* pin_reg = (volatile uint32_t*)(GPIO_PIN0_REG + 4 * dev->int_pin);
  *pin_reg = *pin_reg & ~(PIN_INT_TYPE_M | PIN_INT_ENA_M);
  portENTER_CRITICAL(&_mux);
  for (AsyncSpiDevice*& d : s_int_devs) {
    if (d == dev) {
      d = nullptr;
    }
  }
  portEXIT_CRITICAL(&_mux);
  dev->int_pin = -1;
}

// An INT pin was asserted (active-low)
void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::intIsr(void* arg) {
  uint32_t st[2] = {*s_int_status[0], *s_int_status[1]};
  // Clear flags first, so an edge during the runs isn't lost
  REG_WRITE(GPIO_STATUS_W1TC_REG, st[0]);
  REG_WRITE(GPIO_STATUS1_W1TC_REG, st[1]);
  for (AsyncSpiDevice* d : s_int_devs) {
    // Check if this device's INT pin triggered the interrupt
    if (d && (st[d->int_pin >> 5] & (1u << (d->int_pin & 31)))) {
      // Request a step for this device if so
      d->bus->requestStep(d);
    }
  }
}

void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::applyClock(const AsyncSpiDevice* dev) {
  if (dev->clock_reg != _clock_reg) {
    _clock_reg = dev->clock_reg;
    spi_ll_clock_val_t reg = dev->clock_reg;
    spi_ll_master_set_clock_by_reg(_hw, &reg);
  }
}

void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::requestStep(AsyncSpiDevice* dev) {
  portENTER_CRITICAL_SAFE(&_mux);
  dev->pending = true;
  bool start = !_busy && !_holds;
  _busy = _busy || start;
  portEXIT_CRITICAL_SAFE(&_mux);
  if (start) {
    STAT_SCOPE();
    run();
  }
}

// Increment or decrement the hold count - if held, new runs won't be started
// (but an active one will be allowed to finish)
void AsyncSpiBus::hold(bool on) {
  portENTER_CRITICAL(&_mux);
  _holds = on ? _holds + 1 : _holds - 1;
  bool start = !_holds && !_busy;
  _busy = _busy || start;
  portEXIT_CRITICAL(&_mux);
  if (start) {
    STAT_SCOPE();
    run();  // Runs requested meanwhile
  }
  while (on && _busy) {
    vTaskDelay(1);
  }
}

void AsyncSpiBus::transferBlocking(AsyncSpiDevice* dev, uint8_t* buf, size_t len) {
  xSemaphoreTake(_transfer_lock, portMAX_DELAY);
  hold(true);
  applyClock(dev);
  gpio_ll_set_level(&GPIO, dev->cs, 0);
  // The hardware SPI buffer only holds 64 bytes at a time, so we may need to
  // send in chunks.
  for (size_t off = 0; off < len; off += 64) {
    const size_t n = len - off < 64 ? len - off : 64;
    startChunk(buf + off, n);
    // Busy-wait while the transfer is in progress
    while (_hw->cmd.usr) {
    }
    readChunk(buf + off, n);
  }
  gpio_ll_set_level(&GPIO, dev->cs, 1);
  hold(false);
  xSemaphoreGive(_transfer_lock);
}

// Start an SPI transfer for the current device, setting CS low first, returns immediately
void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::startTransfer(const uint8_t* tx, uint8_t* rx, uint8_t len, bool keep_cs) {
  _rx = rx;
  _len = len;
  _keep_cs = keep_cs;
  gpio_ll_set_level(&GPIO, _cur->cs, 0);
  startChunk(tx, len);
}

// Trigger an SPI transfer for a single chunk (of up to 64 bytes), returns immediately
void IRAM_ATTR __attribute__((noinline)) AsyncSpiBus::startChunk(const uint8_t* tx, size_t n) {
  spi_ll_set_mosi_bitlen(_hw, n * 8);
  spi_ll_set_miso_bitlen(_hw, n * 8);
  spi_ll_write_buffer(_hw, tx, n * 8);
  spi_ll_apply_config(_hw);
  spi_ll_user_start(_hw);
}

// Copy the received n bytes from the hardware SPI buffer into rx.
void IRAM_ATTR __attribute__((noinline)) AsyncSpiBus::readChunk(uint8_t* rx, size_t n) {
  spi_ll_read_buffer(_hw, rx, n * 8);
}

// With _busy set: step the current run, or start the next one.
void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::run() {
  for (;;) {
    if (_cur && _cur->step(_cur)) {
      // A transaction is active and we've stepped it, the rest happens in the ISR.
      return;
    }
    // Find the next device wanting a go
    AsyncSpiDevice* next = nullptr;
    portENTER_CRITICAL_SAFE(&_mux);
    for (int k = 1; k <= ASYNC_SPI_BUS_MAX_DEVICES && !_holds && !next; k++) {
      uint8_t i = (_last + k) % ASYNC_SPI_BUS_MAX_DEVICES;
      if (_devs[i] && _devs[i]->pending) {
        // Found one
        next = _devs[i];
        next->pending = false;
        _last = i;
      }
    }
    // It is crucial that we update busy and _cur whilst still inside the
    // critical section. Otherwise the _cur write could conflict with one from
    // an interrupt and get overwritten with nullptr, and run() never gets
    // called again, and _busy never gets cleared.
    _busy = next != nullptr;
    _cur = next;
    portEXIT_CRITICAL_SAFE(&_mux);
    if (!next) {
      return;
    }
    next->state = 0;
    applyClock(next);
  }
}

void IRAM_ATTR ASYNC_SPI_BUS_HOT AsyncSpiBus::isr(void* arg) {
  STAT_SCOPE();
  AsyncSpiBus* b = static_cast<AsyncSpiBus*>(arg);
  if (!spi_ll_usr_is_done(b->_hw)) {
    return;
  }
  spi_ll_clear_int_stat(b->_hw);
  if (!b->_cur) {
    return;  // A blocking transfer (made with the bus held)
  }
  if (!b->_keep_cs) {
    gpio_ll_set_level(&GPIO, b->_cur->cs, 1);
  }
  if (b->_rx) {
    b->readChunk(b->_rx, b->_len);
  }
  b->run();
}
