#include "mcp2518fd_lite.h"
#include <Arduino.h>

#include "src/devboard/utils/logging.h"

// MCP2518FD SPI instructions (top nibble of the 16-bit command word)
#define INSTRUCTION_WRITE 0x2000
#define INSTRUCTION_READ 0x3000

// MCP2518FD register addresses
#define REG_CiCON 0x000
#define REG_CiNBTCFG 0x004
#define REG_CiDBTCFG 0x008
#define REG_CiTDC 0x00C
#define REG_CiTBC 0x010
#define REG_CiTSCON 0x014
#define REG_CiINT 0x01C
#define REG_CiTEFCON 0x040
#define REG_CiTXQCON 0x050
#define REG_OSC 0xE00
#define REG_IOCON_24_31 0xE07

// FIFO registers (index 1..31)
#define REG_CiFIFOCON(n) (0x05C + 12 * ((n) - 1))

// Filter registers
#define REG_CiFLTCON(n) (0x1D0 + (n))
#define REG_CiFLTOBJ(n) (0x1F0 + 8 * (n))
#define REG_CiMASK(n) (0x1F4 + 8 * (n))

#define CAN_RAM_BASE 0x400

// Controller FIFO assignment. Together the two FIFOs must fit in the 2048
// byte controller RAM: (RX + TX) * 72 <= 2048.
#define CONTROLLER_RX_FIFO_INDEX 1
#define CONTROLLER_RX_FIFO_SIZE 24
#define CONTROLLER_TX_FIFO_INDEX 2
#define CONTROLLER_TX_FIFO_SIZE 4

// CiCON REQOP (write) / OPMOD (read) modes
#define MODE_NORMAL_FD 0
#define MODE_INTERNAL_LOOPBACK 2
#define MODE_CONFIGURATION 4

// PLSIZE encoding for a 64 byte payload
#define PAYLOAD_SIZE_64 7

// We store the pending TX/RX frames in a ring buffer with a custom header. The
// records are variable length, so we can store more 8-byte frames than
// full-length 64-byte FD ones, but this makes efficient use of the RAM.
struct RingHeader {
  uint32_t id;  // CAN ID (native little-endian word)
  uint8_t len;  // payload length in bytes (0-64)
  uint8_t flags;  // bit 0 = ext, bit 1 = fd
  uint8_t reserved[2];  // padding so the payload stays 32-bit aligned
};
static_assert(sizeof(RingHeader) == 8, "RingHeader must be two 32-bit words");
struct RingRecord {
  RingHeader hdr;
  uint8_t payload[64];
};
#define RING_HEADER_LEN sizeof(RingHeader)
#define RING_FLAG_EXT 0x01
#define RING_FLAG_FD 0x02

