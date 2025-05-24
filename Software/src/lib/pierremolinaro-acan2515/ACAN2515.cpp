//··································································································
// A CAN driver for MCP2515
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2515
//··································································································

#include "ACAN2515.h"
#include "../../system_settings.h" //Contains task priority

//··································································································
//   MCP2515 COMMANDS
//··································································································

static const uint8_t RESET_COMMAND = 0xC0 ;
static const uint8_t WRITE_COMMAND = 0x02 ;
static const uint8_t READ_COMMAND  = 0x03 ;
static const uint8_t BIT_MODIFY_COMMAND         = 0x05 ;
static const uint8_t LOAD_TX_BUFFER_COMMAND     = 0x40 ;
static const uint8_t REQUEST_TO_SEND_COMMAND    = 0x80 ;
static const uint8_t READ_FROM_RXB0SIDH_COMMAND = 0x90 ;
static const uint8_t READ_FROM_RXB1SIDH_COMMAND = 0x94 ;
static const uint8_t READ_STATUS_COMMAND        = 0xA0 ;
static const uint8_t RX_STATUS_COMMAND          = 0xB0 ;

//··································································································
//   MCP2515 REGISTERS
//··································································································

static const uint8_t BFPCTRL_REGISTER   = 0x0C ;
static const uint8_t TXRTSCTRL_REGISTER = 0x0D ;
static const uint8_t CANSTAT_REGISTER   = 0x0E ;
static const uint8_t CANCTRL_REGISTER   = 0x0F ;
static const uint8_t TEC_REGISTER       = 0x1C ;
static const uint8_t REC_REGISTER       = 0x1D ;
static const uint8_t RXM0SIDH_REGISTER  = 0x20 ;
static const uint8_t RXM1SIDH_REGISTER  = 0x24 ;
static const uint8_t CNF3_REGISTER      = 0x28 ;
static const uint8_t CNF2_REGISTER      = 0x29 ;
static const uint8_t CNF1_REGISTER      = 0x2A ;
static const uint8_t CANINTF_REGISTER   = 0x2C ;
static const uint8_t EFLG_REGISTER      = 0x2D ;
static const uint8_t TXB0CTRL_REGISTER  = 0x30 ;
static const uint8_t TXB1CTRL_REGISTER  = 0x40 ;
static const uint8_t TXB2CTRL_REGISTER  = 0x50 ;
static const uint8_t RXB0CTRL_REGISTER  = 0x60 ;
static const uint8_t RXB1CTRL_REGISTER  = 0x70 ;

static const uint8_t RXFSIDH_REGISTER [6] = {0x00, 0x04, 0x08, 0x10, 0x14, 0x18} ;

//··································································································
// Note about ESP32
//··································································································
//
// It appears that Arduino ESP32 interrupts are managed in a completely different way
// from "usual" Arduino:
//   - SPI.usingInterrupt is not implemented;
//   - noInterrupts() and interrupts() are NOPs;
//   - interrupt service routines should be fast, otherwise you get smothing like
//     "Guru Meditation Error: Core 1 panic'ed (Interrupt wdt timeout on CPU1)".

// So we handle the ESP32 interrupt in the following way:
//   - interrupt service routine performs a xSemaphoreGive on mISRSemaphore of can driver
//   - this activates the myESP32Task task that performs "isr_core" that is done
//     by interrupt service routine in "usual" Arduino;
//   - as this task runs in parallel with setup / loop routines, SPI access is natively
//     protected by the beginTransaction / endTransaction pair, that manages a mutex.

//··································································································

#ifdef ARDUINO_ARCH_ESP32
  static void myESP32Task (void * pData) {
    ACAN2515 * canDriver = (ACAN2515 *) pData ;
    while (1) {
      canDriver->attachMCP2515InterruptPin () ;
      xSemaphoreTake (canDriver->mISRSemaphore, portMAX_DELAY) ;
      bool loop = true ;
      while (loop) {
        loop = canDriver->isr_core () ;
      }
    }
  }
#endif

//··································································································

#ifdef ARDUINO_ARCH_ESP32
  void ACAN2515::attachMCP2515InterruptPin (void) {
    attachInterrupt (digitalPinToInterrupt (mINT), mInterruptServiceRoutine, ONLOW) ;
  }
#endif

//··································································································
//   CONSTRUCTOR, HARDWARE SPI
//··································································································

ACAN2515::ACAN2515 (const uint8_t inCS,  // CS input of MCP2515
                    SPIClass & inSPI, // Hardware SPI object
                    const uint8_t inINT) : // INT output of MCP2515
mSPI (inSPI),
mSPISettings (10UL * 1000UL * 1000UL, MSBFIRST, SPI_MODE0),  // 10 MHz, UL suffix is required for Arduino Uno
mCS (inCS),
mINT (inINT),
mRolloverEnable (false),
#ifdef ARDUINO_ARCH_ESP32
  mISRSemaphore (xSemaphoreCreateCounting (10, 0)),
