//----------------------------------------------------------------------------------------------------------------------
// A CAN driver for MCP2517FD CAN Controller in CANFD mode
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2517FD
//
//----------------------------------------------------------------------------------------------------------------------

#ifndef ACANFD_BUFFER_CLASS_DEFINED
#define ACANFD_BUFFER_CLASS_DEFINED

//----------------------------------------------------------------------------------------------------------------------

#include <CANFDMessage.h>

//----------------------------------------------------------------------------------------------------------------------

class ACANFDBuffer {

//······················································································································
// Default constructor
//······················································································································

  public: ACANFDBuffer (void)  :
  mBuffer (NULL),
  mSize (0),
  mReadIndex (0),
  mCount (0),
  mPeakCount (0) {
  }

//······················································································································
// Destructor
//······················································································································

  public: ~ ACANFDBuffer (void) {
    delete [] mBuffer ;
  }

//······················································································································
// Private properties
//······················································································································

  private: CANFDMessage * mBuffer ;
  private: uint32_t mSize ;
  private: uint32_t mReadIndex ;
  private: uint32_t mCount ;
  private: uint32_t mPeakCount ; // > mSize if overflow did occur

//······················································································································
// Accessors
//······················································································································

  public: inline uint32_t size (void) const { return mSize ; }
  public: inline uint32_t count (void) const { return mCount ; }
  public: inline bool isFull (void) const { return mCount == mSize ; } // Added in release 2.17 (thanks to Flole998)
  public: inline uint32_t peakCount (void) const { return mPeakCount ; }

//······················································································································
// initWithSize
//······················································································································

  public: void initWithSize (const uint32_t inSize) {
    delete [] mBuffer ; mBuffer = new CANFDMessage [inSize] ;
    mSize = inSize ;
    mReadIndex = 0 ;
    mCount = 0 ;
    mPeakCount = 0 ;
  }

//······················································································································
// append
//······················································································································

  public: bool append (const CANFDMessage & inMessage) {
    const bool ok = mCount < mSize ;
    if (ok) {
      uint32_t writeIndex = mReadIndex + mCount ;
      if (writeIndex >= mSize) {
        writeIndex -= mSize ;
      }
      mBuffer [writeIndex] = inMessage ;
      mCount += 1 ;
      if (mPeakCount < mCount) {
        mPeakCount = mCount ;
      }
    }else{
      mPeakCount = mSize + 1 ;
    }
    return ok ;
  }

//······················································································································
// Remove
//······················································································································

  public: bool remove (CANFDMessage & outMessage) {
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

//······················································································································
// No copy
//······················································································································

  private: ACANFDBuffer (const ACANFDBuffer &) = delete ;
  private: ACANFDBuffer & operator = (const ACANFDBuffer &) = delete ;
} ;

//----------------------------------------------------------------------------------------------------------------------

#endif