static inline uint32_t readLE32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void writeLE32(uint8_t* p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

// Payload bytes for each DLC code
static const uint8_t kDlcToBytes[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

// Smallest DLC code whose payload holds length bytes, so lengths between the
// CAN FD sizes are rounded up (and zero padded by the caller).
static inline uint32_t lengthCodeForLength(uint8_t length) {
  if (length <= 8) {
    return length;
  }
  if (length <= 24) {
    return (length + 3) / 4 + 6;  // 12, 16, 20, 24 -> 9..12
  }
  return length <= 32 ? 13 : (length <= 48 ? 14 : 15);
}

// Bit timing for an 80% sample point, SJW = TSEG2, and one prescaler BRP = 2^k
// - 1 for both phases (equal BRPs as CiA 601-3 recommends, powers of two divide
// 20/40 MHz exactly at all the standard bitrates).

// Tq per bit at BRP = 2^k - 1, rounded.
static uint32_t tqPerBit(uint32_t sysClock, uint32_t bitrate, uint32_t k) {
  return ((sysClock >> k) + bitrate / 2) / bitrate;
}

// Smallest k at which a bit is at most maxTq Tq.
static uint32_t prescalerShift(uint32_t sysClock, uint32_t bitrate, uint32_t maxTq) {
  uint32_t k = 0;
  while (tqPerBit(sysClock, bitrate, k) > maxTq) {
    k++;
  }
  return k;
}

// CiNBTCFG/CiDBTCFG image for an n Tq bit: BRP[31:24] TSEG1[23:16]
// TSEG2[14:8] SJW[6:0], each stored as value-1. CiDBTCFG's fields start at
// the same bits but are narrower (TSEG1[20:16] TSEG2[11:8] SJW[3:0]); the
// 40 Tq data limit keeps the values within them.
static uint32_t encodeBitTiming(uint32_t n, uint32_t k) {
  const uint32_t tseg2 = (n * 205 - 614) >> 10;  // round(n/5) - 1 (20%)
  const uint32_t tseg1 = n - 3 - tseg2;          // n - SYNC - TSEG2 - 1
  return ((((1u << k) - 1) << 8 | tseg1) << 8 | tseg2) << 8 | tseg2;
}

// Fills regs with CiNBTCFG, CiDBTCFG and CiTDC, which are contiguous in chip
// address order (0x004, 0x008, 0x00C). sysClock: 20000000 or 40000000.
// Bitrates in bit/s.
static bool calculateBitTiming(uint32_t sysClock, uint32_t arbBitrate, uint32_t dataBitrate, uint32_t regs[3]) {
  // At 80%: nominal TSEG1 <= 256 Tq -> 320 Tq/bit, data TSEG1 <= 32 -> 40.
  uint32_t k = prescalerShift(sysClock, arbBitrate, 320);
  const uint32_t kData = prescalerShift(sysClock, dataBitrate, 40);
  if (kData > k) {
    k = kData;
  }
  const uint32_t nArb = tqPerBit(sysClock, arbBitrate, k);
  const uint32_t nData = tqPerBit(sysClock, dataBitrate, k);
#if MCP2518FD_LITE_VALIDATE_BITRATE
  // BRP is 8 bits, and below 4 Tq per bit the segments don't fit
  if (k > 8 || nArb < 4 || nData < 4) {
    return false;
  }
#endif
  regs[0] = encodeBitTiming(nArb, k);
  regs[1] = encodeBitTiming(nData, k);
  uint32_t tdc = 1u << 25;  // EDGFLTEN: edge filtering during bus integration
  if (k == 0) {
    // Auto TDC (TDCMOD = 10, bits 17:16), TDCO = data sample point in
    // SYSCLKs = SYNC + TSEG1 = TSEG1 field + 2. With BRP = 0, regs[1] >> 16
    // is just the TSEG1 field.
    tdc |= 2u << 16 | ((regs[1] >> 16) + 2) << 8;
  }
  regs[2] = tdc;
  return true;
}

MCP2518FD_Lite::MCP2518FD_Lite(SPIClass& spi, uint8_t cs, uint8_t int_pin)
    : _spi(spi), _cs(cs), _int_pin(int_pin), _spi_settings(1000000, MSBFIRST, SPI_MODE0) {
  _next_speed = {0, 0, 0};
  ringbuf_init(&_tx_ring, _tx_buf, sizeof(_tx_buf));
  ringbuf_init(&_rx_ring, _rx_buf, sizeof(_rx_buf));
}

MCP2518FD_Lite::~MCP2518FD_Lite() {
  if (_can_task_handle) {
    vTaskDelete(_can_task_handle);
    _can_task_handle = nullptr;
  }
  detachInterrupt(digitalPinToInterrupt(_int_pin));
}

// Use a low SPI bitrate for initial configuration
static const SPISettings configSpiSettings(1000000, MSBFIRST, SPI_MODE0);

uint32_t MCP2518FD_Lite::autodetectOscillatorFrequency() {
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  _spi_settings = configSpiSettings;
  if (!reset()) {
    return 0;
  }
  return detectOscillator();
}

bool MCP2518FD_Lite::begin(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div, bool loopback,
                           bool skip_task_start) {
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  pinMode(_int_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_int_pin), mcp2518fd_isr_handler, this, FALLING);

  _spi_settings = configSpiSettings;
  _clko_div = clko_div & 0x03;
  _loopback = loopback;
  _paused = false;
  _pause_requested = false;
  _speed_change_pending = false;
  _rx_overflow = false;
  _errors = false;

  if (!reset()) {
    return false;
  }

  uint32_t f_osc = speed.f_osc;
  if (f_osc == 0) {
    f_osc = detectOscillator();
    if (f_osc == 0) {
      return false;
    }
  }
  _f_osc = f_osc;

  // Configure the oscillator (no PLL, no divide by 2) and the CLKO divider
  writeReg8(REG_OSC, (uint8_t)(_clko_div << 5));

  // Wait for OSCRDY (bit 10 of the OSC register)
  bool ready = false;
  for (int i = 0; i < 50; i++) {
    if (readReg8(REG_OSC + 1) & (1 << 2)) {
      ready = true;
      break;
    }
    delay(1);
  }
  if (!ready) {
    return false;
  }

  // Switch to the operational SPI clock (0.4 x SYSCLK, ie. 16 MHz at 40 MHz
  // and 8 MHz at 20 MHz)
  _spi_settings = SPISettings((_f_osc * 2) / 5, MSBFIRST, SPI_MODE0);

  // Simple RAM readback to verify the SPI connection
  writeReg32(CAN_RAM_BASE, 0xA5A5A5A5UL);
  if (readReg32(CAN_RAM_BASE) != 0xA5A5A5A5UL) {
    return false;
  }

  // Pin control (keep the default push/pull TXCAN and INT pins)
  writeReg8(REG_IOCON_24_31, 0x03);
  // CiCON: PXEDIS | ISO CRC enabled
  writeReg8(REG_CiCON, 0x60);

  // Disable the Transmit Event FIFO and the Transmit Queue
  writeReg32(REG_CiTEFCON, 0x00000000);
  writeReg32(REG_CiTXQCON, 0x00000000);
  // Enable RTXAT so retransmission attempts are controlled per FIFO
  writeReg8(REG_CiCON + 2, 0x01);

  // RX FIFO: 64 byte payload, configured depth, not-empty interrupt
  writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 3, (PAYLOAD_SIZE_64 << 5) | (CONTROLLER_RX_FIFO_SIZE - 1));
  writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 2, 0x00);
  writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX), 0x01);  // TFNRFNIE

  // TX FIFO: 64 byte payload, configured depth, unlimited retransmissions. We
  // don't enable the TX FIFO not-full interrupt yet, only later on if we
  // actually manage to fill the TX FIFO.
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 2, (2 << 5) | 0x00);  // TXAT = unlimited
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 3, (PAYLOAD_SIZE_64 << 5) | (CONTROLLER_TX_FIFO_SIZE - 1));
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX), 0x80);  // TXEN

  // Drive INT from the FIFO interrupts enabled above (CiINT RXIE | TXIE)
  writeReg8(REG_CiINT + 2, 0x03);

  // Filter 0: accept everything and route to the RX FIFO
  writeReg32(REG_CiFLTOBJ(0), 0x00000000);
  writeReg32(REG_CiMASK(0), 0x00000000);
  writeReg8(REG_CiFLTCON(0), 0x80 | CONTROLLER_RX_FIFO_INDEX);

  // Calculate and apply bit timing
  if (!applySpeed(speed)) {
    return false;
  }

  // Finally leave configuration mode
  if (!requestMode(loopback ? MODE_INTERNAL_LOOPBACK : MODE_NORMAL_FD)) {
    return false;
  }

  if (!skip_task_start) {
    // Start the background task, and wake it once for any frames that
    // arrived (pulling INT low) before the ISR had a task to notify.
    if (xTaskCreate(canTask, "MCP2518FD_Lite", MCP2518FD_LITE_TASK_STACK_SIZE, this, MCP2518FD_LITE_TASK_PRIORITY,
                    &_can_task_handle) != pdPASS) {
      return false;
    }
    xTaskNotifyGive(_can_task_handle);
  }

  return true;
}

