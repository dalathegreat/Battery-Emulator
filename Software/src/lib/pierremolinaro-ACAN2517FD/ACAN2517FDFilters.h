//----------------------------------------------------------------------------------------------------------------------
// An utility class for:
//   - ACAN2517FD CAN driver for MCP2517FD (CANFD mode)
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2517FD
//
//----------------------------------------------------------------------------------------------------------------------

#ifndef ACAN2517FD_FILTERS_CLASS_DEFINED
#define ACAN2517FD_FILTERS_CLASS_DEFINED

//----------------------------------------------------------------------------------------------------------------------

#include <CANFDMessage.h>

//----------------------------------------------------------------------------------------------------------------------
//  ACAN2517FDFilters class
//----------------------------------------------------------------------------------------------------------------------

class ACAN2517FDFilters {

//······················································································································
//   EMBEDDED CLASS
//······················································································································

  private: class Filter {
    public: Filter * mNextFilter ;
    public: const uint32_t mFilterMask ;
    public: const uint32_t mAcceptanceFilter ;
    public: const ACANFDCallBackRoutine mCallBackRoutine ;

    public: Filter (const uint32_t inFilterMask,
                    const uint32_t inAcceptanceFilter,
                    const ACANFDCallBackRoutine inCallBackRoutine) :
    mNextFilter (NULL),
    mFilterMask (inFilterMask),
    mAcceptanceFilter (inAcceptanceFilter),
    mCallBackRoutine (inCallBackRoutine) {
    }

  //--- No copy
    private: Filter (const Filter &) ;
    private: Filter & operator = (const Filter &) ;
  } ;

//······················································································································
//   ENUMERATED TYPE
//······················································································································

  public: typedef enum {
      kFiltersOk,
      kStandardIdentifierTooLarge,
      kExtendedIdentifierTooLarge,
      kStandardAcceptanceTooLarge,
      kExtendedAcceptanceTooLarge,
      kStandardMaskTooLarge,
      kExtendedMaskTooLarge,
      kInconsistencyBetweenMaskAndAcceptance
  } FilterStatus ;

//······················································································································
//   CONSTRUCTOR
//······················································································································

  public: ACAN2517FDFilters (void) {}

//······················································································································
//   DESTRUCTOR
//······················································································································

  public: ~ ACAN2517FDFilters (void) {
    while (mFirstFilter != NULL) {
      Filter * next = mFirstFilter->mNextFilter ;
      delete mFirstFilter ;
      mFirstFilter = next ;
    }
  }

//······················································································································
//   RECEIVE FILTERS
//······················································································································

  public: void appendPassAllFilter (const ACANFDCallBackRoutine inCallBackRoutine) {  // Accept any frame
    Filter * f = new Filter (0, 0, inCallBackRoutine) ;
    if (mFirstFilter == NULL) {
      mFirstFilter = f ;
    }else{
      mLastFilter->mNextFilter  = f ;
    }
    mLastFilter = f ;
    mFilterCount += 1 ;
  }

//······················································································································

  public: void appendFormatFilter (const tFrameFormat inFormat, // Accept any identifier
                                   const ACANFDCallBackRoutine inCallBackRoutine) {
    Filter * f = new Filter (((uint32_t) 1) << 30,
                             (inFormat == kExtended) ? (((uint32_t) 1) << 30) : 0,
                             inCallBackRoutine) ;
    if (mFirstFilter == NULL) {
      mFirstFilter = f ;
    }else{
      mLastFilter->mNextFilter  = f ;
    }
    mLastFilter = f ;
    mFilterCount += 1 ;
  }

//······················································································································