#endif
mReceiveBuffer (),
mCallBackFunctionArray (),
mTXBIsFree () {
  for (uint8_t i=0 ; i<6 ; i++) {
    mCallBackFunctionArray [i] = NULL ;
  }
}

//··································································································
//   BEGIN
//··································································································

uint16_t ACAN2515::begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void)) {

  return beginWithoutFilterCheck (inSettings, inInterruptServiceRoutine, ACAN2515Mask (), ACAN2515Mask (), NULL, 0) ;
}

//··································································································

uint16_t ACAN2515::begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void),
                          const ACAN2515Mask inRXM0,
                          const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                          const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ;
  if (inAcceptanceFilterCount == 0) {
    errorCode = kOneFilterMaskRequiresOneOrTwoAcceptanceFilters ;
  }else if (inAcceptanceFilterCount > 2) {
    errorCode = kOneFilterMaskRequiresOneOrTwoAcceptanceFilters ;
  }else if (inAcceptanceFilters == NULL) {
    errorCode = kAcceptanceFilterArrayIsNULL ;
  }else{
    errorCode = beginWithoutFilterCheck (inSettings, inInterruptServiceRoutine,
                                         inRXM0, inRXM0, inAcceptanceFilters, inAcceptanceFilterCount) ;
  }
  return errorCode ;
}

//··································································································

uint16_t ACAN2515::begin (const ACAN2515Settings & inSettings,
                          void (* inInterruptServiceRoutine) (void),
                          const ACAN2515Mask inRXM0,
                          const ACAN2515Mask inRXM1,
                          const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                          const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ;
  if (inAcceptanceFilterCount < 3) {
    errorCode = kTwoFilterMasksRequireThreeToSixAcceptanceFilters ;
  }else if (inAcceptanceFilterCount > 6) {
    errorCode = kTwoFilterMasksRequireThreeToSixAcceptanceFilters ;
  }else if (inAcceptanceFilters == NULL) {
    errorCode = kAcceptanceFilterArrayIsNULL ;
  }else{
    errorCode = beginWithoutFilterCheck (inSettings, inInterruptServiceRoutine,
                                         inRXM0, inRXM1, inAcceptanceFilters, inAcceptanceFilterCount) ;
  }
  return errorCode ;
}

//··································································································

uint16_t ACAN2515::beginWithoutFilterCheck (const ACAN2515Settings & inSettings,
                                            void (* inInterruptServiceRoutine) (void),
                                            const ACAN2515Mask inRXM0,
                                            const ACAN2515Mask inRXM1,
                                            const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                            const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ; // Means no error
//----------------------------------- Check mINT has interrupt capability
  const int8_t itPin = digitalPinToInterrupt (mINT) ;
  if ((mINT != 255) && (itPin == NOT_AN_INTERRUPT)) {
    errorCode = kINTPinIsNotAnInterrupt ;
  }
//----------------------------------- Check interrupt service routine is not null
  if ((mINT != 255) && (inInterruptServiceRoutine == NULL)) {
    errorCode |= kISRIsNull ;
  }
//----------------------------------- Check consistency between ISR and INT pin
  if ((mINT == 255) && (inInterruptServiceRoutine != NULL)) {
    errorCode |= kISRNotNullAndNoIntPin ;
  }
//----------------------------------- if no error, configure port and MCP2515
  if (errorCode == 0) {
  //--- Configure ports
    if (mINT != 255) { // 255 means interrupt is not used
      pinMode (mINT, INPUT_PULLUP) ;
    }
    pinMode (mCS, OUTPUT) ;
    digitalWrite (mCS, HIGH) ;  // CS is high outside a command
  //--- Send software reset to MCP2515
    mSPI.beginTransaction (mSPISettings) ;
      select () ;
        mSPI.transfer (RESET_COMMAND) ;
      unselect () ;
    mSPI.endTransaction () ;
  //--- DS20001801J, page 55: The Oscillator Start-up Timer keeps the device in a Reset
  // state for 128 OSC1 clock cycles after the occurrence of a Power-on Reset, SPI Reset,
  // after the assertion of the RESET pin, and after a wake-up from Sleep mode
  // Fot a 1 MHz clock --> 128 µs
  // So delayMicroseconds (10) is too short --> use delay (2)
  //    delayMicroseconds (10) ; // Removed in release 2.1.2
    delay (2) ; // Added in release 2.1.2
  //--- Internal begin
    errorCode = internalBeginOperation (inSettings,
                                        inRXM0,
                                        inRXM1,
                                        inAcceptanceFilters,
                                        inAcceptanceFilterCount) ;
  }
//--- Configure interrupt only if no error (thanks to mvSarma)
  if (errorCode == 0) {
     if (mINT != 255) { // 255 means interrupt is not used
      #ifdef ARDUINO_ARCH_ESP32
        mInterruptServiceRoutine = inInterruptServiceRoutine ;
      #else
        mSPI.usingInterrupt (itPin) ; // usingInterrupt is not implemented in Arduino ESP32
        attachInterrupt (itPin, inInterruptServiceRoutine, LOW) ;
      #endif
    }
    #ifdef ARDUINO_ARCH_ESP32
      xTaskCreate (myESP32Task, "ACAN2515Handler", 1200, this, TASK_ACAN2515_PRIORITY, NULL) ;
    #endif
  }
//----------------------------------- Return
  return errorCode ;
}

