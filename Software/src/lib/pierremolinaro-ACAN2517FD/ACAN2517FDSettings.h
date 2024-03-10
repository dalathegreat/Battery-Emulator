//----------------------------------------------------------------------------------------------------------------------
// A CAN driver for MCP2517FD (CANFD mode)
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2517FD
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------------------------

#include "ACANFD_DataBitRateFactor.h"

//----------------------------------------------------------------------------------------------------------------------
//  ACAN2517FDSettings class
//----------------------------------------------------------------------------------------------------------------------

class ACAN2517FDSettings {

//······················································································································
//   ENUMERATED TYPES
//······················································································································

  public: typedef enum : uint8_t {
    OSC_4MHz,
    OSC_4MHz_DIVIDED_BY_2,
    OSC_4MHz10xPLL,
    OSC_4MHz10xPLL_DIVIDED_BY_2,
    OSC_20MHz,
    OSC_20MHz_DIVIDED_BY_2,
    OSC_40MHz,
    OSC_40MHz_DIVIDED_BY_2
  } Oscillator ;

  public: typedef enum : uint8_t {
    CLKO_DIVIDED_BY_1,
    CLKO_DIVIDED_BY_2,
    CLKO_DIVIDED_BY_4,
    CLKO_DIVIDED_BY_10,
    SOF
  } CLKOpin ;

  public: typedef enum : uint8_t {
    NormalFD = 0,
    Sleep = 1,
    InternalLoopBack = 2,
    ListenOnly = 3,
    Configuration = 4,
    ExternalLoopBack = 5,
    Normal20B = 6,
    RestrictedOperation = 7
  } OperationMode ;

  public: typedef enum : uint8_t {Disabled, ThreeAttempts, UnlimitedNumber} RetransmissionAttempts ;

  public: typedef enum : uint8_t {
    PAYLOAD_8  = 0,
    PAYLOAD_12 = 1,
    PAYLOAD_16 = 2,
    PAYLOAD_20 = 3,
    PAYLOAD_24 = 4,
    PAYLOAD_32 = 5,
    PAYLOAD_48 = 6,
    PAYLOAD_64 = 7
  } PayloadSize ;

//······················································································································
//   Deprecated enumeration (now use DataBitRateFactor declared in ACANFD_DataBitRateFactor.h)
//······················································································································

  public : typedef enum : uint8_t {
      DATA_BITRATE_x1 = 1,
      DATA_BITRATE_x2 = 2,
      DATA_BITRATE_x3 = 3,
      DATA_BITRATE_x4 = 4,
      DATA_BITRATE_x5 = 5,
      DATA_BITRATE_x6 = 6,
      DATA_BITRATE_x7 = 7,
      DATA_BITRATE_x8 = 8,
      DATA_BITRATE_x9 = 9,
      DATA_BITRATE_x10 = 10
  } DataBitRateFactor_Deprecated ;

//······················································································································
//   CONSTRUCTOR
//······················································································································

  public: ACAN2517FDSettings (const Oscillator inOscillator,
                              const uint32_t inDesiredArbitrationBitRate,
                              const DataBitRateFactor inDataBitRateFactor,
                              const uint32_t inTolerancePPM = 1000) ;

//······················································································································
//   DEPRECATED CONSTRUCTOR (for compatibility with version < 2.1.0)
//······················································································································

  public: ACAN2517FDSettings (const Oscillator inOscillator,
                              const uint32_t inDesiredArbitrationBitRate,
                              const DataBitRateFactor_Deprecated inDataBitRateFactor,
                              const uint32_t inTolerancePPM = 1000) :
  ACAN2517FDSettings (inOscillator, inDesiredArbitrationBitRate, DataBitRateFactor (inDataBitRateFactor), inTolerancePPM) {
  }

//······················································································································
//   CAN BIT TIMING
//······················································································································

