//----------------------------------------------------------------------------------------------------------------------
// A CAN driver for MCP2517FD (CANFD mode)
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2517FD
//
//----------------------------------------------------------------------------------------------------------------------

#include <ACAN2517FDSettings.h>

//----------------------------------------------------------------------------------------------------------------------

#pragma GCC diagnostic error "-Wswitch-enum"

//----------------------------------------------------------------------------------------------------------------------
//    sysClock
//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::sysClock (const Oscillator inOscillator) {
  uint32_t sysClock = 40UL * 1000 * 1000 ;
  switch (inOscillator) {
  case OSC_4MHz:
    sysClock = 4UL * 1000 * 1000 ;
    break ;
  case OSC_4MHz_DIVIDED_BY_2:
    sysClock = 2UL * 1000 * 1000 ;
    break ;
  case OSC_4MHz10xPLL_DIVIDED_BY_2 :
  case OSC_40MHz_DIVIDED_BY_2:
  case OSC_20MHz:
    sysClock = 20UL * 1000 * 1000 ;
    break ;
  case OSC_20MHz_DIVIDED_BY_2:
    sysClock = 10UL * 1000 * 1000 ;
    break ;
  case OSC_4MHz10xPLL:
  case OSC_40MHz:
    break ;
  }
  return sysClock ;
}

//----------------------------------------------------------------------------------------------------------------------
//   CONSTRUCTOR
//----------------------------------------------------------------------------------------------------------------------

ACAN2517FDSettings::ACAN2517FDSettings (const Oscillator inOscillator,
                                        const uint32_t inDesiredArbitrationBitRate,
                                        const DataBitRateFactor inDataBitRateFactor,
                                        const uint32_t inTolerancePPM) :