//··································································································
//   MESSAGE RECEPTION
//··································································································

bool ACAN2515::available (void) {
  #ifdef ARDUINO_ARCH_ESP32
    mSPI.beginTransaction (mSPISettings) ; // For ensuring mutual exclusion access
  #else
    noInterrupts () ;
  #endif
    const bool hasReceivedMessage = mReceiveBuffer.count () > 0 ;
  #ifdef ARDUINO_ARCH_ESP32
    mSPI.endTransaction () ;
  #else
    interrupts () ;
  #endif
  return hasReceivedMessage ;
}

//··································································································

bool ACAN2515::receive (CANMessage & outMessage) {
  #ifdef ARDUINO_ARCH_ESP32
    mSPI.beginTransaction (mSPISettings) ; // For ensuring mutual exclusion access
  #else
    noInterrupts () ;
  #endif
    const bool hasReceivedMessage = mReceiveBuffer.remove (outMessage) ;
  #ifdef ARDUINO_ARCH_ESP32
    mSPI.endTransaction () ;
  #else
    interrupts () ;
  #endif
//---
  return hasReceivedMessage ;
}

//··································································································

bool ACAN2515::dispatchReceivedMessage (const tFilterMatchCallBack inFilterMatchCallBack) {
  CANMessage receivedMessage ;
  const bool hasReceived = receive (receivedMessage) ;
  if (hasReceived) {
    const uint8_t filterIndex = receivedMessage.idx ;
    if (NULL != inFilterMatchCallBack) {
      inFilterMatchCallBack (filterIndex) ;
    }
    ACANCallBackRoutine callBackFunction = mCallBackFunctionArray [filterIndex] ;
    if (NULL != callBackFunction) {
      callBackFunction (receivedMessage) ;
    }
  }
  return hasReceived ;
}

//··································································································
//  INTERRUPTS ARE DISABLED WHEN THESE FUNCTIONS ARE EXECUTED
//··································································································

