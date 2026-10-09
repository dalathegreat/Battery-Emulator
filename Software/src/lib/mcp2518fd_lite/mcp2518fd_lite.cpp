#include "mcp2518fd_lite.h"
#include <Arduino.h>

#include "driver/gpio.h"
#include "hal/gpio_ll.h"
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

// FIFO status/user address registers
#define REG_CiFIFOSTA(n) (REG_CiFIFOCON(n) + 4)
#define FIFOCON_TXEN 0x80        // Byte 0
#define FIFOCON_TFNRFNIE 0x01    // Byte 0: interrupt while not full (TX) / not empty (RX)
#define FIFOCON_UINC 0x01        // Byte 1
#define FIFOCON_TXREQ 0x02       // Byte 1
#define FIFOSTA_TFNRFNIF 0x01    // Byte 0: not full (TX) / not empty (RX)
#define FIFOSTA_RXOVIF 0x08      // Byte 0: the RX FIFO overflowed

// Message object flags word
#define OBJ_FLAG_IDE (1u << 4)
#define OBJ_FLAG_BRS (1u << 6)
#define OBJ_FLAG_FDF (1u << 7)

// Clock for configuration, until the oscillator is known
#define CONFIG_SPI_HZ 1000000

MCP2518FD_Lite::MCP2518FD_Lite(spi_host_device_t host, int sck, int mosi, int miso, int cs, int int_pin,
                               uint32_t poll_us)
    : AsyncSpiDevice(stepDevice),
      _host(host),
      _sck(sck),
      _mosi(mosi),
      _miso(miso),
      _cs(cs),
      _int_pin(int_pin),
      _poll_us(poll_us) {}

MCP2518FD_Lite::~MCP2518FD_Lite() {
  if (_poll_timer) {
    esp_timer_stop(_poll_timer);
    esp_timer_delete(_poll_timer);
  }
  if (bus) {
    bus->detachInt(this);
    bus->remove(this);
  }
}

bool MCP2518FD_Lite::joinBus() {
  if (bus) {
    return true;
  }
  if (_int_pin >= 0) {
    pinMode(_int_pin, INPUT_PULLUP);
  }
  AsyncSpiBus* b = AsyncSpiBus::get(_host, _sck, _mosi, _miso);
  return b && b->add(this, _cs, CONFIG_SPI_HZ);
}

// Disable stepping whilst a task is reconfiguring the chip.
void MCP2518FD_Lite::disableStepping() {
  _stepping_disabled = true;
  bus->hold(true);
  bus->hold(false);
}

// Undo disableStepping(), and step for anything that came up in the meantime.
void MCP2518FD_Lite::enableStepping() {
  _stepping_disabled = false;
  requestStep();
}

uint32_t MCP2518FD_Lite::autodetectOscillatorFrequency() {
  if (!joinBus()) {
    return 0;
  }
  disableStepping();
  bus->setClock(this, CONFIG_SPI_HZ);
  const uint32_t f_osc = reset() ? detectOscillator() : 0;
  enableStepping();
  return f_osc;
}

bool MCP2518FD_Lite::begin(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div, bool loopback) {
  if (!joinBus()) {
    return false;
  }
  disableStepping();
  const bool ok = configure(speed, clko_div, loopback);
  enableStepping();
  if (!ok) {
    return false;
  }
  if (!_started) {
    if (_int_pin >= 0 && !bus->attachInt(this, _int_pin)) {
      return false;
    }
    if (_poll_us) {
      const esp_timer_create_args_t args = {
          .callback = [](void* self) { static_cast<MCP2518FD_Lite*>(self)->poll(); }, .arg = this, .name = "mcp2518fd"};
      if (esp_timer_create(&args, &_poll_timer) != ESP_OK || esp_timer_start_periodic(_poll_timer, _poll_us) != ESP_OK) {
        return false;
      }
    }
    _started = true;
  }
  requestStep();  // Anything already pending
  return true;
}

