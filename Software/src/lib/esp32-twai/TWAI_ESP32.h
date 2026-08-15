//----------------------------------------------------------------------------------------
//  TWAI_ESP32: native CAN driver for the ESP32 / ESP32-S3, built on the ESP-IDF
//  TWAI driver (esp_twai.h / esp_twai_onchip.h).
//
//  API:
//    - TWAI_ESP32::can  (static driver instance)
//    - begin(bit_rate, tx_pin, rx_pin, mode)  (returns 0 on success, else a
//      non-zero ESP error code; pass GPIO_NUM_NC for the pins to keep the ones
//      from the last begin()/restart(), e.g. for a simple speed change)
//    - restart()        (re-apply the last configuration)
//    - end()            (stop the controller and release the driver)
//    - tryToSend(CANMessage), available(), receive(CANMessage&),
//      statusRegister()
//
//----------------------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "esp_twai.h"
#include "esp_twai_onchip.h"

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

  uint32_t begin(const uint32_t bitrate = 500000,
                 const gpio_num_t tx_pin = GPIO_NUM_NC,
                 const gpio_num_t rx_pin = GPIO_NUM_NC,
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

  // The static instance corresponding to the single TWAI peripheral.
  static TWAI_ESP32 can;

private:
  // Disable copy
  TWAI_ESP32(const TWAI_ESP32&) = delete;
  TWAI_ESP32& operator = (const TWAI_ESP32&) = delete;

  // Private constructor (only one instance, TWAI_ESP32::can, is allowed)
  TWAI_ESP32(void);

  void teardownNode(void);
  // Event callbacks (called from ESP-IDF)
  static bool txDoneCallback(twai_node_handle_t handle,
                                       const twai_tx_done_event_data_t *edata,
                                       void *user_data);
  static bool rxDoneCallback(twai_node_handle_t handle,
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

  void resetTxSlots(void);
  void storeRxFrame(const twai_frame_t &frame);
};