uint16_t ACAN2515::internalBeginOperation (const ACAN2515Settings & inSettings,
                                           const ACAN2515Mask inRXM0,
                                           const ACAN2515Mask inRXM1,
                                           const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                           const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ; // Ok be default
//----------------------------------- Check if MCP2515 is accessible
  mSPI.beginTransaction (mSPISettings) ;
    write2515Register (CNF1_REGISTER, 0x55) ;
    bool ok = read2515Register (CNF1_REGISTER) == 0x55 ;
    if (ok) {
      write2515Register (CNF1_REGISTER, 0xAA) ;
      ok = read2515Register (CNF1_REGISTER) == 0xAA ;
    }
    if (!ok) {
      errorCode = kNoMCP2515 ;
    }
  mSPI.endTransaction () ;
//----------------------------------- Check if settings are correct
  if (!inSettings.mBitRateClosedToDesiredRate) {
    errorCode |= kTooFarFromDesiredBitRate ;
  }
  if (inSettings.CANBitSettingConsistency () != 0) {
    errorCode |= kInconsistentBitRateSettings ;
  }
//----------------------------------- Allocate buffer
  if (!mReceiveBuffer.initWithSize (inSettings.mReceiveBufferSize)) {
    errorCode |= kCannotAllocateReceiveBuffer ;
  }
  if (!mTransmitBuffer [0].initWithSize (inSettings.mTransmitBuffer0Size)) {
    errorCode |= kCannotAllocateTransmitBuffer0 ;
  }
  if (!mTransmitBuffer [1].initWithSize (inSettings.mTransmitBuffer1Size)) {
    errorCode |= kCannotAllocateTransmitBuffer1 ;
  }
  if (!mTransmitBuffer [2].initWithSize (inSettings.mTransmitBuffer2Size)) {
    errorCode |= kCannotAllocateTransmitBuffer2 ;
  }
  mTXBIsFree [0] = true ;
  mTXBIsFree [1] = true ;
  mTXBIsFree [2] = true ;
//----------------------------------- If ok, perform configuration
  if (errorCode == 0) {
    mSPI.beginTransaction (mSPISettings) ;
  //----------------------------------- Set CNF3, CNF2, CNF1 and CANINTE registers
    select () ;
    mSPI.transfer (WRITE_COMMAND) ;
    mSPI.transfer (CNF3_REGISTER) ;
  //--- Register CNF3:
  //  Bit 7: SOF
  //  bit 6 --> 0: No Wake-up Filter bit
  //  Bit 5-3: -
  //  Bit 2-0: PHSEG2 - 1
    const uint8_t cnf3 =
      ((inSettings.mCLKOUT_SOF_pin == ACAN2515Settings::SOF) << 6) /* SOF */ |
      ((inSettings.mPhaseSegment2 - 1) << 0) /* PHSEG2 */
    ;
   mSPI.transfer (cnf3) ;
  //--- Register CNF2:
  //  Bit 7 --> 1: BLTMODE
  //  bit 6: SAM
  //  Bit 5-3: PHSEG1 - 1
  //  Bit 2-0: PRSEG - 1
    const uint8_t cnf2 =
      0x80 /* BLTMODE */ |
      (inSettings.mTripleSampling << 6) /* SAM */ |
      ((inSettings.mPhaseSegment1 - 1) << 3) /* PHSEG1 */ |
      ((inSettings.mPropagationSegment - 1) << 0) /* PRSEG */
    ;
    mSPI.transfer (cnf2) ;
  //--- Register CNF1:
  //  Bit 7-6: SJW - 1
  //  Bit 5-0: BRP - 1
    const uint8_t cnf1 =
      ((inSettings.mSJW - 1) << 6) /* SJW */ | // Incorrect SJW setting fixed in 2.0.1
      ((inSettings.mBitRatePrescaler - 1) << 0) /* BRP */
    ;
    mSPI.transfer (cnf1) ;
  //--- Register CANINTE: activate interrupts
  //  Bit 7 --> 0: MERRE
  //  Bit 6 --> 0: WAKIE
  //  Bit 5 --> 0: ERRIE
  //  Bit 4 --> 1: TX2IE
  //  Bit 3 --> 1: TX1IE
  //  Bit 2 --> 1: TX0IE
  //  Bit 1 --> 1: RX1IE
  //  Bit 0 --> 1: RX0IE
    mSPI.transfer (0x1F) ;
    unselect () ;
  //----------------------------------- Deactivate the RXnBF Pins (High Impedance State)
    write2515Register (BFPCTRL_REGISTER, 0) ;
  //----------------------------------- Set TXnRTS as inputs
    write2515Register (TXRTSCTRL_REGISTER, 0);
  //----------------------------------- RXBnCTRL
    mRolloverEnable = inSettings.mRolloverEnable ;
    const uint8_t acceptAll = (inAcceptanceFilterCount == 0) ? 0x60 : 0x00 ;
    write2515Register (RXB0CTRL_REGISTER, acceptAll | (uint8_t (inSettings.mRolloverEnable) << 2)) ;
    write2515Register (RXB1CTRL_REGISTER, acceptAll) ;
  //----------------------------------- Setup mask registers
    setupMaskRegister (inRXM0, RXM0SIDH_REGISTER) ;
    setupMaskRegister (inRXM1, RXM1SIDH_REGISTER) ;
    if (inAcceptanceFilterCount > 0) {
      uint8_t idx = 0 ;
      while (idx < inAcceptanceFilterCount) {
        setupMaskRegister (inAcceptanceFilters [idx].mMask, RXFSIDH_REGISTER [idx]) ;
        mCallBackFunctionArray [idx] = inAcceptanceFilters [idx].mCallBack ;
        idx += 1 ;
      }
      while (idx < 6) {
        setupMaskRegister (inAcceptanceFilters [inAcceptanceFilterCount-1].mMask, RXFSIDH_REGISTER [idx]) ;
        mCallBackFunctionArray [idx] = inAcceptanceFilters [inAcceptanceFilterCount-1].mCallBack ;
        idx += 1 ;
      }
    }
  //----------------------------------- Set TXBi priorities
    write2515Register (TXB0CTRL_REGISTER, inSettings.mTXBPriority & 3) ;
    write2515Register (TXB1CTRL_REGISTER, (inSettings.mTXBPriority >> 2) & 3) ;
    write2515Register (TXB2CTRL_REGISTER, (inSettings.mTXBPriority >> 4) & 3) ;
    mSPI.endTransaction () ;
  //----------------------------------- Reset device to requested mode
    uint8_t canctrl = inSettings.mOneShotModeEnabled ? (1 << 3) : 0 ;
    switch (inSettings.mCLKOUT_SOF_pin) {
    case ACAN2515Settings::CLOCK :
      canctrl |= 0x04 | 0x00 ; // Same as default setting
      break ;
    case ACAN2515Settings::CLOCK2 :
      canctrl |= 0x04 | 0x01 ;
      break ;
    case ACAN2515Settings::CLOCK4 :
      canctrl |= 0x04 | 0x02 ;
      break ;
    case ACAN2515Settings::CLOCK8 :
      canctrl |= 0x04 | 0x03 ;
      break ;
    case ACAN2515Settings::SOF :
      canctrl |= 0x04 ;
      break ;
    case ACAN2515Settings::HiZ :
      break ;
    }
  //--- Request mode
    const uint8_t requestedMode = (uint8_t) inSettings.mRequestedMode ;
    errorCode |= setRequestedMode (canctrl | requestedMode) ;
  }
//-----------------------------------
  return errorCode ;
}

