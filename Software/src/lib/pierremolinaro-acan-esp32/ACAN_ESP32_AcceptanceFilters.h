#pragma once

//----------------------------------------------------------------------------------------

#include <Arduino.h>

//----------------------------------------------------------------------------------------

class ACAN_ESP32_Filter {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum : uint8_t { standard, extended, standardAndExtended } Format ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: typedef enum : uint8_t { data, remote, dataAndRemote } Type ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // public properties
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint8_t mACR0 ;
  public: uint8_t mACR1 ;
  public: uint8_t mACR2 ;
  public: uint8_t mACR3 ;
  public: uint8_t mAMR0 ;
  public: uint8_t mAMR1 ;
  public: uint8_t mAMR2 ;
  public: uint8_t mAMR3 ;
  public: bool mAMFSingle ;
  public: Format mFormat ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Default private constructor
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: ACAN_ESP32_Filter (void) :
  mACR0 (0),
  mACR1 (0),
  mACR2 (0),
  mACR3 (0),
  mAMR0 (0xFF),
  mAMR1 (0xFF),
  mAMR2 (0xFF),
  mAMR3 (0xFF),
  mAMFSingle (false),
  mFormat (standardAndExtended) {
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accept all filter
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter acceptAll (void) {
    return ACAN_ESP32_Filter () ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accept only standard frames
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter acceptStandardFrames (void) {
    ACAN_ESP32_Filter result ;
    result.mFormat = standard ;
    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accept only extended frames
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter acceptExtendedFrames (void) {
    ACAN_ESP32_Filter result ;
    result.mFormat = extended ;
    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // singleStandardFilter: see SJA100 datasheet, figure 9 page 45 (and figure 10 page 46)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter singleStandardFilter (const ACAN_ESP32_Filter::Type inType,
                                                                const uint16_t inIdentifier,
                                                                const uint16_t inDontCareMask) {
    ACAN_ESP32_Filter result ;
    result.mAMFSingle = true ; // Single Filter
    result.mFormat = standard ;
    result.mACR0 = uint8_t (inIdentifier >> 3) ;
    result.mACR1 = uint8_t (inIdentifier << 5) ;
    result.mAMR0 = uint8_t (inDontCareMask >> 3) ;
    result.mAMR1 = uint8_t (inDontCareMask << 5) | 0x0F ;
    switch (inType) {
    case data :
      break ;
    case remote :
      result.mACR1 |= 0x10 ;
      break ;
    case dataAndRemote :
      result.mAMR1 |= 0x10 ;
      break ;
    }
    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // singleExtendedFilter: see SJA100 datasheet, figure 10 page 46 (and figure 9 page 45)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter singleExtendedFilter (const ACAN_ESP32_Filter::Type inType,
                                                                const uint32_t inIdentifier,
                                                                const uint32_t inDontCareMask) {
    ACAN_ESP32_Filter result ;
    result.mAMFSingle = true ; // Single Filter
    result.mFormat = extended ;
    result.mACR0 = uint8_t (inIdentifier >> 21) ;
    result.mACR1 = uint8_t (inIdentifier >> 13) ;
    result.mACR2 = uint8_t (inIdentifier >> 5) ;
    result.mACR3 = uint8_t (inIdentifier << 3) ;

    result.mAMR0 = uint8_t (inDontCareMask >> 21) ;
    result.mAMR1 = uint8_t (inDontCareMask >> 13) ;
    result.mAMR2 = uint8_t (inDontCareMask >> 5) ;
    result.mAMR3 = uint8_t (inDontCareMask << 3) ;

    switch (inType) {
    case data :
      break ;
    case remote :
      result.mACR3 |= 0x04 ;
      break ;
    case dataAndRemote :
      result.mAMR3 |= 0x04 ;
      break ;
    }
    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // dualStandardFilter: see SJA100 datasheet, figure 11 page 47 (and figure 12 page 48)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter dualStandardFilter (const ACAN_ESP32_Filter::Type inType0,
                                                              const uint16_t inIdentifier0,
                                                              const uint16_t inDontCareMask0,
                                                              const ACAN_ESP32_Filter::Type inType1,
                                                              const uint16_t inIdentifier1,
                                                              const uint16_t inDontCareMask1) {
    ACAN_ESP32_Filter result ;
    result.mAMFSingle = false ; // Dual Filter
    result.mFormat = standard ;

    result.mACR0 = uint8_t (inIdentifier0 >> 3) ;
    result.mACR1 = uint8_t (inIdentifier0 << 5) ;
    result.mAMR0 = uint8_t (inDontCareMask0 >> 3) ;
    result.mAMR1 = uint8_t (inDontCareMask0 << 5) | 0x0F ;
    switch (inType0) {
    case data :
      break ;
    case remote :
      result.mACR1 |= 0x10 ;
      break ;
    case dataAndRemote :
      result.mAMR1 |= 0x10 ;
      break ;
    }

    result.mACR2 = uint8_t (inIdentifier1 >> 3) ;
    result.mACR3 = uint8_t (inIdentifier1 << 5) ;
    result.mAMR2 = uint8_t (inDontCareMask1 >> 3) ;
    result.mAMR3 = uint8_t (inDontCareMask1 << 5) | 0x0F ;
    switch (inType1) {
    case data :
      break ;
    case remote :
      result.mACR3 |= 0x10 ;
      break ;
    case dataAndRemote :
      result.mAMR3 |= 0x10 ;
      break ;
    }
    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // dualExtendedFilter: see SJA100 datasheet, figure 12 page 48 (and figure 11 page 47)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static inline ACAN_ESP32_Filter dualExtendedFilter (const uint32_t inIdentifier0,
                                                              const uint32_t inDontCareMask0,
                                                              const uint32_t inIdentifier1,
                                                              const uint32_t inDontCareMask1) {
    ACAN_ESP32_Filter result ;
    result.mAMFSingle = false ; // Dual Filter
    result.mFormat = extended ;

    result.mACR0 = uint8_t (inIdentifier0 >> 21) ;
    result.mACR1 = uint8_t (inIdentifier0 >> 13) ;
    result.mAMR0 = uint8_t (inDontCareMask0 >> 21) ;
    result.mAMR1 = uint8_t (inDontCareMask0 >> 13) ;

    result.mACR2 = uint8_t (inIdentifier1 >> 21) ;
    result.mACR3 = uint8_t (inIdentifier1 << 13) ;
    result.mAMR2 = uint8_t (inDontCareMask1 >> 21) ;
    result.mAMR3 = uint8_t (inDontCareMask1 << 13) ;

    return result ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//----------------------------------------------------------------------------------------