bool MCP2518FD_Lite::sendFrame(const MCP2518FD_Lite_Frame& msg) {
  const uint8_t length = msg.dlc > 64 ? 64 : msg.dlc;

  // Prepare the frame for the ringbuffer. We do a copy followed by a single
  // atomic write, to avoid leaving incomplete frames in the ring buffer.
  RingRecord record;
  record.hdr.id = msg.id;
  record.hdr.len = length;
  record.hdr.flags = (msg.ext ? RING_FLAG_EXT : 0) | (msg.fd ? RING_FLAG_FD : 0);
  record.hdr.reserved[0] = 0;
  record.hdr.reserved[1] = 0;
  if (length > 0) {
    memcpy(record.payload, msg.data, length);
  }

#if MCP2518FD_LITE_TX_LOCK
  portENTER_CRITICAL(&_tx_lock);
#endif
  const size_t written =
      ringbuf_write(&_tx_ring, reinterpret_cast<const uint8_t*>(&record), sizeof(RingHeader) + length);
#if MCP2518FD_LITE_TX_LOCK
  portEXIT_CRITICAL(&_tx_lock);
#endif
  if (written == 0) {
    return false;
  }

  // Wake the background task so it can move the frame to the controller.
  if (_can_task_handle) {
    xTaskNotifyGive(_can_task_handle);
  }
  return true;
}

