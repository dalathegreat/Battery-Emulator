//----------------------------------------------------------------------------------------
// Include files
//----------------------------------------------------------------------------------------

#include "ACAN_ESP32_Settings.h"

//----------------------------------------------------------------------------------------
//    CAN Settings
//----------------------------------------------------------------------------------------

ACAN_ESP32_Settings::ACAN_ESP32_Settings (const uint32_t inDesiredBitRate,
                                          const uint32_t inTolerancePPM) :
mDesiredBitRate (inDesiredBitRate) {
  uint32_t TQCount = MAX_TQ ;             // TQ: min(3) max(25)
  uint32_t bestBRP = MAX_BRP ;            // Setting for slowest bit rate
  uint32_t bestTQCount = MAX_TQ ;         // Setting for slowest bit rate
  uint32_t smallestError = UINT32_MAX ;
  const uint32_t CANClock = CAN_CLOCK () ;
  uint32_t BRP = CANClock / (inDesiredBitRate * TQCount) ; // BRP: min(2) max(128)
//--- Loop for finding best BRP and best TQCount
  while ((TQCount >= MIN_TQ) && (BRP <= MAX_BRP)) {
  //--- Compute error using BRP (caution: BRP should be > 0)
    if (BRP >= MIN_BRP) {
      const uint32_t error = CANClock - (inDesiredBitRate * TQCount * BRP) ; // error is always >= 0
      if (error < smallestError) {
        smallestError = error ;
        bestBRP = BRP ;
        bestTQCount = TQCount ;
      }
    }
  //--- Compute error using BRP+1 (caution: BRP+1 should be <= 128)
    if (BRP < MAX_BRP) {
      const uint32_t error = (inDesiredBitRate * TQCount * (BRP + 1)) - CANClock ; // error is always >= 0
      if (error < smallestError) {
        smallestError = error ;
        bestBRP = BRP + 1 ;
        bestTQCount = TQCount ;
      }
    }
  //--- Continue with next value of TQCount
    TQCount -= 1 ;
    BRP = CANClock / (inDesiredBitRate * TQCount) ;
  }
//--- Set the BRP
  mBitRatePrescaler = uint8_t (bestBRP) ;
//--- Compute PS2 (2 <= TSeg2 <= 8)
  const uint32_t PS2 = 2 + 3 * (bestTQCount - 5) / 10 ;
  mTimeSegment2 = uint8_t (PS2) ;
//--- Compute PS1 (1 <= PS1 <= 16)
  const uint32_t PS1 = bestTQCount - PS2 - SYNC_SEGMENT ;
  mTimeSegment1 = uint8_t (PS1) ;
//--- SJW (1...4) min of Tseg2
  mRJW = (mTimeSegment2 > 4) ? 4 : mTimeSegment2 ;
//--- Triple sampling ?
  mTripleSampling = (inDesiredBitRate <= 125000) && (mTimeSegment2 > 2) ;
//--- Final check of the configuration
  const uint32_t W = bestTQCount * mDesiredBitRate * mBitRatePrescaler ;
  const uint64_t diff = (CANClock > W) ? (CANClock - W) : (W - CANClock) ;
  const uint64_t ppm = uint64_t (1000UL * 1000UL) ;
  mBitRateClosedToDesiredRate = (diff * ppm) <= (uint64_t (W) * inTolerancePPM) ;
}

//----------------------------------------------------------------------------------------

uint32_t ACAN_ESP32_Settings::actualBitRate (void) const {
  const uint32_t TQCount = SYNC_SEGMENT + mTimeSegment1 + mTimeSegment2 ;
  return CAN_CLOCK () / mBitRatePrescaler / TQCount ;
}

//----------------------------------------------------------------------------------------

bool ACAN_ESP32_Settings::exactBitRate (void) const {
  const uint32_t TQCount = SYNC_SEGMENT + mTimeSegment1 + mTimeSegment2 ;
  return CAN_CLOCK () == (mDesiredBitRate * mBitRatePrescaler * TQCount) ;
}

//----------------------------------------------------------------------------------------

uint32_t ACAN_ESP32_Settings::ppmFromDesiredBitRate(void) const {
  const uint32_t TQCount = SYNC_SEGMENT + mTimeSegment1 + mTimeSegment2 ;
  const uint32_t W = TQCount * mDesiredBitRate * mBitRatePrescaler ;
  const uint32_t CANClock = CAN_CLOCK () ;
  const uint64_t diff = (CANClock > W) ? (CANClock - W) : (W - CANClock) ;
  const uint64_t ppm = (uint64_t)(1000UL * 1000UL) ; // UL suffix is required for Arduino Uno
  return uint32_t ((diff * ppm) / W) ;
}

//----------------------------------------------------------------------------------------

uint32_t ACAN_ESP32_Settings::samplePointFromBitStart (void) const {
  const uint32_t TQCount = SYNC_SEGMENT + mTimeSegment1 + mTimeSegment2 ;
  const uint32_t samplePoint = SYNC_SEGMENT + mTimeSegment1 - mTripleSampling ;
  const uint32_t partPerCent = 100 ;
  return (samplePoint * partPerCent) / TQCount ;
}

//----------------------------------------------------------------------------------------

uint16_t ACAN_ESP32_Settings::CANBitSettingConsistency (void) const {
  uint16_t errorCode = 0 ; // No error
  if (mBitRatePrescaler < MIN_BRP) {
    errorCode |= kBitRatePrescalerIsZero ;
  }else if (mBitRatePrescaler > MAX_BRP) {
    errorCode |= kBitRatePrescalerIsGreaterThan64 ;
  }
  if (mTimeSegment1 == 0) {
    errorCode |= kTimeSegment1IsZero ;
  }else if ((mTimeSegment2 == 2) && mTripleSampling) {
    errorCode |= kTimeSegment2Is2AndTripleSampling ;
  }else if (mTimeSegment1 > MAX_TIME_SEGMENT_1) {
    errorCode |= kTimeSegment1IsGreaterThan16 ;
  }
  if (mTimeSegment2 < 2) {
    errorCode |= kTimeSegment2IsLowerThan2 ;
  }else if (mTimeSegment2 > MAX_TIME_SEGMENT_2) {
    errorCode |= kTimeSegment2IsGreaterThan8 ;
  }
  if (mRJW == 0) {
    errorCode |= kRJWIsZero ;
  }else if (mRJW > mTimeSegment2) {
    errorCode |= kRJWIsGreaterThanTimeSegment2 ;
  }else if (mRJW > MAX_SJW) {
    errorCode |= kRJWIsGreaterThan4 ;
  }
  return errorCode ;
}

//----------------------------------------------------------------------------------------