mOscillator (inOscillator),
mSysClock (sysClock (inOscillator)),
mDesiredArbitrationBitRate (inDesiredArbitrationBitRate),
mDataBitRateFactor (inDataBitRateFactor) {
  if (inDataBitRateFactor == DataBitRateFactor::x1) { // Single bit rate
    const uint32_t maxTQCount = MAX_ARBITRATION_PHASE_SEGMENT_1 + MAX_ARBITRATION_PHASE_SEGMENT_2 + 1 ; // Setting for slowest bit rate
    uint32_t BRP = MAX_BRP ;
    uint32_t smallestError = UINT32_MAX ;
    uint32_t bestBRP = 1 ; // Setting for highest bit rate
    uint32_t bestTQCount = 4 ; // Setting for highest bit rate
    uint32_t TQCount = mSysClock / inDesiredArbitrationBitRate / BRP ;
  //--- Loop for finding best BRP and best TQCount
    while ((TQCount <= (MAX_ARBITRATION_PHASE_SEGMENT_1 + MAX_ARBITRATION_PHASE_SEGMENT_2 + 1)) && (BRP > 0)) {
    //--- Compute error using TQCount
      if ((TQCount >= 4) && (TQCount <= maxTQCount)) {
        const uint32_t error = mSysClock - inDesiredArbitrationBitRate * TQCount * BRP ; // error is always >= 0
        if (error <= smallestError) {
          smallestError = error ;
          bestBRP = BRP ;
          bestTQCount = TQCount ;
        }
      }
    //--- Compute error using TQCount+1
      if ((TQCount >= 3) && (TQCount < maxTQCount)) {
        const uint32_t error = inDesiredArbitrationBitRate * (TQCount + 1) * BRP - mSysClock ; // error is always >= 0
        if (error <= smallestError) {
          smallestError = error ;
          bestBRP = BRP ;
          bestTQCount = TQCount + 1 ;
        }
      }
    //--- Continue with next value of BRP
      BRP -- ;
      TQCount = (BRP == 0) ? (maxTQCount + 1) : (mSysClock / inDesiredArbitrationBitRate / BRP) ;
    }
  //--- Compute PS2 (1 <= PS2 <= 128)
    uint32_t PS2 = bestTQCount / 5 ; // For sampling point at 80%
    if (PS2 == 0) {
      PS2 = 1 ;
    }else if (PS2 > MAX_ARBITRATION_PHASE_SEGMENT_2) {
      PS2 = MAX_ARBITRATION_PHASE_SEGMENT_2 ;
    }
  //--- Compute PS1 (1 <= PS1 <= 256)
    uint32_t PS1 = bestTQCount - PS2 - 1 /* Sync Seg */ ;
    if (PS1 > MAX_ARBITRATION_PHASE_SEGMENT_1) {
      PS2 += PS1 - MAX_ARBITRATION_PHASE_SEGMENT_1 ;
      PS1 = MAX_ARBITRATION_PHASE_SEGMENT_1 ;
    }
  //---
    mBitRatePrescaler = (uint16_t) bestBRP ;
    mArbitrationPhaseSegment1 = (uint16_t) PS1 ;
    mArbitrationPhaseSegment2 = (uint8_t) PS2 ;
    mArbitrationSJW = mArbitrationPhaseSegment2 ; // Always 1 <= SJW <= 128, and SJW <= mArbitrationPhaseSegment2
  //--- Final check of the nominal configuration
    const uint32_t W = bestTQCount * mDesiredArbitrationBitRate * bestBRP ;
    const uint64_t diff = (mSysClock > W) ? (mSysClock - W) : (W - mSysClock) ;
    const uint64_t ppm = (uint64_t) (1000UL * 1000UL) ; // UL suffix is required for Arduino Uno
    mArbitrationBitRateClosedToDesiredRate = (diff * ppm) <= (((uint64_t) W) * inTolerancePPM) ;
  }else{ // Dual bit rate, first compute data bit rate
    const uint32_t maxDataTQCount = MAX_DATA_PHASE_SEGMENT_1 + MAX_DATA_PHASE_SEGMENT_2 ; // Setting for slowest bit rate
    const uint32_t desiredDataBitRate = inDesiredArbitrationBitRate * uint8_t (inDataBitRateFactor) ;
    uint32_t smallestError = UINT32_MAX ;
    uint32_t bestBRP = MAX_BRP ; // Setting for lowest bit rate
    uint32_t bestDataTQCount = maxDataTQCount ; // Setting for lowest bit rate
    uint32_t dataTQCount = 4 ;
    uint32_t brp = mSysClock / desiredDataBitRate / dataTQCount ;
  //--- Loop for finding best BRP and best TQCount
    while ((dataTQCount <= maxDataTQCount) && (brp > 0)) {
    //--- Compute error using brp
      if (brp <= MAX_BRP) {
        const uint32_t error = mSysClock - desiredDataBitRate * dataTQCount * brp ; // error is always >= 0
        if (error <= smallestError) {
          smallestError = error ;
          bestBRP = brp ;
          bestDataTQCount = dataTQCount ;
        }
      }
    //--- Compute error using brp+1
      if (brp < MAX_BRP) {
        const uint32_t error = desiredDataBitRate * dataTQCount * (brp + 1) - mSysClock ; // error is always >= 0
        if (error <= smallestError) {
          smallestError = error ;
          bestBRP = brp + 1 ;
          bestDataTQCount = dataTQCount ;
        }
      }
    //--- Continue with next value of BRP
      dataTQCount += 1 ;
      brp = mSysClock / desiredDataBitRate / dataTQCount ;
    }
  //--- Compute data PS2 (1 <= PS2 <= 16)
    uint32_t dataPS2 = bestDataTQCount / 5 ; // For sampling point at 80%
    if (dataPS2 == 0) {
      dataPS2 = 1 ;
    }
  //--- Compute data PS1 (1 <= PS1 <= 32)
    uint32_t dataPS1 = bestDataTQCount - dataPS2 - 1 /* Sync Seg */ ;
    if (dataPS1 > MAX_DATA_PHASE_SEGMENT_1) {
      dataPS2 += dataPS1 - MAX_DATA_PHASE_SEGMENT_1 ;
      dataPS1 = MAX_DATA_PHASE_SEGMENT_1 ;
    }
  //---
    if ((mDesiredArbitrationBitRate * uint32_t (inDataBitRateFactor)) <= (1000UL * 1000)) {
      mTDCO = 0 ;
    }else{
      const int TDCO = bestBRP * dataPS1 ; // According to DS20005678D, §3.4.8 Page 20
      mTDCO = (TDCO > 63) ? 63 : (int8_t) TDCO ;
    }
    mDataPhaseSegment1 = (uint8_t) dataPS1 ;
    mDataPhaseSegment2 = (uint8_t) dataPS2 ;
    mDataSJW = mDataPhaseSegment2 ;
    const uint32_t arbitrationTQCount = bestDataTQCount * uint8_t (mDataBitRateFactor) ;
  //--- Compute arbiration PS2 (1 <= PS2 <= 128)
    uint32_t arbitrationPS2 = arbitrationTQCount / 5 ; // For sampling point at 80%
    if (arbitrationPS2 == 0) {
      arbitrationPS2 = 1 ;
    }
  //--- Compute PS1 (1 <= PS1 <= 256)
    uint32_t arbitrationPS1 = arbitrationTQCount - arbitrationPS2 - 1 /* Sync Seg */ ;
    if (arbitrationPS1 > MAX_ARBITRATION_PHASE_SEGMENT_1) {
      arbitrationPS2 += arbitrationPS1 - MAX_ARBITRATION_PHASE_SEGMENT_1 ;
      arbitrationPS1 = MAX_ARBITRATION_PHASE_SEGMENT_1 ;
    }
  //---
    mBitRatePrescaler = (uint16_t) bestBRP ;
    mArbitrationPhaseSegment1 = (uint16_t) arbitrationPS1 ;
    mArbitrationPhaseSegment2 = (uint8_t) arbitrationPS2 ;
    mArbitrationSJW = mArbitrationPhaseSegment2 ; // Always 1 <= SJW <= 128, and SJW <= mArbitrationPhaseSegment2
  //--- Final check of the nominal configuration
    const uint32_t W = arbitrationTQCount * mDesiredArbitrationBitRate * bestBRP ;
    const uint64_t diff = (mSysClock > W) ? (mSysClock - W) : (W - mSysClock) ;
    const uint64_t ppm = (uint64_t) (1000UL * 1000UL) ; // UL suffix is required for Arduino Uno
    mArbitrationBitRateClosedToDesiredRate = (diff * ppm) <= (((uint64_t) W) * inTolerancePPM) ;
  }
} ;

