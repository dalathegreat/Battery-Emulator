//··································································································
// This file is not used any more by ACAN2515 driver.
// It is provided for compatibility with sketchs that use it.
//··································································································

#ifndef ACAN_BUFFER_CLASS_DEFINED
#define ACAN_BUFFER_CLASS_DEFINED

//··································································································

#include <CANMessage.h>

//··································································································

class ACANBuffer {

//··································································································
// Default constructor
//··································································································

  public: ACANBuffer (void)  :
  mBuffer (NULL),
  mSize (0),
  mReadIndex (0),
  mWriteIndex (0),
  mCount (0),
  mPeakCount (0) {
  }

//··································································································
// Destructor
//··································································································

  public: ~ ACANBuffer (void) {
    delete [] mBuffer ;
  }

//··································································································
// Private properties
//··································································································

  private: CANMessage * mBuffer ;
  private: uint32_t mSize ;
  private: uint32_t mReadIndex ;
  private: uint32_t mWriteIndex ;
  private: uint32_t mCount ;
  private: uint32_t mPeakCount ; // > mSize if overflow did occur

//··································································································
// Accessors
//··································································································

  public: inline uint32_t size (void) const { return mSize ; }
  public: inline uint32_t count (void) const { return mCount ; }
  public: inline uint32_t peakCount (void) const { return mPeakCount ; }

//··································································································
// initWithSize
//··································································································

  public: void initWithSize (const uint32_t inSize) {
    mBuffer = new CANMessage [inSize] ;
    mSize = inSize ;
    mReadIndex = 0 ;
    mWriteIndex = 0 ;
    mCount = 0 ;
    mPeakCount = 0 ;
  }

//··································································································
// append
//··································································································

  public: bool append (const CANMessage & inMessage) {
    const bool ok = mCount < mSize ;
    if (ok) {
      mBuffer [mWriteIndex] = inMessage ;
      mWriteIndex += 1 ;
      if (mWriteIndex == mSize) {
        mWriteIndex = 0 ;
      }
      mCount ++ ;
      if (mPeakCount < mCount) {
        mPeakCount = mCount ;
      }
    }
    return ok ;
  }

//··································································································
// Remove
//··································································································

  public: bool remove (CANMessage & outMessage) {
    const bool ok = mCount > 0 ;
    if (ok) {
      outMessage = mBuffer [mReadIndex] ;
      mCount -= 1 ;
      mReadIndex += 1 ;
      if (mReadIndex == mSize) {
        mReadIndex = 0 ;
      }
    }
    return ok ;
  }

//··································································································
// No copy
//··································································································

  private: ACANBuffer (const ACANBuffer &) ;
  private: ACANBuffer & operator = (const ACANBuffer &) ;
} ;

//··································································································

#endif
