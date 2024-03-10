//-----------------------------------------------------------------------------
// Generic CANFD Message
// by Pierre Molinaro
//
// https://github.com/pierremolinaro/acan2517FD
//
//-----------------------------------------------------------------------------

#ifndef GENERIC_CANFD_MESSAGE_DEFINED
#define GENERIC_CANFD_MESSAGE_DEFINED

//-----------------------------------------------------------------------------

#include <CANMessage.h>

//-----------------------------------------------------------------------------
//    CANFDMessage class
//-----------------------------------------------------------------------------
// Note that "len" field contains the actual length, not its encoding in CANFD frames
// Valid values are: 0, 1, ..., 8, 12, 16, 20, 24, 32, 48, 64.
// Having other values is an error that prevents frame to be sent by tryToSend
// You can use the "pad" method for padding with 0 bytes to the next valid length

class CANFDMessage {

//·············································································
//   Constructors
//·············································································

  public : CANFDMessage (void) :
  id (0),  // Frame identifier
  ext (false), // false -> base frame, true -> extended frame
  type (CANFD_WITH_BIT_RATE_SWITCH),
  idx (0),  // This field is used by the driver
  len (0), // Length of data (0 ... 64)
  data () {
  }

//·············································································

  public : CANFDMessage (const CANMessage & inMessage) :
  id (inMessage.id),  // Frame identifier
  ext (inMessage.ext), // false -> base frame, true -> extended frame
  type (inMessage.rtr ? CAN_REMOTE : CAN_DATA),
  idx (inMessage.idx),  // This field is used by the driver
  len (inMessage.len), // Length of data (0 ... 64)
  data () {
    data64 [0] = inMessage.data64 ;
  }

//·············································································
//   Enumerated Type
//·············································································

  public: typedef enum : uint8_t {
    CAN_REMOTE,
    CAN_DATA,
    CANFD_NO_BIT_RATE_SWITCH,
    CANFD_WITH_BIT_RATE_SWITCH
  } Type ;

//·············································································
//   Properties
//·············································································

  public : uint32_t id ;  // Frame identifier
  public : bool ext ; // false -> base frame, true -> extended frame
  public : Type type ;
  public : uint8_t idx ;  // This field is used by the driver
  public : uint8_t len ;  // Length of data (0 ... 64)
  public : union {
    uint64_t data64    [ 8] ; // Caution: subject to endianness
    int64_t  data_s64  [ 8] ; // Caution: subject to endianness
    uint32_t data32    [16] ; // Caution: subject to endianness
    int32_t  data_s32  [16] ; // Caution: subject to endianness
    float    dataFloat [16] ; // Caution: subject to endianness
    uint16_t data16    [32] ; // Caution: subject to endianness
    int16_t  data_s16  [32] ; // Caution: subject to endianness
    int8_t   data_s8   [64] ;
    uint8_t  data      [64] ;
  } ;

//·············································································
//   Methods
//·············································································

  public: void pad (void) {
    uint8_t paddedLength = len ;
    if ((len > 8) && (len < 12)) {
      paddedLength = 12 ;
    }else if ((len > 12) && (len < 16)) {
      paddedLength = 16 ;
    }else if ((len > 16) && (len < 20)) {
      paddedLength = 20 ;
    }else if ((len > 20) && (len < 24)) {
      paddedLength = 24 ;
    }else if ((len > 24) && (len < 32)) {
      paddedLength = 32 ;
    }else if ((len > 32) && (len < 48)) {
      paddedLength = 48 ;
    }else if ((len > 48) && (len < 64)) {
      paddedLength = 64 ;
    }
    while (len < paddedLength) {
      data [len] = 0 ;
      len += 1 ;
    }
  }

//·············································································

  public: bool isValid (void) const {
    if ((type == CAN_REMOTE) || (type == CAN_DATA)) { // Remote frame
      return len <= 8 ;
    }else{ // Data frame
      return
        (len <=  8) || (len == 12) || (len == 16) || (len == 20)
      ||
        (len == 24) || (len == 32) || (len == 48) || (len == 64)
      ;
    }
  }

//·············································································

} ;

//-----------------------------------------------------------------------------

typedef void (*ACANFDCallBackRoutine) (const CANFDMessage & inMessage) ;

//-----------------------------------------------------------------------------

#endif
