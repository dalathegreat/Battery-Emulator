//··································································································
// A CAN driver for MCP2515
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2515
//··································································································

#pragma once

//··································································································

#include "ACAN2515_Buffer16.h"
#include "ACAN2515Settings.h"
#include "MCP2515ReceiveFilters.h"
#include <SPI.h>

//··································································································

class ACAN2515 {

//··································································································
//    Constructor: using hardware SPI
//··································································································

  public: ACAN2515 (const uint8_t inCS,  // CS input of MCP2515
                    SPIClass & inSPI, // Hardware SPI object
                    const uint8_t inINT) ; // INT output of MCP2515


//··································································································
//    Initialisation: returns 0 if ok, otherwise see error codes below
//··································································································

  public: uint16_t begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void)) ;

  public: uint16_t begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void),
                          const ACAN2515Mask inRXM0,
                          const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                          const uint8_t inAcceptanceFilterCount) ;

  public: uint16_t begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void),
                          const ACAN2515Mask inRXM0,
                          const ACAN2515Mask inRXM1,
                          const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                          const uint8_t inAcceptanceFilterCount) ;

//··································································································
//    Error codes returned by begin
//··································································································

  public: static const uint16_t kNoMCP2515                   = 1 <<  0 ;
  public: static const uint16_t kTooFarFromDesiredBitRate    = 1 <<  1 ;
  public: static const uint16_t kInconsistentBitRateSettings = 1 <<  2 ;
  public: static const uint16_t kINTPinIsNotAnInterrupt      = 1 <<  3 ;
  public: static const uint16_t kISRIsNull                   = 1 <<  4 ;
  public: static const uint16_t kRequestedModeTimeOut        = 1 <<  5 ;
  public: static const uint16_t kAcceptanceFilterArrayIsNULL = 1 << 6 ;
  public: static const uint16_t kOneFilterMaskRequiresOneOrTwoAcceptanceFilters = 1 << 7 ;
  public: static const uint16_t kTwoFilterMasksRequireThreeToSixAcceptanceFilters = 1 << 8 ;
  public: static const uint16_t kCannotAllocateReceiveBuffer = 1 << 9 ;
  public: static const uint16_t kCannotAllocateTransmitBuffer0 = 1 << 10 ;
  public: static const uint16_t kCannotAllocateTransmitBuffer1 = 1 << 11 ;
  public: static const uint16_t kCannotAllocateTransmitBuffer2 = 1 << 12 ;
  public: static const uint32_t kISRNotNullAndNoIntPin         = 1 << 13 ;


//··································································································
//    Change Mode on the fly
//··································································································

  public: uint16_t changeModeOnTheFly (const ACAN2515Settings::RequestedMode inRequestedMode) ;


//··································································································
//    Set filters on the fly
//··································································································

  public: uint16_t setFiltersOnTheFly (void) ;

  public: uint16_t setFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                       const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                       const uint8_t inAcceptanceFilterCount) ;

  public: uint16_t setFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                       const ACAN2515Mask inRXM1,
                                       const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                       const uint8_t inAcceptanceFilterCount) ;


//··································································································
//    end
//··································································································

  public: void end (void) ;


//··································································································
//    Receiving messages
//··································································································

  public: bool available (void) ;

  public: bool receive (CANMessage & outFrame) ;

  public: typedef void (*tFilterMatchCallBack) (const uint8_t inFilterIndex) ;

  public: bool dispatchReceivedMessage (const tFilterMatchCallBack inFilterMatchCallBack = NULL) ;


//··································································································
//    Handling messages to send and receiving messages
//··································································································

  public: void isr (void) ;
  public: bool isr_core (void) ;
  private: void handleTXBInterrupt (const uint8_t inTXB) ;
  private: void handleRXBInterrupt (void) ;


//··································································································
//    Properties
//··································································································

  private: SPIClass & mSPI ;
  private: const SPISettings mSPISettings ;
  private: const uint8_t mCS ;
  private: const uint8_t mINT ;
  private: bool mRolloverEnable ;
  #ifdef ARDUINO_ARCH_ESP32
    public: SemaphoreHandle_t mISRSemaphore ;
    private: void (* mInterruptServiceRoutine) (void) = nullptr ;
  #endif