  public: void appendFrameFilter (const tFrameFormat inFormat,
                                  const uint32_t inIdentifier,
                                  const ACANFDCallBackRoutine inCallBackRoutine) {
  //--- Check identifier
    if (inFormat == kExtended) {
      if (inIdentifier > 0x1FFFFFFF) {
        mFilterStatus = kExtendedIdentifierTooLarge ;
        mFilterErrorIndex = mFilterCount ;
      }
    }else if (inIdentifier > 0x7FF) {
      mFilterStatus = kStandardIdentifierTooLarge ;
      mFilterErrorIndex = mFilterCount ;
    }
  //--- Re order bits if extended filter
    const uint32_t mask = (((uint32_t) 1) << 30) | ((inFormat == kExtended) ? 0x1FFFFFFF : 0x7FF) ;
    uint32_t acceptance ;
    if (inFormat == kExtended) {
      acceptance = ((inIdentifier >> 18) & 0x7FF) | ((inIdentifier & 0x3FFFF) << 11) | (((uint32_t) 1) << 30) ;
    }else{
      acceptance = inIdentifier ;
    }
  //--- Enter filter
    Filter * f = new Filter (mask, acceptance, inCallBackRoutine) ;
    if (mFirstFilter == NULL) {
      mFirstFilter = f ;
    }else{
      mLastFilter->mNextFilter  = f ;
    }
    mLastFilter = f ;
    mFilterCount += 1 ;
  }

//······················································································································

  public: void appendFilter (const tFrameFormat inFormat,
                             const uint32_t inMask,
                             const uint32_t inAcceptance,
                             const ACANFDCallBackRoutine inCallBackRoutine) {
  //--- Check consistency between mask and acceptance
    if ((inMask & inAcceptance) != inAcceptance) {
      mFilterStatus = kInconsistencyBetweenMaskAndAcceptance ;
      mFilterErrorIndex = mFilterCount ;
    }
  //--- Check identifier
    if (inFormat == kExtended) {
      if (inAcceptance > 0x1FFFFFFF) {
        mFilterStatus = kExtendedAcceptanceTooLarge ;
        mFilterErrorIndex = mFilterCount ;
      }
    }else if (inAcceptance > 0x7FF) {
      mFilterStatus = kStandardAcceptanceTooLarge ;
      mFilterErrorIndex = mFilterCount ;
    }
  //--- Check mask
    if (inFormat == kExtended) {
      if (inMask > 0x1FFFFFFF) {
        mFilterStatus = kExtendedMaskTooLarge ;
        mFilterErrorIndex = mFilterCount ;
      }
    }else if (inMask > 0x7FF) {
      mFilterStatus = kStandardMaskTooLarge ;
      mFilterErrorIndex = mFilterCount ;
    }
  //--- Re order bits if extended filter
    uint32_t mask = ((uint32_t) 1) << 30 ;
    if (inFormat == kExtended) {
      mask |= ((inMask >> 18) & 0x7FF) | ((inMask & 0x3FFFF) << 11) ;
    }else{
      mask |= inMask ;
    }
    uint32_t acceptance ;
    if (inFormat == kExtended) {
      acceptance = ((inAcceptance >> 18) & 0x7FF) | ((inAcceptance & 0x3FFFF) << 11) | (((uint32_t) 1) << 30) ;
    }else{
      acceptance = inAcceptance ;
    }
  //--- Enter filter
    Filter * f = new Filter (mask, acceptance, inCallBackRoutine) ;
    if (mFirstFilter == NULL) {
      mFirstFilter = f ;
    }else{
      mLastFilter->mNextFilter  = f ;
    }
    mLastFilter = f ;
    mFilterCount += 1 ;
  }

//······················································································································
//   ACCESSORS
//······················································································································

  public: FilterStatus filterStatus (void) const { return mFilterStatus ; }

  public: uint8_t filterErrorIndex (void) const { return mFilterErrorIndex ; }

  public: uint8_t filterCount (void) const { return mFilterCount ; }

//······················································································································
//   PRIVATE PROPERTIES
//······················································································································

  private: uint8_t mFilterCount = 0 ;
  private: Filter * mFirstFilter = NULL ;
  private: Filter * mLastFilter  = NULL ;
  private: FilterStatus mFilterStatus = kFiltersOk ;
  private: uint8_t mFilterErrorIndex = 0 ;

//······················································································································
//   NO COPY
//······················································································································

  private: ACAN2517FDFilters (const ACAN2517FDFilters &) = delete ;
  private: ACAN2517FDFilters & operator = (const ACAN2517FDFilters &) = delete ;

//······················································································································
//   Friend
//······················································································································

  friend class ACAN2517FD ;

//······················································································································

} ;

//----------------------------------------------------------------------------------------------------------------------

#endif