  private: Oscillator mOscillator ;
  private: uint32_t mSysClock ; // In Hz
  public: const uint32_t mDesiredArbitrationBitRate ; // In kb/s
  public: const DataBitRateFactor mDataBitRateFactor ;
//--- Data bit rate; if mDataBitRateFactor==1, theses properties are not used for configuring the MCP2517FD.
  public: uint8_t mDataPhaseSegment1 = 0 ; // if mDataBitRateFactor > 1: 2...32, else equal to mArbitrationPhaseSegment1
  public: uint8_t mDataPhaseSegment2 = 0 ; // if mDataBitRateFactor > 1: 1...16, else equal to mArbitrationPhaseSegment2
  public: uint8_t mDataSJW = 0 ; // if mDataBitRateFactor > 1: 1...16, else equal to mArbitrationSJW
//--- Bit rate prescaler is common to arbitration and data bit rates
  public: uint16_t mBitRatePrescaler = 0 ; // 1...256
//--- Arbitration bit rate
  public: uint16_t mArbitrationPhaseSegment1 = 0 ; // 2...256
  public: uint8_t mArbitrationPhaseSegment2 = 0 ; // 1...128
  public: uint8_t mArbitrationSJW = 0 ; // 1...128
  public: bool mArbitrationBitRateClosedToDesiredRate = false ; // The above configuration is not correct
//--- Transmitter Delay Compensation Offset
  public: int8_t mTDCO = 0 ; // -64 ... +63

//······················································································································
//    MCP2517FD TXCAN pin is Open Drain ?
//······················································································································

  public: bool mTXCANIsOpenDrain = false ; // false --> Push/Pull Output, true --> Open Drain Output

//······················································································································
//    MCP2517FD INT pin is Open Drain ?
//······················································································································

  public: bool mINTIsOpenDrain = false ; // false --> Push/Pull Output, true --> Open Drain Output

//······················································································································
//    ISO CRC Enable
//······················································································································

// false --> Do NOT include Stuff Bit Count in CRC Field and use CRC Initialization Vector with all zeros
// true --> Include Stuff Bit Count in CRC Field and use Non-Zero CRC Initialization Vector according to ISO 11898-1:2015
  public: bool mISOCRCEnabled = true ;

//······················································································································
//    CLKO pin function (default value is MCP2517FD power on setting)
//······················································································································

  public: CLKOpin mCLKOPin = CLKO_DIVIDED_BY_10 ;

//······················································································································
//    Requested mode
//······················································································································

  public: OperationMode mRequestedMode = NormalFD ;

//······················································································································
//   TRANSMIT FIFO
//······················································································································

//--- Driver transmit buffer size
  public: uint16_t mDriverTransmitFIFOSize = 16 ; // >= 0

//--- Controller transmit FIFO size
  public: uint8_t mControllerTransmitFIFOSize = 1 ; // 1 ... 32

//--- Payload controller transmit FIFO size
  public: PayloadSize mControllerTransmitFIFOPayload = PAYLOAD_64 ;

//--- Controller transmit FIFO priority (0 --> lowest, 31 --> highest)
  public: uint8_t mControllerTransmitFIFOPriority = 0 ; // 0 ... 31

//--- Controller transmit FIFO retransmission attempts
  public: RetransmissionAttempts mControllerTransmitFIFORetransmissionAttempts = UnlimitedNumber ;

//······················································································································
//   TXQ BUFFER
//······················································································································

//--- TXQ buffer size (0 --> TXQ disabled)
  public: uint8_t mControllerTXQSize = 0 ; // 0 ... 32

//--- Payload controller TXQ buffer size
  public: PayloadSize mControllerTXQBufferPayload = PAYLOAD_64 ;

//--- TXQ buffer priority (0 --> lowest, 31 --> highest)
  public: uint8_t mControllerTXQBufferPriority = 31 ; // 0 ... 31

//--- Controller TXQ buffer retransmission attempts
  public: RetransmissionAttempts mControllerTXQBufferRetransmissionAttempts = UnlimitedNumber ;


//······················································································································
//   RECEIVE FIFO
//······················································································································

//--- Driver receive buffer size
  public: uint16_t mDriverReceiveFIFOSize = 32 ; // > 0

//--- Payload receive FIFO size
  public: PayloadSize mControllerReceiveFIFOPayload = PAYLOAD_64 ;

//--- Controller receive FIFO size
  public: uint8_t mControllerReceiveFIFOSize = 27 ; // 1 ... 32

//······················································································································
//    SYSCLOCK frequency computation
//······················································································································

  public: static uint32_t sysClock (const Oscillator inOscillator) ;

//······················································································································
//    Accessors
//······················································································································