//----------------------------------------------------------------------------------------------------------------------
//   ACCESSORS
//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::actualArbitrationBitRate (void) const {
  const uint32_t arbitrationTQCount = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 + mArbitrationPhaseSegment2 ;
  return mSysClock / mBitRatePrescaler / arbitrationTQCount ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::actualDataBitRate (void) const {
  if (mDataBitRateFactor == DataBitRateFactor::x1) {
    return actualArbitrationBitRate () ;
  }else{
    const uint32_t dataTQCount = 1 /* Sync Seg */ + mDataPhaseSegment1 + mDataPhaseSegment2 ;
    return mSysClock / mBitRatePrescaler / dataTQCount ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FDSettings::exactArbitrationBitRate (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 + mArbitrationPhaseSegment2 ;
  return mSysClock == (mBitRatePrescaler * mDesiredArbitrationBitRate * TQCount) ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FDSettings::exactDataBitRate (void) const {
  if (mDataBitRateFactor == DataBitRateFactor::x1) {
    return exactArbitrationBitRate () ;
  }else{
    const uint32_t TQCount = 1 /* Sync Seg */ + mDataPhaseSegment1 + mDataPhaseSegment2 ;
    return mSysClock == (mBitRatePrescaler * mDesiredArbitrationBitRate * TQCount * uint8_t (mDataBitRateFactor)) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FDSettings::dataBitRateIsAMultipleOfArbitrationBitRate (void) const {
  bool result = mDataBitRateFactor == DataBitRateFactor::x1 ;
  if (!result) {
    const uint32_t dataTQCount = 1 /* Sync Seg */ + mDataPhaseSegment1 + mDataPhaseSegment2 ;
    const uint32_t arbitrationTQCount = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 + mArbitrationPhaseSegment2 ;
    result = arbitrationTQCount == (dataTQCount * uint8_t (mDataBitRateFactor)) ;
  }
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::ppmFromDesiredArbitrationBitRate (void) const {
  const uint32_t TQCount = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 + mArbitrationPhaseSegment2 ;
  const uint32_t W = TQCount * mDesiredArbitrationBitRate * mBitRatePrescaler ;
  const uint64_t diff = (mSysClock > W) ? (mSysClock - W) : (W - mSysClock) ;
  const uint64_t ppm = (uint64_t) (1000UL * 1000UL) ; // UL suffix is required for Arduino Uno
  return (uint32_t) ((diff * ppm) / W) ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::arbitrationSamplePointFromBitStart (void) const {
  const uint32_t nominalTQCount = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 + mArbitrationPhaseSegment2 ;
  const uint32_t samplePoint = 1 /* Sync Seg */ + mArbitrationPhaseSegment1 ;
  const uint32_t partPerCent = 100 ;
  return (samplePoint * partPerCent) / nominalTQCount ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::dataSamplePointFromBitStart (void) const {
  const uint32_t nominalTQCount = 1 /* Sync Seg */ + mDataPhaseSegment1 + mDataPhaseSegment2 ;
  const uint32_t samplePoint = 1 /* Sync Seg */ + mDataPhaseSegment1 ;
  const uint32_t partPerCent = 100 ;
  return (samplePoint * partPerCent) / nominalTQCount ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::CANBitSettingConsistency (void) const {
  uint32_t errorCode = 0 ; // Means no error
//--- Bit rate prescaler
  if (mBitRatePrescaler == 0) {
    errorCode |= kBitRatePrescalerIsZero ;
  }else if (mBitRatePrescaler > MAX_BRP) {
    errorCode |= kBitRatePrescalerIsGreaterThan256 ;
  }
//--- Arbitration Phase Segment 1
  if (mArbitrationPhaseSegment1 < 2) {
    errorCode |= kArbitrationPhaseSegment1IsLowerThan2 ;
  }else if (mArbitrationPhaseSegment1 > MAX_ARBITRATION_PHASE_SEGMENT_1) {
    errorCode |= kArbitrationPhaseSegment1IsGreaterThan256 ;
  }
//--- Arbitration Phase Segment 2
  if (mArbitrationPhaseSegment2 == 0) {
    errorCode |= kArbitrationPhaseSegment2IsZero ;
  }else if (mArbitrationPhaseSegment2 > MAX_ARBITRATION_PHASE_SEGMENT_2) {
    errorCode |= kArbitrationPhaseSegment2IsGreaterThan128 ;
  }
//--- Arbitration SJW
  if (mArbitrationSJW == 0) {
    errorCode |= kArbitrationSJWIsZero ;
  }else if (mArbitrationSJW > MAX_ARBITRATION_SJW) {
    errorCode |= kArbitrationSJWIsGreaterThan128 ;
  }
  if (mArbitrationSJW > mArbitrationPhaseSegment1) {
    errorCode |= kArbitrationSJWIsGreaterThanPhaseSegment1 ;
  }
  if (mArbitrationSJW > mArbitrationPhaseSegment2) {
    errorCode |= kArbitrationSJWIsGreaterThanPhaseSegment2 ;
  }
//--- Data bit rate ?
  if (mDataBitRateFactor != DataBitRateFactor::x1) {
    if (! dataBitRateIsAMultipleOfArbitrationBitRate ()) {
      errorCode |= kArbitrationTQCountNotDivisibleByDataBitRateFactor ;
    }
  //--- Data Phase Segment 1
    if (mDataPhaseSegment1 < 2) {
      errorCode |= kDataPhaseSegment1IsLowerThan2 ;
    }else if (mDataPhaseSegment1 > MAX_DATA_PHASE_SEGMENT_1) {
      errorCode |= kDataPhaseSegment1IsGreaterThan32 ;
    }
  //--- Data Phase Segment 2
    if (mDataPhaseSegment2 == 0) {
      errorCode |= kDataPhaseSegment2IsZero ;
    }else if (mDataPhaseSegment2 > MAX_DATA_PHASE_SEGMENT_2) {
      errorCode |= kDataPhaseSegment2IsGreaterThan16 ;
    }
  //--- Data SJW
    if (mDataSJW == 0) {
      errorCode |= kDataSJWIsZero ;
    }else if (mDataSJW > MAX_DATA_SJW) {
      errorCode |= kDataSJWIsGreaterThan16 ;
    }
    if (mDataSJW > mDataPhaseSegment1) {
      errorCode |= kDataSJWIsGreaterThanPhaseSegment1 ;
    }
    if (mDataSJW > mDataPhaseSegment2) {
      errorCode |= kDataSJWIsGreaterThanPhaseSegment2 ;
    }
  }
//---
  return errorCode ;
}

//----------------------------------------------------------------------------------------------------------------------
//   RAM USAGE
//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::ramUsage (void) const {
  uint32_t result = 0 ;
//--- TXQ
  result += objectSizeForPayload (mControllerTXQBufferPayload) * mControllerTXQSize ;
//--- Receive FIFO (FIFO #1)
  result += objectSizeForPayload (mControllerReceiveFIFOPayload) * mControllerReceiveFIFOSize ;
//--- Send FIFO (FIFO #2)
  result += objectSizeForPayload (mControllerTransmitFIFOPayload) * mControllerTransmitFIFOSize ;
//---
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FDSettings::objectSizeForPayload (const PayloadSize inPayload) {
  static const uint8_t kPayload [8] = {16, 20, 24, 28, 32, 40, 56, 72} ;
  return kPayload [inPayload] ;
}

//----------------------------------------------------------------------------------------------------------------------