bool MCP2518FD_Lite::receiveFrame(MCP2518FD_Lite_Frame& msg) {
  if (_rx_overflow) {
    DEBUG_PRINTF("MCP2518FD RX ring overflow!\n");
    _rx_overflow = false;
  }

  // The ring only ever holds complete records (each sendFrame is a single
  // atomic write), so if at least a header is available the whole first
  // frame is guaranteed to be present.
  const size_t used = sizeof(_rx_buf) - ringbuf_free_space(&_rx_ring);
  if (used < RING_HEADER_LEN) {
    return false;
  }

  RingHeader hdr;
  if (ringbuf_read(&_rx_ring, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) != sizeof(hdr)) {
    return false;
  }

  const uint8_t length = hdr.len;
  const uint8_t flags = hdr.flags;
  if (length > 64 || (flags & ~(RING_FLAG_EXT | RING_FLAG_FD)) != 0) {
    _errors = true;
    return false;
  }

  msg.id = hdr.id;
  msg.dlc = length;
  msg.ext = (flags & RING_FLAG_EXT) != 0;
  msg.fd = (flags & RING_FLAG_FD) != 0;
  if (length > 0 && ringbuf_read(&_rx_ring, msg.data, length) != length) {
    _errors = true;
    return false;
  }
  return true;
}

void MCP2518FD_Lite::changeSpeed(const MCP2518FD_Lite_Speed& new_speed) {
  _next_speed = new_speed;
  _speed_change_pending = true;
  // Wake the task to enact the speed change
  if (_can_task_handle) {
    xTaskNotifyGive(_can_task_handle);
  }
}

void MCP2518FD_Lite::pause(bool paused) {
  _pause_requested = paused;
  // Wake the task to apply the pause state
  if (_can_task_handle) {
    xTaskNotifyGive(_can_task_handle);
  }
}