//··································································································
//   setRequestedMode (private method)
//··································································································

uint16_t ACAN2515::setRequestedMode (const uint8_t inCANControlRegister) {
  uint16_t errorCode = 0 ;
//--- Request mode
  mSPI.beginTransaction (mSPISettings) ;
    write2515Register (CANCTRL_REGISTER, inCANControlRegister) ;
  mSPI.endTransaction () ;
//--- Wait until requested mode is reached (during 1 or 2 ms)
  bool wait = true ;
  const uint32_t deadline = millis () + 2 ;
  while (wait) {
    mSPI.beginTransaction (mSPISettings) ;
      const uint8_t actualMode = read2515Register (CANSTAT_REGISTER) & 0xE0 ;
    mSPI.endTransaction () ;
    wait = actualMode != (inCANControlRegister & 0xE0) ;
    if (wait && (millis () >= deadline)) {
      errorCode |= kRequestedModeTimeOut ;
      wait = false ;
    }
  }
//---
  return errorCode ;
}

//··································································································
//    Change Mode
//··································································································

uint16_t ACAN2515::changeModeOnTheFly (const ACAN2515Settings::RequestedMode inRequestedMode) {
//--- Read current mode register (for saving settings of bits 0 ... 4)
  mSPI.beginTransaction (mSPISettings) ;
    const uint8_t currentMode = read2515Register (CANCTRL_REGISTER) ;
  mSPI.endTransaction () ;
//--- New mode
  const uint8_t newMode = (currentMode & 0x1F) | (uint8_t) inRequestedMode ;
//--- Set new mode
  const uint16_t errorCode = setRequestedMode (newMode) ;
//---
  return errorCode ;
}

//··································································································
//    Set filters on the fly
//··································································································

uint16_t ACAN2515::setFiltersOnTheFly (void) {
  return internalSetFiltersOnTheFly (ACAN2515Mask (), ACAN2515Mask (), NULL, 0) ;
}

//··································································································

uint16_t ACAN2515::setFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                       const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                       const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ;
  if (inAcceptanceFilterCount == 0) {
    errorCode = kOneFilterMaskRequiresOneOrTwoAcceptanceFilters ;
  }else if (inAcceptanceFilterCount > 2) {
    errorCode = kOneFilterMaskRequiresOneOrTwoAcceptanceFilters ;
  }else if (inAcceptanceFilters == NULL) {
    errorCode = kAcceptanceFilterArrayIsNULL ;
  }else{
    errorCode = internalSetFiltersOnTheFly (inRXM0, ACAN2515Mask (), inAcceptanceFilters, inAcceptanceFilterCount) ;
  }
  return errorCode ;
}

//··································································································

uint16_t ACAN2515::setFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                       const ACAN2515Mask inRXM1,
                                       const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                       const uint8_t inAcceptanceFilterCount) {
  uint16_t errorCode = 0 ;
  if (inAcceptanceFilterCount < 3) {
    errorCode = kTwoFilterMasksRequireThreeToSixAcceptanceFilters ;
  }else if (inAcceptanceFilterCount > 6) {
    errorCode = kTwoFilterMasksRequireThreeToSixAcceptanceFilters ;
  }else if (inAcceptanceFilters == NULL) {
    errorCode = kAcceptanceFilterArrayIsNULL ;
  }else{
    errorCode = internalSetFiltersOnTheFly (inRXM0, inRXM1, inAcceptanceFilters, inAcceptanceFilterCount) ;
  }
  return errorCode ;
}

//··································································································

uint16_t ACAN2515::internalSetFiltersOnTheFly (const ACAN2515Mask inRXM0,
                                               const ACAN2515Mask inRXM1,
                                               const ACAN2515AcceptanceFilter inAcceptanceFilters [],
                                               const uint8_t inAcceptanceFilterCount) {
//--- Read current mode register
  mSPI.beginTransaction (mSPISettings) ;
    const uint8_t currentMode = read2515Register (CANCTRL_REGISTER) ;
  mSPI.endTransaction () ;
//--- Request configuration mode
  const uint8_t configurationMode = (currentMode & 0x1F) | (0b100 << 5) ; // Preserve bits 0 ... 4
  uint16_t errorCode = setRequestedMode (configurationMode) ;
//--- Setup mask registers
  if (errorCode == 0) {
    const uint8_t acceptAll = (inAcceptanceFilterCount == 0) ? 0x60 : 0x00 ;
    write2515Register (RXB0CTRL_REGISTER, acceptAll | (uint8_t (mRolloverEnable) << 2)) ;
    write2515Register (RXB1CTRL_REGISTER, acceptAll) ;
    setupMaskRegister (inRXM0, RXM0SIDH_REGISTER) ;
    setupMaskRegister (inRXM1, RXM1SIDH_REGISTER) ;
    if (inAcceptanceFilterCount > 0) {
      uint8_t idx = 0 ;
      while (idx < inAcceptanceFilterCount) {
        setupMaskRegister (inAcceptanceFilters [idx].mMask, RXFSIDH_REGISTER [idx]) ;
        mCallBackFunctionArray [idx] = inAcceptanceFilters [idx].mCallBack ;
        idx += 1 ;
      }
      while (idx < 6) {
        setupMaskRegister (inAcceptanceFilters [inAcceptanceFilterCount-1].mMask, RXFSIDH_REGISTER [idx]) ;
        mCallBackFunctionArray [idx] = inAcceptanceFilters [inAcceptanceFilterCount-1].mCallBack ;
        idx += 1 ;
      }
    }
  }
//--- Restore saved mode
  if (errorCode == 0) {
    errorCode = setRequestedMode (currentMode) ;
  }
//---
  return errorCode ;
}

