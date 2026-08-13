//----------------------------------------------------------------------------------------
//  TWAI_ESP32: drop-in replacement for the ACAN_ESP32 native CAN driver, built on the
//  ESP-IDF TWAI driver (esp_twai.h / esp_twai_onchip.h).
//
//  Why this exists: ACAN_ESP32 talks to the TWAI registers directly (raw bit-banging)
//  and does not include the silicon errata workarounds (e.g. ESP32 TWAI_ERRATA_FIX_*:
//  bus-off recovery, TX interrupt lost, RX frame invalid, RX FIFO corrupt, listen-only
//  dominant). The ESP-IDF TWAI driver implements those workarounds.
//
//  The public interface mirrors the subset of ACAN_ESP32 used by comm_can.cpp:
//    - TWAI_ESP32::can  (static instance, like ACAN_ESP32::can)
//    - begin(settings)  (returns 0 on success, else a non-zero error code)
//    - end()            (stop the controller and release the driver)
//    - tryToSend(CANMessage), available(), receive(CANMessage&), statusRegister()
//    - TWAI_ESP32_Settings (bit rate, pins, mode, plus timing fields for logging)
//
//----------------------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "esp_twai.h"
#include "esp_twai_onchip.h"

//----------------------------------------------------------------------------------------
//  Generic CAN message (identical to the one shipped with ACAN_ESP32 / ACAN2517FD;
//  the include guard makes all copies interchangeable)
//----------------------------------------------------------------------------------------

#ifndef GENERIC_CAN_MESSAGE_DEFINED
#define GENERIC_CAN_MESSAGE_DEFINED

class CANMessage {
  public : uint32_t id = 0 ;  // Frame identifier
  public : bool ext = false ; // false -> standard frame, true -> extended frame
  public : bool rtr = false ; // false -> data frame, true -> remote frame
  public : uint8_t idx = 0 ;  // This field is used by the driver
  public : uint8_t len = 0 ;  // Length of data (0 ... 8)
  public : union {
    uint64_t data64        ; // Caution: subject to endianness
    int64_t  data_s64      ; // Caution: subject to endianness
    uint32_t data32    [2] ; // Caution: subject to endianness
    int32_t  data_s32  [2] ; // Caution: subject to endianness
    float    dataFloat [2] ; // Caution: subject to endianness
    uint16_t data16    [4] ; // Caution: subject to endianness
    int16_t  data_s16  [4] ; // Caution: subject to endianness
    int8_t   data_s8   [8] ;
    uint8_t  data      [8] = {0, 0, 0, 0, 0, 0, 0, 0} ;
  } ;
} ;

#endif

//----------------------------------------------------------------------------------------
//  Status register bits, same values and semantics as ACAN_ESP32 (SJA1000 status
//  register). statusRegister() synthesizes them from the ESP-IDF driver state.
//----------------------------------------------------------------------------------------

static const uint32_t TWAI_BUS_OFF_ST = 0x80 ; // Controller in bus-off state
static const uint32_t TWAI_ERR_ST     = 0x40 ; // Error status (warning/passive/bus-off)

//----------------------------------------------------------------------------------------
//  TWAI_ESP32_Settings
//
//  Holds the desired bit rate, pins and mode. The timing fields (mBitRatePrescaler,
//  mTimeSegment1, ...) are computed in the constructor with the same algorithm the
//  ESP-IDF TWAI driver uses, so the values logged by comm_can.cpp match what the
//  driver programs into the controller. Note the prescaler follows the driver's
//  convention (80 MHz APB reference); the HAL writes brp/2 - 1 into the register.
//----------------------------------------------------------------------------------------

