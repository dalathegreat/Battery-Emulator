//----------------------------------------------------------------------------------------
//  TWAI_ESP32: native CAN driver for the ESP32 / ESP32-S3, built on the ESP-IDF
//  TWAI driver (esp_twai.h / esp_twai_onchip.h).
//
//  API:
//    - TWAI_ESP32(tx_pin, rx_pin)  (construct with the controller GPIO pins;
//      they are fixed for the lifetime of the instance)
//    - begin(bit_rate, mode)  (returns 0 on success, else a non-zero ESP error
//      code; reuses the constructor pins, so a speed change is just
//      begin(new_bit_rate))
//    - restart()        (re-apply the last configuration)
//    - end()            (stop the controller and release the driver)
//    - tryToSend(CANMessage), available(), receive(CANMessage&),
//      statusRegister(), recoverFromBusOff()
//
//----------------------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "esp_twai.h"
#include "esp_twai_onchip.h"

// If CONFIG_TWAI_ISR_CACHE_SAFE is set, we must ensure that all our callbacks,
// and all the code they themselves call, are IRAM_ATTR. This allows the ISR to
// run with the flash cache disabled.

// There is also an opt-in to move the callbacks to IRAM, as long as the ISR
// itself is in IRAM (CONFIG_TWAI_ISR_IN_IRAM), which should increase
// performance.
// Enable this by defining TWAI_CALLBACKS_IN_IRAM.

#if CONFIG_TWAI_ISR_CACHE_SAFE || (CONFIG_TWAI_ISR_IN_IRAM && defined(TWAI_CALLBACKS_IN_IRAM))
#define TWAI_ISR_CALLBACK_ATTR IRAM_ATTR
#else
#define TWAI_ISR_CALLBACK_ATTR
#endif

//----------------------------------------------------------------------------------------
//  Generic CAN message (identical to the one shipped with ACAN2517FD; the include
//  guard makes all copies interchangeable)
//----------------------------------------------------------------------------------------

#ifndef GENERIC_CAN_MESSAGE_DEFINED
#define GENERIC_CAN_MESSAGE_DEFINED

class CANMessage {
  public : uint32_t id = 0;  // Frame identifier
  public : bool ext = false; // false -> standard frame, true -> extended frame
  public : bool rtr = false; // false -> data frame, true -> remote frame
  public : uint8_t idx = 0;  // This field is used by the driver
  public : uint8_t len = 0;  // Length of data (0 ... 8)
  public : union {
    uint64_t data64;
    int64_t  data_s64;
    uint32_t data32    [2];
    int32_t  data_s32  [2];
    float    dataFloat [2];
    uint16_t data16    [4];
    int16_t  data_s16  [4];
    int8_t   data_s8   [8];
    uint8_t  data      [8] = {0, 0, 0, 0, 0, 0, 0, 0};
  };
};

// Same companion typedefs as in the ACAN2517FD CANMessage.h, so the include
// guard makes the two headers fully interchangeable regardless of include order.
typedef enum {kStandard, kExtended} tFrameFormat;
typedef enum {kData, kRemote} tFrameKind;
typedef void(*ACANCallBackRoutine) (const CANMessage &inMessage);

#endif

// Status register bits (for creating a synthetic status register output)
static const uint32_t TWAI_BUS_OFF_ST = 0x80; // Controller in bus-off state
static const uint32_t TWAI_ERR_ST     = 0x40; // Error status (warning/passive/bus-off)

//----------------------------------------------------------------------------------------
//  TWAI_ESP32 class
//----------------------------------------------------------------------------------------

class TWAI_ESP32 {
public: 
  
  typedef enum : uint8_t {
    NormalMode,
    ListenOnlyMode,
    LoopBackMode
  } CANMode;

  TWAI_ESP32(gpio_num_t tx_pin, gpio_num_t rx_pin);
  ~TWAI_ESP32();

  uint32_t begin(const uint32_t bitrate = 500000,
                 const CANMode mode = NormalMode);
    
  uint32_t restart(void);

  // Stop the controller and release the driver
  void end(void);

  // Whether there are frames in the receive buffer (non-blocking)
  bool available(void) const;
  // Read a frame from the receive buffer (non-blocking). Returns true if a frame was read.
  bool receive(CANMessage &message);

  // Attempt to enqueue a message for transmission (non-blocking)
  bool tryToSend(const CANMessage &message);

  uint32_t statusRegister(void) const;
  // Attempt to recover from bus-off (non-blocking)
  bool recoverFromBusOff(void);

private:
  // Disable copy
  TWAI_ESP32(const TWAI_ESP32&) = delete;
  TWAI_ESP32& operator = (const TWAI_ESP32&) = delete;

  void teardownNode(void);
  // Event callbacks (called from ESP-IDF)
  static TWAI_ISR_CALLBACK_ATTR bool txDoneCallback(twai_node_handle_t handle,
                                       const twai_tx_done_event_data_t *edata,
                                       void *user_data);
  static TWAI_ISR_CALLBACK_ATTR bool rxDoneCallback(twai_node_handle_t handle,
                                       const twai_rx_done_event_data_t *edata,
                                       void *user_data);

  // The transmit slots
  static const uint8_t _tx_slot_count = 32;
  struct TxSlot {
    twai_frame_t frame;
    uint8_t data[8];
    volatile bool in_use;
  };
  TxSlot _tx_slots[_tx_slot_count];

  // TX flow control (workaround for a race in the ESP-IDF on-chip TWAI driver,
  // esp_twai_onchip.c:_node_queue_tx). The driver's internal TX queue asserts
  // "should always get frame at this moment" when a sender is preempted for
  // longer than one CAN frame between queueing a frame and its "second chance"
  // hw_busy CAS: the TX-done ISR then drains the queue and clears hw_busy, so
  // the CAS wins but the queue is empty. To avoid the racy path entirely we
  // hand the driver at most one frame at a time (_tx_in_flight) and park the
  // rest in our own FIFO, feeding them to the driver one-by-one as each
  // transmission completes. The driver's internal queue is then never used.
  volatile bool _tx_in_flight = false;
  volatile uint8_t _tx_pending[_tx_slot_count];
  volatile uint8_t _tx_pending_head = 0;
  volatile uint8_t _tx_pending_tail = 0;
  volatile uint8_t _tx_pending_count = 0;

  // The receive ring buffer (filled by the RX callback, drained by receive())
  static const uint16_t _rx_buffer_size = 32;
  CANMessage _rx_buffer[_rx_buffer_size];
  volatile uint16_t _rx_head = 0;
  volatile uint16_t _rx_tail = 0;
  volatile uint16_t _rx_count = 0;

  uint32_t _bit_rate = 0;
  gpio_num_t _tx_pin = GPIO_NUM_5;
  gpio_num_t _rx_pin = GPIO_NUM_4;
  CANMode _mode = NormalMode;

  twai_node_handle_t _node = nullptr;
  bool _node_enabled = false;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  void resetTxSlots(void);
  TWAI_ISR_CALLBACK_ATTR void storeRxFrame(const twai_frame_t &frame);
};