void MCP2518FD_Lite::canTask(void* pvParameters) {
  MCP2518FD_Lite* self = static_cast<MCP2518FD_Lite*>(pvParameters);
  // Whether the TX FIFO not-full interrupt is enabled
  bool tx_ie = false;

  while (true) {
    // Sleep until sendFrame/pause/changeSpeed or the ISR wakes us up. We
    // also wake after a timeout in case we've missed an interrupt.
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MCP2518FD_LITE_POLL_TIMEOUT_MS));

    // Pause/unpause if requested. Configuration mode stops ACKing, which
    // is how we pause communication.
    const uint8_t run_mode = self->_loopback ? MODE_INTERNAL_LOOPBACK : MODE_NORMAL_FD;
    if (self->_paused && !self->_pause_requested) {
      self->requestMode(run_mode);
      self->_paused = false;
    } else if (!self->_paused && self->_pause_requested) {
      self->requestMode(MODE_CONFIGURATION);
      self->_paused = true;
      continue;
    } else if (self->_paused) {
      continue;
    }

    // Apply a pending speed change in configuration mode.
    if (self->_speed_change_pending) {
      self->requestMode(MODE_CONFIGURATION);
      self->applySpeed(self->_next_speed);
      self->requestMode(self->_loopback ? MODE_INTERNAL_LOOPBACK : MODE_NORMAL_FD);
      self->_speed_change_pending = false;
    }

    // Check controller error flags.
    const uint8_t err_flags = self->readReg8(REG_CiINT + 1);
    if (err_flags & (1u << 4)) {  // SERRIF: system error
      self->_errors = true;
      self->writeReg8(REG_CiINT + 1, (uint8_t) ~(1u << 4));
    }
    if (err_flags & (1u << 5)) {  // CERRIF: CAN bus error
      self->_errors = true;
      self->writeReg8(REG_CiINT + 1, (uint8_t) ~(1u << 5));
    }

    // TX/RX between the controller FIFOs and the ring buffers until there's
    // nothing left to do.
    bool work_done;
    do {
      work_done = false;

      // RECEIVE

      // Read FIFOSTA + FIFOUA in one transaction.
      uint8_t regs[8];
      self->readBytes(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 4, regs, sizeof(regs));
      const uint32_t sta = readLE32(regs);
      const uint32_t ua = readLE32(regs + 4);

      if (sta & (1u << 3)) {  // RXOVIF: the controller dropped a frame
        self->_rx_overflow = true;
        self->writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 4, (uint8_t) ~(1u << 3));
      }

      if (sta & 0x01) {  // RX FIFO not empty
        const uint16_t ram_addr = CAN_RAM_BASE + (uint16_t)ua;

#if MCP2518FD_READ_FULL_FRAMES
        // Read the full frame (header + payload) in a single SPI transaction.
        uint8_t obj[8 + 64];
        self->readBytes(ram_addr, obj, sizeof(obj));
#else
        // Read in two transactions, starting with the header plus the first 8 payload bytes.
        uint8_t obj[16];
        self->readBytes(ram_addr, obj, sizeof(obj));
#endif
        const uint32_t raw_id = readLE32(obj);
        const uint32_t flags = readLE32(obj + 4);
        const uint8_t length = kDlcToBytes[flags & 0x0F];

        RingRecord record;
        const bool ext = (flags & (1u << 4)) != 0;  // IDE bit
        record.hdr.flags = (ext ? RING_FLAG_EXT : 0) | ((flags & (1u << 7)) ? RING_FLAG_FD : 0);  // FDF bit
        if (ext) {
          record.hdr.id = ((raw_id >> 11) & 0x3FFFF) | ((raw_id & 0x7FF) << 18);
        } else {
          record.hdr.id = raw_id & 0x7FF;
        }
        record.hdr.len = length;
        record.hdr.reserved[0] = 0;
        record.hdr.reserved[1] = 0;

        if (length > 0) {
#if MCP2518FD_READ_FULL_FRAMES
          memcpy(record.payload, obj + 8, length);
#else
          const uint8_t first = length < 8 ? length : 8;
          memcpy(record.payload, obj + 8, first);
          if (length > 8) {
            // Frame was longer than 8 bytes, so read the rest in a second
            // transaction.
            const uint8_t words = (length + 3) / 4;
            self->readBytes(ram_addr + 16, record.payload + 8, words * 4 - 8);
          }
#endif
        }

        if (ringbuf_write(&self->_rx_ring, reinterpret_cast<const uint8_t*>(&record),
                          sizeof(RingHeader) + length) == 0) {
          // Receive ring buffer is full, drop the frame and record that we
          // overflowed.
          self->_rx_overflow = true;
        }

        // Set UINC to release the FIFO slot.
        self->writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 1, 0x01);
        work_done = true;
      }

      // TRANSMIT

      // Check there's something in the ring buffer to send
      const size_t tx_used = sizeof(self->_tx_buf) - ringbuf_free_space(&self->_tx_ring);
      if (tx_used >= sizeof(RingHeader)) {
        // Read FIFOSTA + FIFOUA in one transaction.
        uint8_t tx_regs[8];
        self->readBytes(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 4, tx_regs, sizeof(tx_regs));

        if (readLE32(tx_regs) & 0x01) {  // TX FIFO not full
          const uint16_t tx_ram_addr = CAN_RAM_BASE + (uint16_t)readLE32(tx_regs + 4);

          // Read the frame header
          RingHeader hdr;
          uint8_t payload[64];
          bool popped =
              ringbuf_read(&self->_tx_ring, reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) == sizeof(hdr);
          if (popped && (hdr.len > 64 || (hdr.flags & ~(RING_FLAG_EXT | RING_FLAG_FD)) != 0)) {
            popped = false;
          }
          // And the rest of the frame
          if (popped && hdr.len > 0) {
            popped = ringbuf_read(&self->_tx_ring, payload, hdr.len) == hdr.len;
          }
          if (!popped) {
            self->_errors = true;
          } else {
            const uint8_t length = hdr.len;
            const bool ext = (hdr.flags & RING_FLAG_EXT) != 0;
            const uint32_t dlc = lengthCodeForLength(length);
            // Whole words of the payload as sent, including the padding
            const uint8_t words = (kDlcToBytes[dlc] + 3) / 4;

            // Pack the ID the way the controller expects it.
            uint32_t idf = hdr.id;
            if (ext) {
              idf = ((hdr.id >> 18) & 0x7FF) | ((hdr.id & 0x3FFFF) << 11);
            }

            uint32_t tx_flags = dlc;
            if (ext) {
              tx_flags |= 1 << 4;  // IDE
            }
            if (hdr.flags & RING_FLAG_FD) {
              tx_flags |= 1 << 7;  // FDF
              tx_flags |= 1 << 6;  // BRS
            }

            // Write the header + payload in a single transaction. We write in
            // 32-bit words even if the payload is not a multiple of 4 bytes,
            // hence the zero initialization.
            uint8_t object[8 + 64] = {0};
            writeLE32(object, idf);
            writeLE32(object + 4, tx_flags);
            if (length > 0) {
              memcpy(object + 8, payload, length);
            }
            self->writeBytes(tx_ram_addr, object, 8 + words * 4);

            // Set UINC to advance the FIFO and TXREQ to transmit.
            self->writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 1, 0x03);
            work_done = true;
          }
        } else if (!tx_ie) {
          // TX FIFO is full so we can't send any more frames right now. Enable
          // the TX not-full interrupt so we'll be notified when a slot frees.
          self->writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX), 0x81);  // TXEN | TFNRFNIE
          tx_ie = true;
        }
      } else if (tx_ie) {
        // We have nothing left to send, but the TX not-full interrupt is still
        // enabled - disable it.
        self->writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX), 0x80);  // TXEN
        tx_ie = false;
        work_done = true;
      }
    } while (work_done);
  }
}