  public: Oscillator oscillator (void) const { return mOscillator ; }
  public: uint32_t sysClock (void) const { return mSysClock ; }
  public: uint32_t actualArbitrationBitRate (void) const ;
  public: uint32_t actualDataBitRate (void) const ;
  public: bool exactArbitrationBitRate (void) const ;
  public: bool exactDataBitRate (void) const ;
  public: bool dataBitRateIsAMultipleOfArbitrationBitRate (void) const ;

//······················································································································
//    RAM USAGE
//······················································································································

  public: uint32_t ramUsage (void) const ;

  public: static uint32_t objectSizeForPayload (const PayloadSize inPayload) ;

//······················································································································
//    Distance between actual bit rate and requested bit rate (in ppm, part-per-million)
//······················································································································

  public: uint32_t ppmFromDesiredArbitrationBitRate (void) const ;
  public: uint32_t ppmFromDesiredDataBitRate (void) const ;

//······················································································································
//    Distance of sample point from bit start (in ppc, part-per-cent, denoted by %)
//······················································································································

  public: uint32_t arbitrationSamplePointFromBitStart (void) const ;
  public: uint32_t dataSamplePointFromBitStart (void) const ;

//······················································································································
//    Bit settings are consistent ? (returns 0 if ok)
//······················································································································

  public: uint32_t CANBitSettingConsistency (void) const ;

//······················································································································
//    Constants returned by CANBitSettingConsistency
//······················································································································

  public: static const uint32_t kBitRatePrescalerIsZero                 = ((uint32_t) 1) <<  0 ;
  public: static const uint32_t kBitRatePrescalerIsGreaterThan256       = ((uint32_t) 1) <<  1 ;
  public: static const uint32_t kArbitrationPhaseSegment1IsLowerThan2       = ((uint32_t) 1) <<  2 ;
  public: static const uint32_t kArbitrationPhaseSegment1IsGreaterThan256   = ((uint32_t) 1) <<  3 ;
  public: static const uint32_t kArbitrationPhaseSegment2IsZero             = ((uint32_t) 1) <<  4 ;
  public: static const uint32_t kArbitrationPhaseSegment2IsGreaterThan128   = ((uint32_t) 1) <<  5 ;
  public: static const uint32_t kArbitrationSJWIsZero                       = ((uint32_t) 1) <<  6 ;
  public: static const uint32_t kArbitrationSJWIsGreaterThan128             = ((uint32_t) 1) <<  7 ;
  public: static const uint32_t kArbitrationSJWIsGreaterThanPhaseSegment1   = ((uint32_t) 1) <<  8 ;
  public: static const uint32_t kArbitrationSJWIsGreaterThanPhaseSegment2   = ((uint32_t) 1) <<  9 ;
  public: static const uint32_t kArbitrationTQCountNotDivisibleByDataBitRateFactor = ((uint32_t) 1) << 10 ;
  public: static const uint32_t kDataPhaseSegment1IsLowerThan2          = ((uint32_t) 1) << 11 ;
  public: static const uint32_t kDataPhaseSegment1IsGreaterThan32       = ((uint32_t) 1) << 12 ;
  public: static const uint32_t kDataPhaseSegment2IsZero                = ((uint32_t) 1) << 13 ;
  public: static const uint32_t kDataPhaseSegment2IsGreaterThan16       = ((uint32_t) 1) << 14 ;
  public: static const uint32_t kDataSJWIsZero                          = ((uint32_t) 1) << 15 ;
  public: static const uint32_t kDataSJWIsGreaterThan16                 = ((uint32_t) 1) << 16 ;
  public: static const uint32_t kDataSJWIsGreaterThanPhaseSegment1      = ((uint32_t) 1) << 17 ;
  public: static const uint32_t kDataSJWIsGreaterThanPhaseSegment2      = ((uint32_t) 1) << 18 ;

//······················································································································
// Max values
//······················································································································

  public: static const uint16_t MAX_BRP = 256 ;

  public: static const uint16_t MAX_ARBITRATION_PHASE_SEGMENT_1 = 256 ;
  public: static const uint8_t  MAX_ARBITRATION_PHASE_SEGMENT_2 = 128 ;
  public: static const uint8_t  MAX_ARBITRATION_SJW             = 128 ;

  public: static const uint16_t MAX_DATA_PHASE_SEGMENT_1 = 32 ;
  public: static const uint8_t  MAX_DATA_PHASE_SEGMENT_2 = 16 ;
  public: static const uint8_t  MAX_DATA_SJW             = 16 ;

//······················································································································

} ;

//----------------------------------------------------------------------------------------------------------------------

