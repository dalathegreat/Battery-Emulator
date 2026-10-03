#pragma once

#include <stdint.h>

#include "esp_timer.h"

#include "../mcp2515_lite/async_spi_bus.h"
#include "mcp2518fd_lite_util.h"

/* MCP2518FD_Lite: Minimal interrupt-driven MCP2518FD (CAN FD) driver.

 - Non-blocking send/receive
 - Variable-length frame storage in ring buffers (can buffer more 8-bit frames
   than 64-bit ones)
 - Can be used without an INT pin (polled by a timer or by calling `poll()`)
 - Pause/resume (suspends rx/tx and ACKs), on-the-fly speed changes
 - Can share its SPI bus with other AsyncSpiBus devices
*/

// Request stepping (and check the error flags) this often anyway, in case an INT
// edge was missed
#define MCP2518FD_LITE_POLL_TIMEOUT_MS 1000

// Host-side ring buffer sizes in bytes, must be a power of two.
#define MCP2518FD_LITE_TX_BUF_SIZE 2048
#define MCP2518FD_LITE_RX_BUF_SIZE 2048

// Enable to make sendFrame safe for concurrent calls from multiple tasks.
#ifndef MCP2518FD_LITE_TX_LOCK
#define MCP2518FD_LITE_TX_LOCK 1
#endif

// This has the same layout as CAN_frame.
typedef struct {
  bool fd;
  bool ext;
  uint8_t dlc;
  uint32_t id;
  uint8_t data[64];
} MCP2518FD_Lite_Frame;

// FD requires both nominal (arbitration) and data bitrates. f_osc of 0
// means autodetect (20 or 40MHz).
typedef struct {
  uint32_t nominal_bitrate;
  uint32_t data_bitrate;
  uint32_t f_osc;
} MCP2518FD_Lite_Speed;

class MCP2518FD_Lite : public AsyncSpiDevice {
 public:
  // Sets up `host` with these pins, unless another AsyncSpiBus device already
  // did. Set int_pin to -1 to use polling only.
  MCP2518FD_Lite(spi_host_device_t host, int sck, int mosi, int miso, int cs, int int_pin, uint32_t poll_us = 0);
  ~MCP2518FD_Lite();

  // Measure the connected oscillator (returns 20000000/40000000, or 0 on failure)
  uint32_t autodetectOscillatorFrequency();

  // clko_div is the CLKO pin divider (0..3), used as the oscillator for a
  // second daisy-chained controller on some hardware.
  bool begin(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div = 3, bool loopback = false);

  // Queue a frame to send if there's room (non-blocking)
  bool sendFrame(const MCP2518FD_Lite_Frame& msg);

  // Receive the next available frame (non-blocking)
  bool receiveFrame(MCP2518FD_Lite_Frame& msg);

  // Change CAN speed (blocks briefly)
  void changeSpeed(const MCP2518FD_Lite_Speed& new_speed);

  // Trigger a step to process SPI transfers and move frames between the
  // controller and the ring buffers. Normally triggered by the external
  // interrupt if you're using that.
  void poll() { requestStep(); }

  // Pauses all communication (and stops acknowledging messages)
  void pause(bool paused);

  // Debugging: prints the chip's key registers and the driver's state
  int debugState(char* buf, size_t n);

  inline bool hasErrors() {
    auto ret = _errors;
    _errors = false;
    return ret;
  }

 private:
  spi_host_device_t _host;
  int _sck, _mosi, _miso;
  uint8_t _cs;
  int8_t _int_pin;  // -1: no external interrupt pin
  uint32_t _poll_us;
  esp_timer_handle_t _poll_timer = nullptr;
  bool _started = false;  // Whether begin() has attached the INT handler
  uint32_t _f_osc = 0;
  uint8_t _run_mode = 0;  // Normal FD or internal loopback
  uint8_t _clko_div = 3;

  volatile bool _paused = false;
  volatile bool _stepping_disabled = false;  // A task is reconfiguring the chip
  volatile bool _rx_overflow = false;
  volatile bool _errors = false;

  using RingRecord = MCP2518FD_Lite_RingRecord;
  static constexpr uint8_t RECORD_EXT = RingRecord::EXT;
  static constexpr uint8_t RECORD_FD = RingRecord::FD;
  template <size_t Size>
  using RecordRing = MCP2518FD_Lite_RecordRing<Size>;
  RecordRing<MCP2518FD_LITE_TX_BUF_SIZE> _tx;
  RecordRing<MCP2518FD_LITE_RX_BUF_SIZE> _rx;
#if MCP2518FD_LITE_TX_LOCK
  portMUX_TYPE _tx_lock = portMUX_INITIALIZER_UNLOCKED;
#endif

  // Stepping state (see step())
  bool _tx_ie = false;  // TX FIFO not-full interrupt enabled
  bool _did_work = false;
  uint8_t _no_work_count = 0;
  uint8_t _fifo_regs[12];    // FIFO status read: echo 2, FIFOSTA 4, FIFOUA 4
  uint8_t _cmd_buf[4];       // Buffer for doing command register writes
  uint16_t _object_addr = 0;  // Chip RAM address of the RX object being read
  uint8_t _object_len = 0;    // Number of bytes of _object to send
  uint8_t _object[2 + 72];    // A command and CAN frame (in the chip SPI format)
  uint8_t _rx_rest[2 + 56];   // Rest of a long RX payload (if the first read didn't get it all)
  uint32_t _last_poll_ms = 0;

  bool joinBus();
  void requestStep() {
    if (_started) bus->requestStep(this);
  }
  bool step();
  static bool stepDevice(AsyncSpiDevice* dev);
  // Start an SPI transfer and go to next_state when it's done.
  bool spiTransfer(uint8_t next_state, const uint8_t* tx, uint8_t* rx, uint8_t len, bool keep_cs = false) {
    state = next_state;
    bus->startTransfer(tx, rx, len, keep_cs);
    return true;
  }

  // Blocking register access, from tasks
  void readBytes(uint16_t address, uint8_t* dest, size_t nbytes);
  void writeBytes(uint16_t address, const uint8_t* src, size_t nbytes);
  uint8_t readReg8(uint16_t address);
  uint32_t readReg32(uint16_t address);
  void writeReg8(uint16_t address, uint8_t value);
  void writeReg32(uint16_t address, uint32_t value);

  void disableStepping();
  void enableStepping();
  bool configure(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div, bool loopback);
  bool reset();
  bool requestMode(uint8_t mode);
  bool waitForMode(uint8_t mode, uint32_t timeout_ms);
  uint32_t detectOscillator();
  bool applySpeed(const MCP2518FD_Lite_Speed& speed);
  void checkErrors();


  MCP2518FD_Lite(const MCP2518FD_Lite&) = delete;
  MCP2518FD_Lite& operator=(const MCP2518FD_Lite&) = delete;
};