// Reset and set up the chip
bool MCP2518FD_Lite::configure(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div, bool loopback) {
  bus->setClock(this, CONFIG_SPI_HZ);
  _paused = false;
  _rx_overflow = false;
  _errors = false;
  _tx_ie = false;

  if (!reset()) {
    return false;
  }

  _f_osc = speed.f_osc ? speed.f_osc : detectOscillator();
  if (_f_osc == 0) {
    return false;
  }

  // Configure the oscillator (no PLL, no divide by 2) and the CLKO divider
  _clko_div = clko_div & 0x03;
  writeReg8(REG_OSC, (uint8_t)(_clko_div << 5));

  // Wait for OSCRDY (bit 10 of the OSC register)
  bool ready = false;
  for (int i = 0; i < 50 && !ready; i++) {
    ready = readReg8(REG_OSC + 1) & (1 << 2);
    if (!ready) {
      delay(1);
    }
  }
  if (!ready) {
    return false;
  }

  // The operational SPI clock: 0.4 x SYSCLK (16 MHz at 40 MHz, 8 MHz at 20 MHz),
  // within the 0.85 x SYSCLK / 2 limit
  bus->setClock(this, (_f_osc * 2) / 5);

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
  writeReg8(REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX), FIFOCON_TFNRFNIE);

  // TX FIFO: 64 byte payload, configured depth, unlimited retransmissions. Its
  // not-full interrupt is only enabled while the FIFO is full (see step()).
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 2, (2 << 5) | 0x00);  // TXAT = unlimited
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 3, (PAYLOAD_SIZE_64 << 5) | (CONTROLLER_TX_FIFO_SIZE - 1));
  writeReg8(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX), FIFOCON_TXEN);

  // Drive INT from the FIFO interrupts enabled above (CiINT RXIE | TXIE)
  writeReg8(REG_CiINT + 2, 0x03);

  // Filter 0: accept everything and route to the RX FIFO
  writeReg32(REG_CiFLTOBJ(0), 0x00000000);
  writeReg32(REG_CiMASK(0), 0x00000000);
  writeReg8(REG_CiFLTCON(0), 0x80 | CONTROLLER_RX_FIFO_INDEX);

  if (!applySpeed(speed)) {
    return false;
  }

  // Finally leave configuration mode
  _run_mode = loopback ? MODE_INTERNAL_LOOPBACK : MODE_NORMAL_FD;
  return requestMode(_run_mode);
}

bool MCP2518FD_Lite::sendFrame(const MCP2518FD_Lite_Frame& msg) {
  RingRecord record;
  record.id = msg.id;
  record.len = msg.dlc > 64 ? 64 : msg.dlc;
  record.flags = (msg.ext ? RECORD_EXT : 0) | (msg.fd ? RECORD_FD : 0);
  record.reserved[0] = 0;
  record.reserved[1] = 0;
  memcpy(record.payload, msg.data, record.len);

  // This might get called from multiple tasks, for which we need a critical
  // section to protect the TX ring buffer.
#if MCP2518FD_LITE_TX_LOCK
  portENTER_CRITICAL(&_tx_lock);
#endif
  const bool written = _tx.write(record);
#if MCP2518FD_LITE_TX_LOCK
  portEXIT_CRITICAL(&_tx_lock);
#endif
  if (written) {
    requestStep();
  }
  return written;
}

bool MCP2518FD_Lite::receiveFrame(MCP2518FD_Lite_Frame& msg) {
  if (_rx_overflow) {
    DEBUG_PRINTF("MCP2518FD RX ring overflow!\n");
    _rx_overflow = false;
  }
  // Safety net for a missed INT edge, and error flag housekeeping
  if (_started && millis() - _last_poll_ms >= MCP2518FD_LITE_POLL_TIMEOUT_MS) {
    _last_poll_ms = millis();
    checkErrors();
    requestStep();
  }

  RingRecord record;
  if (!_rx.read(record)) {
    return false;
  }
  msg.id = record.id;
  msg.dlc = record.len;
  msg.ext = record.flags & RECORD_EXT;
  msg.fd = record.flags & RECORD_FD;
  memcpy(msg.data, record.payload, record.len);
  return true;
}

