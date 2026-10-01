#pragma once

#include <stdint.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ringbuf.h"

/* MCP2518FD_Lite: Minimal MCP2518FD (CAN-FD) library for Arduino+FreeRTOS.

Features:
 - Non-blocking send/receive APIs
 - High-priority background task performing blocking SPI transactions
 - Packed buffer layout for efficient memory usage
 - Pause/resume functionality (suspends rx/tx and ACKs)
 - On-the-fly CAN bus speed changes
 - As few SPI transactions as possible for minimum overhead

*/

// A high priority will avoid drops during message bursts
#define MCP2518FD_LITE_TASK_PRIORITY 10
// Minimal stack size (+1500 if you want to do any printf debugging!)
#define MCP2518FD_LITE_TASK_STACK_SIZE 2048
// Poll this often if there are no interrupts
#define MCP2518FD_LITE_POLL_TIMEOUT_MS 1000

// Host-side ring buffer sizes in bytes, must be a power of two.
#define MCP2518FD_LITE_TX_BUF_SIZE 2048
#define MCP2518FD_LITE_RX_BUF_SIZE 2048

// Enable to make sendFrame safe for concurrent calls from multiple tasks.
#ifndef MCP2518FD_LITE_TX_LOCK
#define MCP2518FD_LITE_TX_LOCK 1
#endif

// Whether to validate the requested bitrates (rejecting if they can't be
// encoded), else it will fail silently.
#ifndef MCP2518FD_LITE_VALIDATE_BITRATE
#define MCP2518FD_LITE_VALIDATE_BITRATE 1
#endif

// Whether to read full 64-byte CAN-FD frames each time, or to split the read
// into two. Full reads use more SPI bus time, but fewer transactions.
#ifndef MCP2518FD_READ_FULL_FRAMES
#define MCP2518FD_READ_FULL_FRAMES 0
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

class MCP2518FD_Lite {
 public:
  // Requires an initialized SPIClass (e.g. SPI or SPI2) passed by reference
  MCP2518FD_Lite(SPIClass& spi, uint8_t cs, uint8_t int_pin);
  ~MCP2518FD_Lite();

  // Measure the connected oscillator (returns 20000000/40000000, or 0 on failure)
  uint32_t autodetectOscillatorFrequency();

  // clko_div is the CLKO pin divider (0..3), used as the oscillator for a
  // second daisy-chained controller on some hardware.
  bool begin(const MCP2518FD_Lite_Speed& speed, uint8_t clko_div = 3, bool loopback = false,
             bool skip_task_start = false);

  // Non-blocking: pushes message to the TX ringbuffer. Returns false if the
  // ring doesn't have room for the whole frame.
  bool sendFrame(const MCP2518FD_Lite_Frame& msg);

  // Non-blocking: pops message from the RX ringbuffer. Returns false if the
  // ring is empty.
  bool receiveFrame(MCP2518FD_Lite_Frame& msg);

  // Non-blocking: signals the task to change CAN bus speed
  void changeSpeed(const MCP2518FD_Lite_Speed& new_speed);

  // Non-blocking: pauses all communication (and stops acknowledging messages)
  void pause(bool paused);

  inline bool hasErrors() {
    auto ret = _errors;
    _errors = false;
    return ret;
  }

 private:
  SPIClass& _spi;
  uint8_t _cs;
  uint8_t _int_pin;
  SPISettings _spi_settings;

  // Ring buffer wrappers for TX and RX frames.
  struct ringbuf _tx_ring;
  struct ringbuf _rx_ring;
  // Storage aligned so the task can copy frames out as 32-bit words if desired.
  alignas(uint32_t) uint8_t _tx_buf[MCP2518FD_LITE_TX_BUF_SIZE];
  alignas(uint32_t) uint8_t _rx_buf[MCP2518FD_LITE_RX_BUF_SIZE];
#if MCP2518FD_LITE_TX_LOCK
  portMUX_TYPE _tx_lock = portMUX_INITIALIZER_UNLOCKED;
#endif

  MCP2518FD_Lite_Speed _next_speed;
  volatile bool _speed_change_pending = false;
  volatile bool _pause_requested = false;
  volatile bool _paused = false;
  volatile bool _rx_overflow = false;
  volatile bool _errors = false;
  uint32_t _f_osc = 0;
  uint8_t _clko_div = 3;
  bool _loopback = false;

  // Background task for handling sequential blocking transfers
  TaskHandle_t _can_task_handle = nullptr;
  static void canTask(void* pvParameters);

  // Internal SPI helpers
  void spiTransfer(uint8_t* buf, size_t len);
  void readBytes(uint16_t address, uint8_t* dest, size_t nbytes);
  void writeBytes(uint16_t address, const uint8_t* src, size_t nbytes);
  uint8_t readReg8(uint16_t address);
  uint32_t readReg32(uint16_t address);
  void writeReg8(uint16_t address, uint8_t value);
  void writeReg32(uint16_t address, uint32_t value);

  bool reset();
  bool requestMode(uint8_t mode);
  bool waitForMode(uint8_t mode, uint32_t timeout_ms);
  uint32_t detectOscillator();
  bool applySpeed(const MCP2518FD_Lite_Speed& speed);

  // ISR handler for the CAN interrupt pin
  static void IRAM_ATTR mcp2518fd_isr_handler(void* arg);

  MCP2518FD_Lite(const MCP2518FD_Lite&) = delete;
  MCP2518FD_Lite& operator=(const MCP2518FD_Lite&) = delete;
};
