//----------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------
//  Include files
//----------------------------------------------------------------------------------------

#include <stdint.h>

//--- For getting getApbFrequency function declaration
#ifdef ARDUINO
  #include <Arduino.h>
#endif

//----------------------------------------------------------------------------------------
//  CAN CLOCK
//----------------------------------------------------------------------------------------

#ifdef ARDUINO
  inline uint32_t CAN_CLOCK (void) { return getApbFrequency () / 2 ; }// APB_CLK_FREQ: 80 MHz APB CLOCK
#else
  inline uint32_t CAN_CLOCK (void) { return 40 * 1000 * 1000 ; } // 40 MHz
#endif

//----------------------------------------------------------------------------------------
//  ESP32 ACANSettings class
//----------------------------------------------------------------------------------------

class ACAN_ESP32_Settings {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   ENUMERATED TYPE
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //--- CAN driver operating modes
    public: typedef enum
    #ifdef ARDUINO
      : uint8_t
    #endif
      {
        NormalMode,
        ListenOnlyMode,
        LoopBackMode
    } CANMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CONSTRUCTOR
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: ACAN_ESP32_Settings (const uint32_t inDesiredBitRate,
                                 const uint32_t inTolerancePPM = 1000) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CAN PINS
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #ifdef ARDUINO
    public: gpio_num_t mTxPin = GPIO_NUM_5 ;
    public: gpio_num_t mRxPin = GPIO_NUM_4 ;
  #endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CAN BIT TIMING
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint32_t mDesiredBitRate ;                 // In kb/s
    public: uint8_t mBitRatePrescaler = 0 ;            // 1...64
    public: uint8_t mTimeSegment1 = 0 ;                // 1...16
    public: uint8_t mTimeSegment2 = 0 ;                // 2...8
    public: uint8_t mRJW = 0 ;                         // 1...4
    public: bool mTripleSampling = false ;             // true --> triple sampling, false --> single sampling
    public: bool mBitRateClosedToDesiredRate = false ; // The above configuration is correct

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Max values
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: static const uint8_t SYNC_SEGMENT       = 1 ; // Fixed Sync Segment
    public: static const uint32_t MAX_BRP           = 64 ;
    public: static const uint32_t MIN_BRP           = 1 ;
    public: static const uint32_t MAX_TQ            = 25 ;
    public: static const uint32_t MIN_TQ            = 5 ;
    public: static const uint8_t MAX_TIME_SEGMENT_1 = 16 ;
    public: static const uint8_t MAX_TIME_SEGMENT_2 = 8 ;
    public: static const uint8_t MAX_SJW            = 4 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   Requested mode
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: CANMode mRequestedCANMode = NormalMode ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Receive buffer size
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint16_t mDriverReceiveBufferSize = 32 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Transmit buffer sizes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint16_t mDriverTransmitBufferSize = 16 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Compute actual bit rate
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint32_t actualBitRate (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Exact bit rate ?
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: bool exactBitRate (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Distance between actual bit rate and requested bit rate (in ppm, part-per-million)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint32_t ppmFromDesiredBitRate (void) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Distance of sample point from bit start (in ppc, part-per-cent, denoted by %)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint32_t samplePointFromBitStart (void) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Bit settings are consistent ? (returns 0 if ok)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: uint16_t CANBitSettingConsistency (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Constants returned by CANBitSettingConsistency
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    public: static const uint16_t kBitRatePrescalerIsZero           = 1 <<  0 ;
    public: static const uint16_t kBitRatePrescalerIsGreaterThan64  = 1 <<  1 ;
    public: static const uint16_t kTimeSegment1IsZero               = 1 <<  2 ;
    public: static const uint16_t kTimeSegment1IsGreaterThan16      = 1 <<  3 ;
    public: static const uint16_t kTimeSegment2IsLowerThan2         = 1 <<  4 ;
    public: static const uint16_t kTimeSegment2IsGreaterThan8       = 1 <<  5 ;
    public: static const uint16_t kTimeSegment2Is2AndTripleSampling = 1 <<  6 ;
    public: static const uint16_t kRJWIsZero                        = 1 <<  7 ;
    public: static const uint16_t kRJWIsGreaterThan4                = 1 <<  8 ;
    public: static const uint16_t kRJWIsGreaterThanTimeSegment2     = 1 <<  9 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//----------------------------------------------------------------------------------------