//··································································································
//    end
//··································································································

void ACAN2515::end (void) {
//--- Remove interrupt capability of mINT pin
  if (mINT != 255) {
    detachInterrupt (digitalPinToInterrupt (mINT)) ;
  }
//--- Request configuration mode
  const uint8_t configurationMode = (0b100 << 5) ;
  const uint16_t errorCode __attribute__((unused)) = setRequestedMode (configurationMode) ;
//--- Deallocate driver buffers
  mTransmitBuffer [0].free () ;
  mTransmitBuffer [1].free () ;
  mTransmitBuffer [2].free () ;
  mReceiveBuffer.free () ;
}


//··································································································
//    POLLING (ESP32)
//··································································································

#ifdef ARDUINO_ARCH_ESP32
  void ACAN2515::poll (void) {
    xSemaphoreGive (mISRSemaphore) ;
  }
#endif

//··································································································
//    POLLING (other than ESP32)
//··································································································

#ifndef ARDUINO_ARCH_ESP32
  void ACAN2515::poll (void) {
    noInterrupts () ;
      while (isr_core ()) {}
    interrupts () ;
  }
#endif

//··································································································
//   INTERRUPT SERVICE ROUTINE (ESP32)
// https://stackoverflow.com/questions/51750377/how-to-disable-interrupt-watchdog-in-esp32-or-increase-isr-time-limit
//··································································································

#ifdef ARDUINO_ARCH_ESP32
  void IRAM_ATTR ACAN2515::isr (void) {
    detachInterrupt (digitalPinToInterrupt (mINT)) ;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE ;
    xSemaphoreGiveFromISR (mISRSemaphore, &xHigherPriorityTaskWoken) ;
    if (xHigherPriorityTaskWoken) {
      portYIELD_FROM_ISR () ;
    }
  }
#endif

//··································································································
//   INTERRUPT SERVICE ROUTINE (other than ESP32)
//··································································································

#ifndef ARDUINO_ARCH_ESP32
  void ACAN2515::isr (void) {
    isr_core () ;
  }
#endif

//··································································································

bool ACAN2515::isr_core (void) {
  bool handled = false ;
  mSPI.beginTransaction (mSPISettings) ;
  uint8_t itStatus = read2515Register (CANSTAT_REGISTER) & 0x0E ;
  while (itStatus != 0) {
    handled = true ;
    switch (itStatus) {
    case 0 : // No interrupt
      break ;
    case 1 << 1 : // Error interrupt
      bitModify2515Register (CANINTF_REGISTER, 0x20, 0) ; // Ack interrupt
      break ;
    case 2 << 1 : // Wake-up interrupt
      bitModify2515Register (CANINTF_REGISTER, 0x40, 0) ; // Ack interrupt
      break ;
    case 3 << 1 : // TXB0 interrupt
      handleTXBInterrupt (0) ;
      break ;
    case 4 << 1 : // TXB1 interrupt
      handleTXBInterrupt (1) ;
      break ;
    case 5 << 1 : // TXB2 interrupt
      handleTXBInterrupt (2) ;
      break ;
    case 6 << 1 : // RXB0 interrupt
    case 7 << 1 : // RXB1 interrupt
      handleRXBInterrupt () ;
      break ;
    }
    itStatus = read2515Register (CANSTAT_REGISTER) & 0x0E ;
  }
  mSPI.endTransaction () ;
  return handled ;
}

//··································································································
// This function is called by ISR when a MCP2515 receive buffer becomes full

