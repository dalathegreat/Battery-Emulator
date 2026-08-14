//----------------------------------------------------------------------------------------
//  TWAI_ESP32: native CAN driver for the ESP32 / ESP32-S3, built on the ESP-IDF TWAI
//  driver (esp_twai.h / esp_twai_onchip.h).
//
//  Why this instead of talking to the TWAI registers directly: the ESP-IDF driver
//  implements the silicon errata workarounds (ESP32 TWAI_ERRATA_FIX_*: bus-off
//  recovery, TX interrupt lost, RX frame invalid, RX FIFO corrupt, listen-only
//  dominant).
//
//  API:
//    - TWAI_ESP32::can  (static driver instance)
//    - begin(bitRate, txPin, rxPin, mode)  (returns 0 on success, else a non-zero
//      ESP error code; pass GPIO_NUM_NC for the pins to keep the ones from the
//      last begin()/restart(), e.g. for a simple speed change)
//    - restart()        (re-apply the last configuration)
//    - end()            (stop the controller and release the driver)
//    - tryToSend(CANMessage), available(), receive(CANMessage&), statusRegister()
//    - Query methods (bitRatePrescaler(), timeSegment1(), actualBitRate(), ...)
//      expose the bit timing computed by the last begin(), for logging.
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

// Same companion typedefs as in the ACAN2517FD CANMessage.h, so the include
// guard makes the two headers fully interchangeable regardless of include order.
typedef enum {kStandard, kExtended} tFrameFormat ;
typedef enum {kData, kRemote} tFrameKind ;
typedef void (*ACANCallBackRoutine) (const CANMessage & inMessage) ;

#endif

//----------------------------------------------------------------------------------------
//  Status register bits, same values and semantics as the classic SJA1000 CAN
//  controller status register. statusRegister() synthesizes them from the ESP-IDF
//  driver state.
//----------------------------------------------------------------------------------------

static const uint32_t TWAI_BUS_OFF_ST = 0x80 ; // Controller in bus-off state
static const uint32_t TWAI_ERR_ST     = 0x40 ; // Error status (warning/passive/bus-off)

//----------------------------------------------------------------------------------------
//  TWAI_ESP32 class
//----------------------------------------------------------------------------------------

class TWAI_ESP32 {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CAN driver operating modes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum : uint8_t {
    NormalMode,
    ListenOnlyMode,
    LoopBackMode
  } CANMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Initialisation: returns 0 if ok, otherwise a non-zero ESP error code.
  //
  //   inBitRate   Desired bit rate, in bits/s. The bit timing is computed here
  //               with the same algorithm the ESP-IDF TWAI driver uses, and is
  //               exposed through the query methods below.
  //   inTxPin     GPIO for CAN TX; GPIO_NUM_NC (the default) keeps the pin from
  //               the last begin()/restart().
  //   inRxPin     Same for CAN RX.
  //   inMode      NormalMode (default), ListenOnlyMode or LoopBackMode.
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t begin (const uint32_t inBitRate = 500000,
                          const gpio_num_t inTxPin = GPIO_NUM_NC,
                          const gpio_num_t inRxPin = GPIO_NUM_NC,
                          const CANMode inMode = NormalMode) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Re-apply the configuration of the last begin() (bus recovery, restart
  //   after end()...). Returns 0 if ok, ESP_ERR_INVALID_STATE if never begun.
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t restart (void) ;

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
  //   Synthesized status register (SJA1000-style bits)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t statusRegister (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Bit timing computed by the last begin(), for logging. The values match
  //   exactly what the driver programs into the controller. Note the prescaler
  //   follows the driver's convention (80 MHz APB reference); the HAL writes
  //   brp/2 - 1 into the register. The ESP-IDF driver does not expose triple
  //   sampling.
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Inline: these are called from the logging code and would otherwise cost a
  // function call and a function body each (flash is tight on this target).
  public: uint32_t bitRatePrescaler (void) const { return mBitRatePrescaler ; } // 1...1024
  public: uint32_t timeSegment1 (void) const { return mTimeSegment1 ; }         // 1...16 (prop + phase seg 1)
  public: uint32_t timeSegment2 (void) const { return mTimeSegment2 ; }         // 1...8
  public: uint32_t rjw (void) const { return mRJW ; }                           // 1...4
  public: uint32_t actualBitRate (void) const {
    const uint32_t TQCount = 1 + mTimeSegment1 + mTimeSegment2 ; // Sync + TSEG1 + TSEG2
    if (mBitRatePrescaler == 0 || TQCount == 0) return 0 ;
    return getApbFrequency () / mBitRatePrescaler / TQCount ;
  }
  public: bool exactBitRate (void) const { return actualBitRate () == mBitRate ; }
  public: uint32_t samplePointFromBitStart (void) const {
    const uint32_t TQCount = 1 + mTimeSegment1 + mTimeSegment2 ;
    if (TQCount == 0) return 0 ;
    return ((1 + mTimeSegment1) * 100) / TQCount ;
  }

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

  //--- Compute the bit timing for the given bit rate and store it in the m*
  //    members; returns false if the bit rate is unsupported.
  private: bool computeBitTiming (const uint32_t inBitRate) ;

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

  //--- Last configuration (for restart() and the query methods)
  private: uint32_t mBitRate = 0 ;        // Requested bit rate in bits/s; 0 = never begun
  private: gpio_num_t mTxPin = GPIO_NUM_5 ;
  private: gpio_num_t mRxPin = GPIO_NUM_4 ;
  private: CANMode mMode = NormalMode ;
  private: uint8_t mBitRatePrescaler = 0 ;
  private: uint8_t mTimeSegment1 = 0 ;
  private: uint8_t mTimeSegment2 = 0 ;
  private: uint8_t mRJW = 0 ;

  private: twai_node_handle_t mNode = nullptr ;
  private: bool mNodeEnabled = false ;
  private: portMUX_TYPE mMux = portMUX_INITIALIZER_UNLOCKED ;

  private: void resetTxSlots (void) ;
  private: void storeRxFrame (const twai_frame_t & inFrame) ;

} ;
