//··································································································
// A CAN driver for MCP2515
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2515
//··································································································

#pragma once

//··································································································

#include <stdint.h>

//··································································································

class ACAN2515Settings {

//··································································································
// Enumerations
//··································································································

  public: typedef enum : uint8_t {
      NormalMode = 0 << 5,
      SleepMode  = 1 << 5,
      LoopBackMode = 2 << 5,
      ListenOnlyMode = 3 << 5
  } RequestedMode ;

//··································································································

  public: typedef enum : uint8_t {CLOCK, CLOCK2, CLOCK4, CLOCK8, SOF, HiZ} CLKOUT_SOF ;

//··································································································
//   Constructor for a given baud rate
//··································································································

  public: explicit ACAN2515Settings (const uint32_t inQuartzFrequency, // In Hertz
                                     const uint32_t inDesiredBitRate,
                                     const uint32_t inTolerancePPM = 1000) ;
//··································································································
//   Constructor with explicit bit settings
//··································································································

  public: explicit ACAN2515Settings (const uint32_t inQuartzFrequency, // In Hertz
                                     const uint8_t inBitRatePrescaler,  // 1...64
                                     const uint8_t inPropagationSegment, // 1...8
                                     const uint8_t inPhaseSegment1, // 1...8
                                     const uint8_t inPhaseSegment2, // 2...8
                                     const uint8_t inSJW) ; // 1...4

//··································································································
//   CAN bit timing properties
//··································································································

  public: const uint32_t mQuartzFrequency ;
  public: uint32_t mDesiredBitRate = mQuartzFrequency / 64 ; // In kb/s
  public: uint8_t mPropagationSegment = 5 ; // 1...8
  public: uint8_t mPhaseSegment1 = 5 ; // 1...8
  public: uint8_t mPhaseSegment2 = 5 ; // 2...8
  public: uint8_t mSJW = 4 ; // 1...4
  public: uint8_t mBitRatePrescaler = 32 / (1 + mPropagationSegment + mPhaseSegment1 + mPhaseSegment2) ; // 1...64
  public: bool mTripleSampling = false ; // true --> triple sampling, false --> single sampling
  public: bool mBitRateClosedToDesiredRate = true ; // The above configuration is correct


//··································································································
//   One shot mode
//      true --> Enabled; messages will only attempt to transmit one time
//      false --> Disabled; messages will reattempt transmission if required
//··································································································

  public: bool mOneShotModeEnabled = false ;


//··································································································
//    MCP2515 TXBi priorities
//       bits 7-6: unused
//       bits 5-4: TXB2 priority
//       bits 3-2: TXB1 priority
//       bits 1-0: TXB0 priority
//··································································································

  public: uint8_t mTXBPriority = 0 ;


//··································································································
//   Requested mode
//··································································································

  public: RequestedMode mRequestedMode = NormalMode ;


//··································································································
//   Signal on CLKOUT/SOF pin
//··································································································

  public: CLKOUT_SOF mCLKOUT_SOF_pin = CLOCK ;


//··································································································
//    Rollover Enable Bit (is set to the BUKT bit of the RXB0CTRL register)
//       true  --> RXB0 message will roll over and be written to RXB1 if RXB0 is full
//       false --> Rollover is disabled
//··································································································

  public : bool mRolloverEnable = true ;


//··································································································
//    Receive buffer size
//··································································································

  public: uint16_t mReceiveBufferSize = 32 ;


//··································································································
//    Transmit buffer sizes
//··································································································

  public: uint16_t mTransmitBuffer0Size = 16 ;
  public: uint16_t mTransmitBuffer1Size = 0 ;
  public: uint16_t mTransmitBuffer2Size = 0 ;


//··································································································
//    Compute actual bit rate
//··································································································

  public: uint32_t actualBitRate (void) const ;


//··································································································
//    Exact bit rate ?
//··································································································

  public: bool exactBitRate (void) const ;


//··································································································
//    Distance between actual bit rate and requested bit rate (in ppm, part-per-million)
//··································································································

  public: uint32_t ppmFromDesiredBitRate (void) const ;


//··································································································
//    Distance of sample point from bit start (in ppc, part-per-cent, denoted by %)
//··································································································

  public: uint32_t samplePointFromBitStart (void) const ;


//··································································································
//    Bit settings are consistent ? (returns 0 if ok)
//··································································································

  public: uint16_t CANBitSettingConsistency (void) const ;


//··································································································
//    Constants returned by CANBitSettingConsistency
//··································································································

  public: static const uint16_t kBitRatePrescalerIsZero                 = 1 <<  0 ;
  public: static const uint16_t kBitRatePrescalerIsGreaterThan64        = 1 <<  1 ;
  public: static const uint16_t kPropagationSegmentIsZero               = 1 <<  2 ;
  public: static const uint16_t kPropagationSegmentIsGreaterThan8       = 1 <<  3 ;
  public: static const uint16_t kPhaseSegment1IsZero                    = 1 <<  4 ;
  public: static const uint16_t kPhaseSegment1IsGreaterThan8            = 1 <<  5 ;
  public: static const uint16_t kPhaseSegment2IsLowerThan2              = 1 <<  6 ;
  public: static const uint16_t kPhaseSegment2IsGreaterThan8            = 1 <<  7 ;
  public: static const uint16_t kPhaseSegment1Is1AndTripleSampling      = 1 <<  8 ;
  public: static const uint16_t kSJWIsZero                              = 1 <<  9 ;
  public: static const uint16_t kSJWIsGreaterThan4                      = 1 << 10 ;
  public: static const uint16_t kSJWIsGreaterThanOrEqualToPhaseSegment2 = 1 << 11 ;
  public: static const uint16_t kPhaseSegment2IsGreaterThanPSPlusPS1    = 1 << 12 ;

//··································································································

} ;

//··································································································