void ACAN2515::handleRXBInterrupt (void) {
  const uint8_t rxStatus = read2515RxStatus () ; // Bit 6: message in RXB0, bit 7: message in RXB1
  const bool received = (rxStatus & 0xC0) != 0 ;
  if (received) { // Message in RXB0 and / or RXB1
    const bool accessRXB0 = (rxStatus & 0x40) != 0 ;
    CANMessage message ;
  //--- Set idx field to matching receive filter
    message.idx = rxStatus & 0x07 ;
    if (message.idx > 5) {
      message.idx -= 6 ;
    }
  //---
    select () ;
    mSPI.transfer (accessRXB0 ? READ_FROM_RXB0SIDH_COMMAND : READ_FROM_RXB1SIDH_COMMAND) ;
  //--- SIDH
    message.id = mSPI.transfer (0) ;
    message.id <<= 3 ;
  //--- SIDL
    const uint32_t sidl = mSPI.transfer (0) ;
    message.id |= sidl >> 5 ;
    message.rtr = (sidl & 0x10) != 0 ; // Only significant for standard frame
    message.ext = (sidl & 0x08) != 0 ;
  //--- EID8
    const uint32_t eid8 = mSPI.transfer (0) ;
    if (message.ext) {
      message.id <<= 2 ;
      message.id |= (sidl & 0x03) ;
      message.id <<= 8 ;
      message.id |= eid8 ;
    }
  //--- EID0
    const uint32_t eid0 = mSPI.transfer (0) ;
    if (message.ext) {
      message.id <<= 8 ;
      message.id |= eid0 ;
    }
  //--- DLC
    const uint8_t dlc = mSPI.transfer (0) ;
    message.len = dlc & 0x0F ;
    if (message.ext) { // Added in 2.1.1 (thanks to Achilles)
      message.rtr = (dlc & 0x40) != 0 ; // RTR bit in DLC is significant only for extended frame
    }
  //--- Read data
    for (int i=0 ; i<message.len ; i++) {
      message.data [i] = mSPI.transfer (0) ;
    }
  //---
    unselect () ;
  //--- Free receive buffer command
    bitModify2515Register (CANINTF_REGISTER, accessRXB0 ? 0x01 : 0x02, 0) ;
  //--- Enter received message in receive buffer (if not full)
    mReceiveBuffer.append (message) ;
  }
}

//··································································································
// This function is called by ISR when a MCP2515 transmit buffer becomes empty

void ACAN2515::handleTXBInterrupt (const uint8_t inTXB) { // inTXB value is 0, 1 or 2
//--- Acknowledge interrupt
  bitModify2515Register (CANINTF_REGISTER, 0x04 << inTXB, 0) ;
//--- Send an other message ?
  CANMessage message ;
  const bool ok = mTransmitBuffer [inTXB].remove (message) ;
  if (ok) {
    internalSendMessage (message, inTXB) ;
  }else{
    mTXBIsFree [inTXB] = true ;
  }
}

//··································································································

void ACAN2515::internalSendMessage (const CANMessage & inFrame, const uint8_t inTXB) { // inTXB is 0, 1 or 2
//--- Send command
//      send via TXB0: 0x81
//      send via TXB1: 0x82
//      send via TXB2: 0x84
  const uint8_t sendCommand = REQUEST_TO_SEND_COMMAND | (1 << inTXB) ;
//--- Load TX buffer command
//      Load TXB0, start at TXB0SIDH: 0x40
//      Load TXB1, start at TXB1SIDH: 0x42
//      Load TXB2, start at TXB2SIDH: 0x44
  const uint8_t loadTxBufferCommand = LOAD_TX_BUFFER_COMMAND | (inTXB << 1) ;
//--- Send message
  select () ;
  mSPI.transfer (loadTxBufferCommand) ;
  if (inFrame.ext) { // Extended frame
    uint32_t v = inFrame.id >> 21 ;
    mSPI.transfer ((uint8_t) v) ; // ID28 ... ID21 --> SIDH
    v  = (inFrame.id >> 13) & 0xE0 ; // ID20, ID19, ID18 in bits 7, 6, 5
    v |= (inFrame.id >> 16) & 0x03 ; // ID17, ID16 in bits 1, 0
    v |= 0x08 ; // Extended bit
    mSPI.transfer ((uint8_t) v) ; // ID20, ID19, ID18, -, 1, -, ID17, ID16 --> SIDL
    v  = (inFrame.id >> 8) & 0xFF ; // ID15, ..., ID8
    mSPI.transfer ((uint8_t) v) ; // ID15, ID14, ID13, ID12, ID11, ID10, ID9, ID8 --> EID8
    v  = inFrame.id & 0xFF ; // ID7, ..., ID0
    mSPI.transfer ((uint8_t) v) ; // ID7, ID6, ID5, ID4, ID3, ID2, ID1, ID0 --> EID0
  }else{ // Standard frame
    uint32_t v = inFrame.id >> 3 ;
    mSPI.transfer ((uint8_t) v) ; // ID10 ... ID3 --> SIDH
    v  = (inFrame.id << 5) & 0xE0 ; // ID2, ID1, ID0 in bits 7, 6, 5
    mSPI.transfer ((uint8_t) v) ; // ID2, ID1, ID0, -, 0, -, 0, 0 --> SIDL
    mSPI.transfer (0x00) ; // any value --> EID8
    mSPI.transfer (0x00) ; // any value --> EID0
  }
//--- DLC
  uint8_t v = inFrame.len ;
  if (v > 8) {
    v = 8 ;
  }
  if (inFrame.rtr) {
    v |= 0x40 ;
  }
  mSPI.transfer (v) ;
//--- Send data
  if (!inFrame.rtr) {
    for (uint8_t i=0 ; i<inFrame.len ; i++) {
      mSPI.transfer (inFrame.data [i]) ;
    }
  }
  unselect () ;
//--- Write send command
  select () ;
    mSPI.transfer (sendCommand) ;
  unselect () ;
}

