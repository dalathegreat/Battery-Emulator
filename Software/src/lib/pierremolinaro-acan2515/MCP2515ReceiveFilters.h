//··································································································
// MCP2515 Receive filter classes
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2515
//··································································································

#ifndef MCP2515_RECEIVE_FILTER_ENTITIES_DEFINED
#define MCP2515_RECEIVE_FILTER_ENTITIES_DEFINED

//··································································································

#include <Arduino.h>

//··································································································

class ACAN2515Mask {

//--- Default constructor
  public: ACAN2515Mask (void) :
  mSIDH (0),
  mSIDL (0),
  mEID8 (0),
  mEID0 (0) {
  }

//--- Properties
  public: uint8_t mSIDH ;
  public: uint8_t mSIDL ;
  public: uint8_t mEID8 ;
  public: uint8_t mEID0 ;
} ;

//··································································································

class ACAN2515AcceptanceFilter {
  public: typedef void (*tCallBackRoutine) (const CANMessage & inMessage) ;
  public: const ACAN2515Mask mMask ;
  public: const tCallBackRoutine mCallBack ;
} ;

//··································································································

inline ACAN2515Mask standard2515Mask (const uint16_t inIdentifier,
                                      const uint8_t inByte0,
                                      const uint8_t inByte1) {
  ACAN2515Mask result ;
  result.mSIDH = (uint8_t) (inIdentifier >> 3) ;
  result.mSIDL = (uint8_t) (inIdentifier << 5) ;
  result.mEID8 = inByte0 ;
  result.mEID0 = inByte1 ;
  return result ;
}

//··································································································

inline ACAN2515Mask extended2515Mask (const uint32_t inIdentifier) {
  ACAN2515Mask result ;
  result.mSIDH = (uint8_t) (inIdentifier >> 21) ;
  result.mSIDL = (uint8_t) (((inIdentifier >> 16) & 0x03) | ((inIdentifier >> 13) & 0xE0)) ;
  result.mEID8 = (uint8_t) (inIdentifier >> 8) ;
  result.mEID0 = (uint8_t) inIdentifier ;
  return result ;
}

//··································································································

inline ACAN2515Mask standard2515Filter (const uint16_t inIdentifier,
                                 const uint8_t inByte0,
                                 const uint8_t inByte1) {
  ACAN2515Mask result ;
  result.mSIDH = (uint8_t) (inIdentifier >> 3) ;
  result.mSIDL = (uint8_t) (inIdentifier << 5) ;
  result.mEID8 = inByte0 ;
  result.mEID0 = inByte1 ;
  return result ;
}

//··································································································

inline ACAN2515Mask extended2515Filter (const uint32_t inIdentifier) {
  ACAN2515Mask result ;
  result.mSIDH = (uint8_t) (inIdentifier >> 21) ;
  result.mSIDL = (uint8_t) (((inIdentifier >> 16) & 0x03) | ((inIdentifier >> 13) & 0xE0)) | 0x08 ;
  result.mEID8 = (uint8_t) (inIdentifier >> 8) ;
  result.mEID0 = (uint8_t) inIdentifier ;
  return result ;
}

//··································································································

#endif
