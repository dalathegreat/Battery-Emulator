//----------------------------------------------------------------------------------------
//  TWAI_ESP32: native CAN driver for the ESP32 family, built on the ESP-IDF TWAI
//  driver (esp_twai.h / esp_twai_onchip.h).
//
//  API:
//    - TWAI_ESP32::instance(controller, tx_pin, rx_pin)
//      One immortal instance per on-chip TWAI controller, allocated from
//      internal RAM on first use (so unused controllers cost no RAM) and
//      never freed. The controller index selects the peripheral slot
//      (0 .. SOC_TWAI_CONTROLLER_NUM-1: 1 slot on ESP32/S2/S3/C3/H2, 2 on
//      ESP32-C6, 3 on ESP32-P4); the pins are fixed at first use. Instances
//      can never be destroyed, stack-allocated or created any other way -
//      misuse is a compile error, and invalid controller indices, pin
//      clashes or out-of-memory abort at runtime.
//    - begin(bit_rate, mode)  (returns 0 on success, else a non-zero ESP error
//      code; reuses the constructor pins, so a speed change is just
//      begin(new_bit_rate))
//    - restart()        (re-apply the last configuration)
//    - end()            (stop the controller and release the driver)
//    - tryToSend(CANMessage), available(), receive(CANMessage&),
//      statusRegister(), recoverFromBusOff()
//
//  Note: the ESP-IDF driver assigns physical controllers automatically (first
//  free slot) so the controller index is just a convenience.
//
//----------------------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "soc/soc_caps.h"

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
//  TWAI_ESP32 class (singleton per on-chip TWAI controller)
//----------------------------------------------------------------------------------------

class TWAI_ESP32 {
public: 
  
  typedef enum : uint8_t {
    NormalMode,
    ListenOnlyMode,
    LoopBackMode
  } CANMode;

  // Return the instance corresponding to the given controller index, creating it
  // if necessary.
  static TWAI_ESP32 &instance(uint8_t controller, gpio_num_t tx_pin, gpio_num_t rx_pin);

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
  // Instances are immortal singletons, created only via instance(). This also
  // guarantees the ESP-IDF driver's callback user_data and queued frame
  // pointers always reference live memory.
  TWAI_ESP32(gpio_num_t tx_pin, gpio_num_t rx_pin);
  ~TWAI_ESP32(); // Never invoked: instances are never destroyed
  TWAI_ESP32(const TWAI_ESP32&) = delete;
  TWAI_ESP32& operator = (const TWAI_ESP32&) = delete;

  // Stop the node. May block for a while if the node is bus-off, and is not
  // guaranteed to succeed.
  void teardownNode(void);
  // Attempt bus-off recovery, blocking until success or timeout.
  esp_err_t recoverAndWait(twai_node_handle_t node);

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

  static constexpr uint32_t kRecoveryTimeoutMs = 100;

  void resetTxSlots(void);
  TWAI_ISR_CALLBACK_ATTR void storeRxFrame(const twai_frame_t &frame);
};