void MCP2518FD_Lite::changeSpeed(const MCP2518FD_Lite_Speed& new_speed) {
  if (!bus) {
    return;
  }
  disableStepping();
  if (requestMode(MODE_CONFIGURATION)) {
    applySpeed(new_speed);
    if (!_paused) {
      requestMode(_run_mode);
    }
  } else {
    // It wouldn't leave normal mode: reset it instead (straight to config
    // mode), and set it up again
    const bool paused = _paused;
    configure(new_speed, _clko_div, _run_mode == MODE_INTERNAL_LOOPBACK);
    if (paused) {
      requestMode(MODE_CONFIGURATION);
      _paused = true;
    }
  }
  enableStepping();
}

// We can pause the chip by putting it in configuration mode, which disables
// ACKing, and also stop stepping.
void MCP2518FD_Lite::pause(bool paused) {
  if (!bus || paused == _paused) {
    return;
  }
  disableStepping();
  requestMode(paused ? MODE_CONFIGURATION : _run_mode);
  _paused = paused;
  enableStepping();
}

// Check for SERRIF (system error) and CERRIF (CAN bus error) flags
void MCP2518FD_Lite::checkErrors() {
  const uint8_t err_flags = readReg8(REG_CiINT + 1) & ((1u << 4) | (1u << 5));
  if (err_flags) {
    _errors = true;
    writeReg8(REG_CiINT + 1, (uint8_t)~err_flags);
  }
}

int MCP2518FD_Lite::debugState(char* buf, size_t n) {
  if (!bus) {
    return snprintf(buf, n, "2518: no bus");
  }
  return snprintf(buf, n,
                  "2518: CiCON %08lx CiNBTCFG %08lx CiINT %08lx CiTREC %08lx CiBDIAG1 %08lx FIFOSTA1 %08lx FIFOCON2 "
                  "%08lx FIFOSTA2 %08lx | tx_ie %d paused %d stepping_disabled %d started %d state %u pending %d int %d f_osc %lu",
                  (unsigned long)readReg32(REG_CiCON), (unsigned long)readReg32(REG_CiNBTCFG),
                  (unsigned long)readReg32(REG_CiINT), (unsigned long)readReg32(0x034), (unsigned long)readReg32(0x03C),
                  (unsigned long)readReg32(REG_CiFIFOSTA(CONTROLLER_RX_FIFO_INDEX)),
                  (unsigned long)readReg32(REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX)),
                  (unsigned long)readReg32(REG_CiFIFOSTA(CONTROLLER_TX_FIFO_INDEX)), _tx_ie, _paused, _stepping_disabled, _started,
                  state, pending, _int_pin >= 0 ? (int)gpio_ll_get_level(&GPIO, _int_pin) : -1, (unsigned long)_f_osc);
}

// --- Blocking register access ---

void MCP2518FD_Lite::readBytes(uint16_t address, uint8_t* dest, size_t nbytes) {
  uint8_t buf[2 + 72];
  cmd16(buf, INSTRUCTION_READ, address);
  bus->transferBlocking(this, buf, 2 + nbytes);  // In place: the reply follows the 2 command bytes
  memcpy(dest, buf + 2, nbytes);
}