// ISR called when the MCP2518FD signals an interrupt via the INT pin.
void IRAM_ATTR MCP2518FD_Lite::mcp2518fd_isr_handler(void* arg) {
  MCP2518FD_Lite* instance = static_cast<MCP2518FD_Lite*>(arg);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Notify task that there's an interrupt to handle
  if (instance->_can_task_handle) {
    vTaskNotifyGiveFromISR(instance->_can_task_handle, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

// --- Internal SPI helpers ---

void MCP2518FD_Lite::spiTransfer(uint8_t* buf, size_t len) {
  _spi.beginTransaction(_spi_settings);
  digitalWrite(_cs, LOW);
  _spi.transfer(buf, len);
  digitalWrite(_cs, HIGH);
  _spi.endTransaction();
}

void MCP2518FD_Lite::readBytes(uint16_t address, uint8_t* dest, size_t nbytes) {
  uint8_t buf[74];
  const uint16_t cmd = INSTRUCTION_READ | (address & 0x0FFF);
  buf[0] = (uint8_t)(cmd >> 8);
  buf[1] = (uint8_t)cmd;
  // transfer() is in-place, so the reply is put in the same buffer
  _spi.beginTransaction(_spi_settings);
  digitalWrite(_cs, LOW);
  _spi.transfer(buf, nbytes + 2);
  digitalWrite(_cs, HIGH);
  _spi.endTransaction();
  memcpy(dest, buf + 2, nbytes);
}

void MCP2518FD_Lite::writeBytes(uint16_t address, const uint8_t* src, size_t nbytes) {
  uint8_t buf[74];
  const uint16_t cmd = INSTRUCTION_WRITE | (address & 0x0FFF);
  buf[0] = (uint8_t)(cmd >> 8);
  buf[1] = (uint8_t)cmd;
  memcpy(buf + 2, src, nbytes);
  spiTransfer(buf, nbytes + 2);
}

uint8_t MCP2518FD_Lite::readReg8(uint16_t address) {
  uint8_t value = 0;
  readBytes(address, &value, 1);
  return value;
}

uint32_t MCP2518FD_Lite::readReg32(uint16_t address) {
  uint8_t buf[4];
  readBytes(address, buf, sizeof(buf));
  return readLE32(buf);
}

void MCP2518FD_Lite::writeReg8(uint16_t address, uint8_t value) {
  writeBytes(address, &value, 1);
}

void MCP2518FD_Lite::writeReg32(uint16_t address, uint32_t value) {
  uint8_t buf[4];
  writeLE32(buf, value);
  writeBytes(address, buf, sizeof(buf));
}

// --- Mode handling ---

bool MCP2518FD_Lite::waitForMode(uint8_t mode, uint32_t timeout_ms) {
  for (uint32_t i = 0; i <= timeout_ms; i++) {
    if (((readReg8(REG_CiCON + 2) >> 5) & 0x07) == mode) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool MCP2518FD_Lite::requestMode(uint8_t mode) {
  writeReg8(REG_CiCON + 3, mode);
  return waitForMode(mode, 10);
}

bool MCP2518FD_Lite::reset() {
  // Request configuration mode first, then issue the SPI reset instruction
  writeReg8(REG_CiCON + 3, MODE_CONFIGURATION);
  waitForMode(MODE_CONFIGURATION, 10);

  uint8_t cmd[2] = {0x00, 0x00};
  spiTransfer(cmd, sizeof(cmd));
  delay(5);

  // The reset leaves the controller in configuration mode
  writeReg8(REG_CiCON + 3, MODE_CONFIGURATION);
  return waitForMode(MODE_CONFIGURATION, 10);
}

uint32_t MCP2518FD_Lite::detectOscillator() {
  // Enable the free running Time Base Counter (CiTSCON.TBCEN is bit 16)
  writeReg8(REG_CiTSCON + 2, 0x01);
  const uint32_t t1 = micros();
  const uint32_t c1 = readReg32(REG_CiTBC);
  delay(10);
  const uint32_t t2 = micros();
  const uint32_t c2 = readReg32(REG_CiTBC);
  writeReg8(REG_CiTSCON + 2, 0x00);

  const uint32_t elapsed = t2 - t1;
  if (elapsed == 0) {
    return 0;
  }
  // Frequency in 0.1MHz units
  const uint32_t freq_times_10 = (uint32_t)(((uint64_t)(c2 - c1) * 10) / elapsed);
  if (freq_times_10 < 50) {
    return 0;
  }
  return (freq_times_10 > 300) ? 40000000UL : 20000000UL;
}

bool MCP2518FD_Lite::applySpeed(const MCP2518FD_Lite_Speed& speed) {
  const uint32_t f_osc = _f_osc != 0 ? _f_osc : speed.f_osc;
  if (f_osc == 0 || speed.nominal_bitrate == 0 || speed.data_bitrate == 0) {
    return false;
  }
  uint32_t regs[3];
  if (!calculateBitTiming(f_osc, speed.nominal_bitrate, speed.data_bitrate, regs)) {
    return false;
  }
  // The three registers are contiguous, so they go out as one 12-byte
  // little-endian write.
  writeBytes(REG_CiNBTCFG, reinterpret_cast<const uint8_t*>(regs), sizeof(regs));
  return true;
}