class TWAI_ESP32_Settings {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CAN driver operating modes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum : uint8_t {
    NormalMode,
    ListenOnlyMode,
    LoopBackMode
  } CANMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CONSTRUCTOR: computes the bit timing for the given desired bit rate
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: TWAI_ESP32_Settings (const uint32_t inDesiredBitRate) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CAN PINS
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: gpio_num_t mTxPin = GPIO_NUM_5 ;
  public: gpio_num_t mRxPin = GPIO_NUM_4 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Desired bit rate (in bits/s) and computed bit timing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t mDesiredBitRate ;         // In bits/s
  public: uint8_t mBitRatePrescaler = 0 ;    // 1...1024 (driver convention, 80 MHz reference)
  public: uint8_t mTimeSegment1 = 0 ;        // 1...16 (propagation + phase segment 1)
  public: uint8_t mTimeSegment2 = 0 ;        // 1...8
  public: uint8_t mRJW = 0 ;                 // 1...4
  public: bool mTripleSampling = false ;     // The ESP-IDF driver does not expose triple sampling

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Requested mode
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: CANMode mRequestedCANMode = NormalMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Query methods (used by comm_can.cpp logging)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t actualBitRate (void) const ;   // Computed bit rate, matches the driver
  public: bool exactBitRate (void) const ;        // true if actualBitRate == mDesiredBitRate
  public: uint32_t samplePointFromBitStart (void) const ; // Sample point in %

} ;

//----------------------------------------------------------------------------------------
//  TWAI_ESP32 class
//----------------------------------------------------------------------------------------

class TWAI_ESP32 {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Initialisation: returns 0 if ok, otherwise a non-zero ESP error code
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t begin (const TWAI_ESP32_Settings & inSettings) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Deinit: stop the controller and release the driver
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: void end (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Receiving messages (polled from the application loop)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool available (void) const ;
  public: bool receive (CANMessage & outMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Transmitting messages (non-blocking)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool tryToSend (const CANMessage & inMessage) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Synthesized status register (same bits as ACAN_ESP32)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t statusRegister (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Driver instance
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static TWAI_ESP32 can ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   No copy
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: TWAI_ESP32 (const TWAI_ESP32 &) = delete ;
  private: TWAI_ESP32 & operator = (const TWAI_ESP32 &) = delete ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Implementation
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: TWAI_ESP32 (void) ;

  //--- Stop and delete the current node (handles the bus-off case); safe to call
  //    when no node exists.
  private: void teardownNode (void) ;

  //--- Event callbacks (invoked from ISR context by the ESP-IDF driver)
  private: static bool txDoneCallback (twai_node_handle_t handle,
                                       const twai_tx_done_event_data_t * edata,
                                       void * userCtx) ;
  private: static bool rxDoneCallback (twai_node_handle_t handle,
                                       const twai_rx_done_event_data_t * edata,
                                       void * userCtx) ;

  //--- Transmit slots: the ESP-IDF driver only stores a pointer to the frame, so
  //    each in-flight frame and its data buffer must stay valid until the TX-done
  //    event. Slots are returned by the TX-done callback.
  private: static const uint8_t kTxSlotCount = 16 ;
  private: struct TxSlot {
    twai_frame_t frame ;
    uint8_t data [8] ;
    volatile bool inUse ;
  } ;
  private: TxSlot mTxSlots [kTxSlotCount] ;

  //--- Receive ring buffer (filled by the RX callback, drained by receive())
  private: static const uint16_t kRxBufferSize = 32 ;
  private: CANMessage mRxBuffer [kRxBufferSize] ;
  private: volatile uint16_t mRxHead = 0 ;
  private: volatile uint16_t mRxTail = 0 ;
  private: volatile uint16_t mRxCount = 0 ;
  private: uint8_t mRxScratch [8] ;  // Buffer handed to twai_node_receive_from_isr()

  private: twai_node_handle_t mNode = nullptr ;
  private: bool mNodeEnabled = false ;
  private: portMUX_TYPE mMux = portMUX_INITIALIZER_UNLOCKED ;

  private: void resetTxSlots (void) ;
  private: void storeRxFrame (const twai_frame_t & inFrame) ;

} ;