//··································································································
//  INTERNAL SPI FUNCTIONS
//··································································································

void ACAN2515::write2515Register (const uint8_t inRegister, const uint8_t inValue) {
  select () ;
    mSPI.transfer (WRITE_COMMAND) ;
    mSPI.transfer (inRegister) ;
    mSPI.transfer (inValue) ;
  unselect () ;
}

//··································································································

uint8_t ACAN2515::read2515Register (const uint8_t inRegister) {
  select () ;
    mSPI.transfer (READ_COMMAND) ;
    mSPI.transfer (inRegister) ;
    const uint8_t readValue = mSPI.transfer (0) ;
  unselect () ;
  return readValue ;
}

//··································································································

uint8_t ACAN2515::read2515Status (void) {
  select () ;
    mSPI.transfer (READ_STATUS_COMMAND) ;
    const uint8_t readValue = mSPI.transfer (0) ;
  unselect () ;
  return readValue ;
}

//··································································································

uint8_t ACAN2515::read2515RxStatus (void) {
  select () ;
    mSPI.transfer (RX_STATUS_COMMAND) ;
    const uint8_t readValue = mSPI.transfer (0) ;
  unselect () ;
  return readValue ;
}

//··································································································

void ACAN2515::bitModify2515Register (const uint8_t inRegister,
                                      const uint8_t inMask,
                                      const uint8_t inData) {
  select () ;
    mSPI.transfer (BIT_MODIFY_COMMAND) ;
    mSPI.transfer (inRegister) ;
    mSPI.transfer (inMask) ;
    mSPI.transfer (inData) ;
  unselect () ;
}

//··································································································

void ACAN2515::setupMaskRegister (const ACAN2515Mask inMask, const uint8_t inRegister) {
  select () ;
    mSPI.transfer (WRITE_COMMAND) ;
    mSPI.transfer (inRegister) ;
    mSPI.transfer (inMask.mSIDH) ;
    mSPI.transfer (inMask.mSIDL) ;
    mSPI.transfer (inMask.mEID8) ;
    mSPI.transfer (inMask.mEID0) ;
  unselect () ;
}

//··································································································
//   MCP2515 controller state
//··································································································

uint8_t ACAN2515::transmitErrorCounter (void) {
  mSPI.beginTransaction (mSPISettings) ;
    const uint8_t result = read2515Register (TEC_REGISTER) ;
  mSPI.endTransaction () ;
  return result ;
}

//··································································································

uint8_t ACAN2515::receiveErrorCounter (void) {
  mSPI.beginTransaction (mSPISettings) ;
    const uint8_t result = read2515Register (REC_REGISTER) ;
  mSPI.endTransaction () ;
  return result ;
}

//··································································································

uint8_t ACAN2515::errorFlagRegister (void) {
  mSPI.beginTransaction (mSPISettings) ;
    const uint8_t result = read2515Register (EFLG_REGISTER) ;
  mSPI.endTransaction () ;
  return result ;
}

//··································································································
//   MESSAGE EMISSION
//··································································································

bool ACAN2515::sendBufferNotFullForIndex (const uint32_t inIndex) {
//--- Fix send buffer index
  uint8_t idx = inIndex ;
  if (idx > 2) {
    idx = 0 ;
  }
  #ifndef ARDUINO_ARCH_ESP32
    noInterrupts () ;
  #endif
    mSPI.beginTransaction (mSPISettings) ;
      const bool ok = mTXBIsFree [idx] || !mTransmitBuffer [idx].isFull () ;
    mSPI.endTransaction () ;
  #ifndef ARDUINO_ARCH_ESP32
    interrupts () ;
  #endif
  return ok ;
}

//··································································································

bool ACAN2515::tryToSend (const CANMessage & inMessage) {
//--- Fix send buffer index
  uint8_t idx = inMessage.idx ;
  if (idx > 2) {
    idx = 0 ;
  }
//--- Bug fix in 2.0.6 (thanks to Fergus Duncan): interrupts were only disabled for Teensy boards
  #ifndef ARDUINO_ARCH_ESP32
    noInterrupts () ;
  #endif
   //---
    mSPI.beginTransaction (mSPISettings) ;
      bool ok = mTXBIsFree [idx] ;
      if (ok) { // Transmit buffer and TXB are both free: transmit immediatly
        mTXBIsFree [idx] = false ;
        internalSendMessage (inMessage, idx) ;
      }else{ // Enter in transmit buffer, if not full
        ok = mTransmitBuffer [idx].append (inMessage) ;
      }
    mSPI.endTransaction () ;
  #ifndef ARDUINO_ARCH_ESP32
    interrupts () ;
  #endif
  return ok ;
}

//··································································································