//··································································································
//    Receive buffer
//··································································································

  private: ACAN2515_Buffer16 mReceiveBuffer ;


//··································································································
//    Receive buffer size
//··································································································

  public: inline uint16_t receiveBufferSize (void) const {
    return mReceiveBuffer.size () ;
  }


//··································································································
//    Receive buffer count
//··································································································

  public: inline uint16_t receiveBufferCount (void) const {
    return mReceiveBuffer.count () ;
  }


//··································································································
//    Receive buffer peak count
//··································································································

  public: inline uint16_t receiveBufferPeakCount (void) const {
    return mReceiveBuffer.peakCount () ;
  }


//··································································································
//    Call back function array
//··································································································

  private: ACANCallBackRoutine mCallBackFunctionArray [6] ;


//··································································································
//    Transmitting messages
//··································································································

  public: bool sendBufferNotFullForIndex (const uint32_t inIndex) ; // 0 ... 2

  public: bool tryToSend (const CANMessage & inMessage) ;


//··································································································
//    Driver transmit buffer
//··································································································

  private: ACAN2515_Buffer16 mTransmitBuffer [3] ;
  private: bool mTXBIsFree [3] ;

  public: inline uint16_t transmitBufferSize (const uint8_t inIndex) const {
    return mTransmitBuffer [inIndex].size () ;
  }

  public: inline uint16_t transmitBufferCount (const uint8_t inIndex) const {
    return mTransmitBuffer [inIndex].count () ;
  }

  public: inline uint16_t transmitBufferPeakCount (const uint8_t inIndex) const {
    return mTransmitBuffer [inIndex].peakCount () ;
  }
  private: void internalSendMessage (const CANMessage & inFrame, const uint8_t inTXB) ;


//··································································································
//    Polling
//··································································································

  public: void poll (void) ;


//··································································································
//    Private methods
//··································································································

  private: inline void select (void) { digitalWrite (mCS, LOW) ; }

  private: inline void unselect (void) { digitalWrite (mCS, HIGH) ; }

  private: uint16_t beginWithoutFilterCheck (const ACAN2515Settings & inSettings,
                                             void (* inInterruptServiceRoutine) (void),
                                             const ACAN2515Mask inRXM0,
                                             const ACAN2515Mask inRXM1,
                                             const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                             const uint8_t inAcceptanceFilterCount) ;

  private: uint16_t internalBeginOperation (const ACAN2515Settings & inSettings,
                                            const ACAN2515Mask inRXM0,
                                            const ACAN2515Mask inRXM1,
                                            const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                            const uint8_t inAcceptanceFilterCount) ;

  private: void write2515Register (const uint8_t inRegister, const uint8_t inValue) ;

  private: uint8_t read2515Register (const uint8_t inRegister) ;

  private: uint8_t read2515Status (void) ;

  private: uint8_t read2515RxStatus (void) ;

  private: void bitModify2515Register (const uint8_t inRegister, const uint8_t inMask, const uint8_t inData) ;

  private: void setupMaskRegister (const ACAN2515Mask inMask, const uint8_t inRegister) ;

  private: uint16_t setRequestedMode (const uint8_t inCANControlRegister) ;


  private: uint16_t internalSetFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                                const ACAN2515Mask inRXM1,
                                                const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                                const uint8_t inAcceptanceFilterCount) ;

  #ifdef ARDUINO_ARCH_ESP32
    public: void attachMCP2515InterruptPin (void) ;
  #endif

//··································································································
//    MCP2515 controller state
//··································································································

  public: uint8_t receiveErrorCounter (void) ;
  public: uint8_t transmitErrorCounter (void) ;
  public: uint8_t errorFlagRegister (void) ;


//··································································································
//    No Copy
//··································································································

  private: ACAN2515 (const ACAN2515 &) = delete ;
  private: ACAN2515 & operator = (const ACAN2515 &) = delete ;

//··································································································

} ;

//··································································································