void MCP2518FD_Lite::writeBytes(uint16_t address, const uint8_t* src, size_t nbytes) {
  uint8_t buf[2 + 72];
  cmd16(buf, INSTRUCTION_WRITE, address);
  memcpy(buf + 2, src, nbytes);
  bus->transferBlocking(this, buf, 2 + nbytes);
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
  requestMode(MODE_CONFIGURATION);

  uint8_t cmd[2] = {0x00, 0x00};
  bus->transferBlocking(this, cmd, sizeof(cmd));
  delay(5);

  // The reset leaves the controller in configuration mode
  return requestMode(MODE_CONFIGURATION);
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

// --- Interrupt-driven stepping ---
//
// A run of steps moves a frame from the RX FIFO to the RX ring, and one from
// the TX ring to the TX FIFO (enabling the TX FIFO's not-full interrupt when
// it's full, and disabling it when there's nothing to send). Steps are
// triggered from AsyncSpiBus in response to interrupts (each step is one SPI
// transaction).

enum { S_START, S_RX_STA, S_RX_CHECK, S_RX_HDR, S_RX_PUSH, S_TX, S_TX_STA, S_TX_MORE, S_TX_UINC, S_END };

bool IRAM_ATTR MCP2518FD_Lite::stepDevice(AsyncSpiDevice* dev) {
  return static_cast<MCP2518FD_Lite*>(dev)->step();
}

bool IRAM_ATTR ASYNC_SPI_BUS_HOT MCP2518FD_Lite::step() {
  for (;;) {
    switch (state) {
      case S_START:
        if (_paused || _stepping_disabled) {
          return false;
        }
        _did_work = false;
        // Read the RX FIFO status and usage registers
        cmd16(_fifo_regs, INSTRUCTION_READ, REG_CiFIFOSTA(CONTROLLER_RX_FIFO_INDEX));  // FIFOSTA, FIFOUA
        return spiTransfer(S_RX_STA, _fifo_regs, _fifo_regs, 10);

      case S_RX_STA:
        if (_fifo_regs[2] & FIFOSTA_RXOVIF) {
          // RX FIFO overflow detected
          _rx_overflow = true;
          // Clear the RX FIFO overflow flag in the controller
          cmd16(_cmd_buf, INSTRUCTION_WRITE, REG_CiFIFOSTA(CONTROLLER_RX_FIFO_INDEX));
          _cmd_buf[2] = (uint8_t)~FIFOSTA_RXOVIF;
          return spiTransfer(S_RX_CHECK, _cmd_buf, nullptr, 3);
        }
        state = S_RX_CHECK;
        break;

      case S_RX_CHECK:
        if (!(_fifo_regs[2] & FIFOSTA_TFNRFNIF)) {
          // RX FIFO empty, nothing to read, go to TX step
          state = S_TX;
          break;
        }
        // Read the header and the first 8 payload bytes
        _object_addr = CAN_RAM_BASE + (uint16_t)readLE32(_fifo_regs + 6);
        cmd16(_object, INSTRUCTION_READ, _object_addr);
        return spiTransfer(S_RX_HDR, _object, _object, 2 + 16);

      case S_RX_HDR: {
        const uint8_t length = kDlcToBytes[readLE32(_object + 6) & 0x0F];
        if (length > 8) {
          // Read the rest of the payload if it's larger than 8 bytes
          cmd16(_rx_rest, INSTRUCTION_READ, _object_addr + 16);
          return spiTransfer(S_RX_PUSH, _rx_rest, _rx_rest, 2 + (length + 3) / 4 * 4 - 8);
        }
        state = S_RX_PUSH;
        break;
      }

      case S_RX_PUSH: {
        const uint32_t flags = readLE32(_object + 6);
        const bool ext = flags & OBJ_FLAG_IDE;
        RingRecord record;
        record.id = canIdFromChipId(readLE32(_object + 2), ext);
        record.len = kDlcToBytes[flags & 0x0F];
        record.flags = (ext ? RECORD_EXT : 0) | ((flags & OBJ_FLAG_FDF) ? RECORD_FD : 0);
        record.reserved[0] = 0;
        record.reserved[1] = 0;
        // Assemble the payload from the object buffer(s)
        memcpy(record.payload, _object + 10, record.len < 8 ? record.len : 8);
        if (record.len > 8) {
          memcpy(record.payload + 8, _rx_rest + 2, record.len - 8);
        }
        // Push the received record into the RX ring buffer
        if (!_rx.write(record)) {
          _rx_overflow = true;
        }
        // Release the FIFO slot by writing to UINC
        cmd16(_cmd_buf, INSTRUCTION_WRITE, REG_CiFIFOCON(CONTROLLER_RX_FIFO_INDEX) + 1);
        _cmd_buf[2] = FIFOCON_UINC;
        _did_work = true;
        return spiTransfer(S_TX, _cmd_buf, nullptr, 3);
      }

      case S_TX:
        if (!_tx.empty()) {
          // Read the TX FIFO status to check if there's room for new transmissions
          cmd16(_fifo_regs, INSTRUCTION_READ, REG_CiFIFOSTA(CONTROLLER_TX_FIFO_INDEX));  // FIFOSTA, FIFOUA
          return spiTransfer(S_TX_STA, _fifo_regs, _fifo_regs, 10);
        }
        if (_tx_ie) {
          // Nothing left to send, disable the previously-enabled not-full interrupt.
          cmd16(_cmd_buf, INSTRUCTION_WRITE, REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX));
          _cmd_buf[2] = FIFOCON_TXEN;
          _tx_ie = false;
          _did_work = true;
          return spiTransfer(S_END, _cmd_buf, nullptr, 3);
        }
        state = S_END;
        break;

      case S_TX_STA: {
        if (!(_fifo_regs[2] & FIFOSTA_TFNRFNIF)) {
          // The TX FIFO is full, so enable the not-full interrupt so we'll be
          // notified when there's space for more frames.
          if (!_tx_ie) {
            cmd16(_cmd_buf, INSTRUCTION_WRITE, REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX));
            _cmd_buf[2] = FIFOCON_TXEN | FIFOCON_TFNRFNIE;
            _tx_ie = true;
            return spiTransfer(S_END, _cmd_buf, nullptr, 3);
          }
          state = S_END;
          break;
        }
        RingRecord record;
        _tx.read(record);
        const bool ext = record.flags & RECORD_EXT;
        const uint32_t dlc = lengthCodeForLength(record.len);
        const uint8_t words = (kDlcToBytes[dlc] + 3) / 4;  // Whole words, including the padding
        cmd16(_object, INSTRUCTION_WRITE, CAN_RAM_BASE + (uint16_t)readLE32(_fifo_regs + 6));
        writeLE32(_object + 2, chipIdFromCanId(record.id, ext));
        const uint32_t fd_flags = (record.flags & RECORD_FD) ? OBJ_FLAG_FDF | OBJ_FLAG_BRS : 0;
        writeLE32(_object + 6, dlc | (ext ? OBJ_FLAG_IDE : 0) | fd_flags);
        memset(_object + 10, 0, words * 4);
        memcpy(_object + 10, record.payload, record.len);
        _object_len = 10 + words * 4;
        if (_object_len > 64) {
          // The SPI hardware can only transfer 64 bytes at a time, so we'll need to split.
          return spiTransfer(S_TX_MORE, _object, nullptr, 64, true);
        }
        return spiTransfer(S_TX_UINC, _object, nullptr, _object_len);
      }

      case S_TX_MORE:
        // Send the remaining chunk
        return spiTransfer(S_TX_UINC, _object + 64, nullptr, _object_len - 64);

      case S_TX_UINC:
        // Queue the frame for transmission by setting the UINC and TXREQ bits.
        cmd16(_cmd_buf, INSTRUCTION_WRITE, REG_CiFIFOCON(CONTROLLER_TX_FIFO_INDEX) + 1);
        _cmd_buf[2] = FIFOCON_UINC | FIFOCON_TXREQ;
        _did_work = true;
        return spiTransfer(S_END, _cmd_buf, nullptr, 3);

      default: {  // S_END
        // End of the SPI transaction sequence. We'll go round again if INT is
        // still low, or there's a frame to send and the TX FIFO isn't known to
        // be full. If INT is low, but we did no work, increment the no-work
        // counter to avoid spinning indefinitely.
        const bool int_low = _int_pin >= 0 ? gpio_ll_get_level(&GPIO, _int_pin) == 0 : _did_work;
        _no_work_count = (_did_work || !int_low) ? 0 : _no_work_count + 1;
        if ((int_low && _no_work_count < 3) || (!_tx_ie && !_tx.empty())) {
          // Work is pending, we'll go round again
          pending = true;
        }
        return false;
      }
    }
  }
}
