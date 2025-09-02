//----------------------------------------------------------------------------------------------------------------------
// A CAN driver for MCP2517FD, CANFD mode
// by Pierre Molinaro
// https://github.com/pierremolinaro/acan2517FD
//
//----------------------------------------------------------------------------------------------------------------------

#include "ACAN2517FD.h"
#include "../../system_settings.h" //Contains task priority

//----------------------------------------------------------------------------------------------------------------------

static const uint8_t TXBWS = 0 ;

//----------------------------------------------------------------------------------------------------------------------
// Note about ESP32
//----------------------------------------------------------------------------------------------------------------------
//
// It appears that Arduino ESP32 interrupts are managed in a completely different way from "usual" Arduino:
//   - SPI.usingInterrupt is not implemented;
//   - noInterrupts() and interrupts() are NOPs: use taskDISABLE_INTERRUPTS and taskENABLE_INTERRUPTS;
//   - interrupt service routines should be fast, otherwise you get an "Guru Meditation Error: Core 1 panic'ed
//     (Interrupt wdt timeout on CPU1)".
//
// So we handle the ESP32 interrupt in the following way:
//   - interrupt service routine performs a xSemaphoreGiveFromISR on mISRSemaphore of can driver
//   - this activates the myESP32Task task that performs "isr_poll_core" that is done by interrupt service routine
//     in "usual" Arduino;
//   - as this task runs in parallel with setup / loop routines, SPI access is natively protected by the
//     beginTransaction / endTransaction pair, that manages a mutex;
//   - (May 29, 2019) it appears that MCP2717FD wants the CS line to deasserted as soon as possible (thanks for
//     Nick Kirkby for having signaled me this point, see https://github.com/pierremolinaro/acan2517/issues/5);
//     so we mask interrupts when we access the MCP2517FD, the sequence becomes:
//           mSPI.beginTransaction (mSPISettings) ;
//             #ifdef ARDUINO_ARCH_ESP32
//               taskDISABLE_INTERRUPTS () ;
//             #endif
//               assertCS () ;
//                  ... Access the MCP2517FD ...
//               deassertCS () ;
//             #ifdef ARDUINO_ARCH_ESP32
//               taskENABLE_INTERRUPTS () ;
//             #endif
//           mSPI.endTransaction () ;
//
//----------------------------------------------------------------------------------------------------------------------

#ifdef ARDUINO_ARCH_ESP32
  static void myESP32Task (void * pData) {
    ACAN2517FD * canDriver = (ACAN2517FD *) pData ;
    while (1) {
      xSemaphoreTake (canDriver->mISRSemaphore, portMAX_DELAY) ;
      canDriver->isr_poll_core () ;
    }
  }
#endif

//----------------------------------------------------------------------------------------------------------------------
// ACAN2517FD register addresses
//----------------------------------------------------------------------------------------------------------------------

static const uint16_t CON_REGISTER      = 0x000 ;
static const uint16_t NBTCFG_REGISTER   = 0x004 ;
static const uint16_t DBTCFG_REGISTER   = 0x008 ;
static const uint16_t TDC_REGISTER      = 0x00C ;

static const uint16_t TREC_REGISTER     = 0x034 ;
static const uint16_t BDIAG0_REGISTER   = 0x038 ;
static const uint16_t BDIAG1_REGISTER   = 0x03C ;

//······················································································································
//   TXQ REGISTERS
//······················································································································

static const uint16_t TXQCON_REGISTER   = 0x050 ;
static const uint16_t TXQSTA_REGISTER   = 0x054 ;
static const uint16_t TXQUA_REGISTER    = 0x058 ;

//······················································································································
//   INTERRUPT REGISTERS
//······················································································································

static const uint16_t INT_REGISTER = 0x01C ;

//······················································································································
//   FIFO REGISTERS
//······················································································································

static uint16_t FIFOCON_REGISTER (const uint16_t inFIFOIndex) { // 1 ... 31
  return 0x05C + 12 * (inFIFOIndex - 1) ;
}

//······················································································································

static uint16_t FIFOSTA_REGISTER (const uint16_t inFIFOIndex) { // 1 ... 31
  return 0x060 + 12 * (inFIFOIndex - 1) ;
}

//······················································································································

static uint16_t FIFOUA_REGISTER (const uint16_t inFIFOIndex) { // 1 ... 31
  return 0x064 + 12 * (inFIFOIndex - 1) ;
}

//······················································································································
//   FILTER REGISTERS
//······················································································································

static uint16_t FLTCON_REGISTER (const uint16_t inFilterIndex) { // 0 ... 31 (DS20005688B, page 58)
  return 0x1D0 + inFilterIndex ;
}

//······················································································································

static uint16_t FLTOBJ_REGISTER (const uint16_t inFilterIndex) { // 0 ... 31 (DS20005688B, page 60)
  return 0x1F0 + 8 * inFilterIndex ;
}

//······················································································································

static uint16_t MASK_REGISTER (const uint16_t inFilterIndex) { // 0 ... 31 (DS20005688B, page 61)
  return 0x1F4 + 8 * inFilterIndex ;
}

//······················································································································
//   OSCILLATOR REGISTER
//······················································································································

static const uint16_t OSC_REGISTER = 0xE00 ;

//······················································································································
//   INPUT / OUPUT CONTROL REGISTER
//······················································································································

static const uint16_t IOCON_REGISTER_00_07 = 0xE04 ;
static const uint16_t IOCON_REGISTER_08_15 = 0xE05 ;
static const uint16_t IOCON_REGISTER_16_23 = 0xE06 ;
static const uint16_t IOCON_REGISTER_24_31 = 0xE07 ;

//----------------------------------------------------------------------------------------------------------------------
//    RECEIVE FIFO INDEX
//----------------------------------------------------------------------------------------------------------------------

static const uint8_t RECEIVE_FIFO_INDEX  = 1 ;
static const uint8_t TRANSMIT_FIFO_INDEX = 2 ;

//----------------------------------------------------------------------------------------------------------------------
//    BYTE BUFFER UTILITY FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------

static void enterU32InBufferAtIndex (const uint32_t inValue, uint8_t ioBuffer [], const uint8_t inIndex) {
  ioBuffer [inIndex + 0] = (uint8_t) inValue ;
  ioBuffer [inIndex + 1] = (uint8_t) (inValue >>  8) ;
  ioBuffer [inIndex + 2] = (uint8_t) (inValue >> 16) ;
  ioBuffer [inIndex + 3] = (uint8_t) (inValue >> 24) ;
}

//······················································································································

static uint32_t u32FromBufferAtIndex (uint8_t ioBuffer [], const uint8_t inIndex) {
  uint32_t result = (uint32_t) ioBuffer [inIndex + 0] ;
  result |= ((uint32_t) ioBuffer [inIndex + 1]) <<  8 ;
  result |= ((uint32_t) ioBuffer [inIndex + 2]) << 16 ;
  result |= ((uint32_t) ioBuffer [inIndex + 3]) << 24 ;
  return result ;
}

//······················································································································

static uint16_t u16FromBufferAtIndex (uint8_t ioBuffer [], const uint8_t inIndex) {
  uint16_t result = (uint16_t) ioBuffer [inIndex + 0] ;
  result |= ((uint16_t) ioBuffer [inIndex + 1]) <<  8 ;
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

static inline void turnOffInterrupts () {
  #ifndef DISABLEMCP2517FDCOMPAT
    #ifdef ARDUINO_ARCH_ESP32
      taskDISABLE_INTERRUPTS () ;
    #else
      noInterrupts () ;
    #endif
  #endif
}

//----------------------------------------------------------------------------------------------------------------------

static inline void turnOnInterrupts() {
  #ifndef DISABLEMCP2517FDCOMPAT
    #ifdef ARDUINO_ARCH_ESP32
      taskENABLE_INTERRUPTS () ;
    #else
      interrupts () ;
    #endif
  #endif
}

//----------------------------------------------------------------------------------------------------------------------

ACAN2517FD::ACAN2517FD (const uint8_t inCS, // CS input of MCP2517FD
                        SPIClass & inSPI, // Hardware SPI object
                        const uint8_t inINT) : // INT output of MCP2517FD
mSPISettings (),
mSPI (inSPI),
mCS (inCS),
mINT (inINT),
mUsesTXQ (false),
mHardwareTxFIFOFull (false),
mRxInterruptEnabled (true),
mHasDataBitRate (false),
mTransmitFIFOPayload (0),
mTXQBufferPayload (0),
mReceiveFIFOPayload (0),
mTXBWS_RequestedMode (0),
mHardwareReceiveBufferOverflowCount (0),
mDriverReceiveBuffer (),
mDriverTransmitBuffer ()
#ifdef ARDUINO_ARCH_ESP32
  , mISRSemaphore (xSemaphoreCreateCounting (10, 0))
#endif
{
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::begin (const ACAN2517FDSettings & inSettings,
                            void (* inInterruptServiceRoutine) (void)) {
//--- Add pass-all filter
  ACAN2517FDFilters filters ;
  filters.appendPassAllFilter (NULL) ;
//---
  return begin (inSettings, inInterruptServiceRoutine, filters) ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::begin (const ACAN2517FDSettings & inSettings,
                            void (* inInterruptServiceRoutine) (void),
                            const ACAN2517FDFilters & inFilters) {
  uint32_t errorCode = 0 ; // Means no error
//----------------------------------- If ok, check if settings are correct
  if (!inSettings.mArbitrationBitRateClosedToDesiredRate) {
    errorCode |= kTooFarFromDesiredBitRate ;
  }
  if (inSettings.CANBitSettingConsistency () != 0) {
    errorCode |= kInconsistentBitRateSettings ;
  }
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
//----------------------------------- Check TXQ size is <= 32
  if (inSettings.mControllerTXQSize > 32) {
    errorCode |= kControllerTXQSizeGreaterThan32 ;
  }
//----------------------------------- Check TXQ priority is <= 31
  if (inSettings.mControllerTXQBufferPriority > 31) {
    errorCode |= kControllerTXQPriorityGreaterThan31 ;
  }
//----------------------------------- Check controller receive FIFO size is 1 ... 32
  if (inSettings.mControllerReceiveFIFOSize == 0) {
    errorCode |= kControllerReceiveFIFOSizeIsZero ;
  }else if (inSettings.mControllerReceiveFIFOSize > 32) {
    errorCode |= kControllerReceiveFIFOSizeGreaterThan32 ;
  }
//----------------------------------- Check controller transmit FIFO size is 1 ... 32
  if (inSettings.mControllerTransmitFIFOSize == 0) {
    errorCode |= kControllerTransmitFIFOSizeIsZero ;
  }else if (inSettings.mControllerTransmitFIFOSize > 32) {
    errorCode |= kControllerTransmitFIFOSizeGreaterThan32 ;
  }
//----------------------------------- Check Transmit FIFO priority is <= 31
  if (inSettings.mControllerTransmitFIFOPriority > 31) {
    errorCode |= kControllerTransmitFIFOPriorityGreaterThan31 ;
  }
//----------------------------------- Check MCP2517FD controller RAM usage is <= 2048 bytes
  if (inSettings.ramUsage () > 2048) {
    errorCode |= kControllerRamUsageGreaterThan2048 ;
  }
//----------------------------------- Check Filter definition
  if (inFilters.filterCount () > 32) {
    errorCode |= kMoreThan32Filters ;
  }
  if (inFilters.filterStatus () != ACAN2517FDFilters::kFiltersOk) {
    errorCode |= kFilterDefinitionError ;
  }
//----------------------------------- Check TDCO value
  if ((inSettings.mTDCO > 63) || (inSettings.mTDCO < -64)) {
    errorCode |= kInvalidTDCO ;
  }
//----------------------------------- INT, CS pins, reset MCP2517FD
  if (errorCode == 0) {
    if (mINT != 255) { // 255 means interrupt is not used (thanks to Tyler Lewis)
      pinMode (mINT, INPUT_PULLUP) ;
    }
    initCS () ;
  //----------------------------------- Set SPI clock to 800 kHz
    mSPISettings = SPISettings (800UL * 1000, MSBFIRST, SPI_MODE0) ;
  //----------------------------------- Request configuration mode
    bool wait = true ;
    const uint32_t startTime = millis () ;
    while (wait) {
      writeRegister8 (CON_REGISTER + 3, 0x04 | (1 << 3)) ; // Request configuration mode, abort all transmissions
      const uint8_t actualMode = (readRegister8 (CON_REGISTER + 2) >> 5) & 0x07 ;
      wait = actualMode != 0x04 ;
      if (wait && ((millis () - startTime) > 2)) { // Wait (2 ms max) until the configuration mode is reached
        errorCode |= kRequestedConfigurationModeTimeOut ;
        wait = false ;
      }
    }
  //----------------------------------- Reset MCP2517FD (always use a 800 kHz clock)
    reset2517FD () ;
  }
//----------------------------------- Check SPI connection is on (with a 800 kHz clock)
// We write and the read back MCP2517FD RAM at address 0x400
  for (uint32_t i=1 ; (i != 0) && (errorCode == 0) ; i <<= 1) {
    const uint16_t RAM_WORD_ADDRESS = 0x400 ;
    writeRegister32 (RAM_WORD_ADDRESS, i) ;
    const uint32_t readBackValue = readRegister32 (RAM_WORD_ADDRESS) ;
    if (readBackValue != i) {
      errorCode = kReadBackErrorWith1MHzSPIClock ;
    }
  }
//----------------------------------- Now, set internal clock with OSC register
//     Bit 0: (rw) 1 --> 10xPLL
//     Bit 4: (rw) 0 --> SCLK is divided by 1, 1 --> SCLK is divided by 2
//     Bits 5-6: Clock Output Divisor
  if (errorCode == 0) {
    uint8_t pll = 0 ; // No PLL
    uint8_t osc = 0 ; // Divide by 1
    switch (inSettings.oscillator ()) {
    case ACAN2517FDSettings::OSC_4MHz:
    case ACAN2517FDSettings::OSC_20MHz:
    case ACAN2517FDSettings::OSC_40MHz:
      break ;
    case ACAN2517FDSettings::OSC_4MHz_DIVIDED_BY_2:
    case ACAN2517FDSettings::OSC_20MHz_DIVIDED_BY_2:
    case ACAN2517FDSettings::OSC_40MHz_DIVIDED_BY_2:
      osc = 1 << 4 ; // Divide by 2
      break ;
    case ACAN2517FDSettings::OSC_4MHz10xPLL_DIVIDED_BY_2 :
      pll = 1 ; // Enable 10x PLL
      osc = 1 << 4 ; // Divide by 2
      break ;
    case ACAN2517FDSettings::OSC_4MHz10xPLL :
      pll = 1 ; // Enable 10x PLL
      break ;
    }
    osc |= pll ;
    if (inSettings.mCLKOPin != ACAN2517FDSettings::SOF) {
      osc |= ((uint8_t) inSettings.mCLKOPin) << 5 ;
    }
    writeRegister8 (OSC_REGISTER, osc) ; // DS20005688B, page 16
  //--- Wait for PLL is ready (wait max 2 ms)
    if (pll != 0) {
      bool wait = true ;
      const uint32_t startTime = millis () ;
      while (wait) {
        wait = (readRegister8 (OSC_REGISTER + 1) & 0x1) == 0 ;  // DS20005688B, page 16
        if (wait && ((millis () - startTime) > 2)) {
          errorCode = kX10PLLNotReadyWithin1MS ;
          wait = false ;
        }
      }
    }
  }
//----------------------------------- Set full speed clock
  mSPISettings = SPISettings ((inSettings.sysClock () * 2) / 5, MSBFIRST, SPI_MODE0) ;
//----------------------------------- Checking SPI connection is on (with a full speed clock)
//    We write and read back 2517 RAM at address 0x400
  for (uint32_t i=1 ; (i != 0) && (errorCode == 0) ; i <<= 1) {
    writeRegister32 (0x400, i) ;
    const uint32_t readBackValue = readRegister32 (0x400) ;
    if (readBackValue != i) {
      errorCode = kReadBackErrorWithFullSpeedSPIClock ;
    }
  }
//----------------------------------- Install interrupt, configure external interrupt
  if (errorCode == 0) {
  //----------------------------------- Configure transmit and receive buffers
    mDriverTransmitBuffer.initWithSize (inSettings.mDriverTransmitFIFOSize) ;
    mDriverReceiveBuffer.initWithSize (inSettings.mDriverReceiveFIFOSize) ;
  //----------------------------------- Reset RAM
    for (uint16_t address = 0x400 ; address < 0xC00 ; address += 4) {
      writeRegister32 (address, 0) ;
    }
  //----------------------------------- Configure CLKO pin
    uint8_t data8 = 0x03 ; // Respect PM1-PM0 default values
    if (inSettings.mCLKOPin == ACAN2517FDSettings::SOF) {
      data8 |= 1 << 5 ; // SOF
    }
    if (inSettings.mTXCANIsOpenDrain) {
      data8 |= 1 << 4 ; // TXCANOD
    }
    if (inSettings.mINTIsOpenDrain) {
      data8 |= 1 << 6 ; // INTOD
    }
    writeRegister8 (IOCON_REGISTER_24_31, data8) ; // DS20005688B, page 24
  //----------------------------------- Configure ISO CRC Enable bit
    data8 = 1 << 6 ; // PXEDIS <-- 1
    if (inSettings.mISOCRCEnabled) {
      data8 |= 1 << 5 ; //  Enable ISO CRC in CAN FD Frames bit
    }
    writeRegister8 (CON_REGISTER, data8) ; // DS20005688B, page 24
  //----------------------------------- Configure DTC (DS20005688B, page 29)
    uint32_t data32 = 1UL << 25 ; // Enable Edge Filtering during Bus Integration state bit (added in 1.1.4)
    if (inSettings.mTDCO != 0) {
      data32 |= 1UL << 17 ; // Auto TDC
      const uint32_t TCDO = uint32_t (inSettings.mTDCO) & 0x7F ;
      data32 |= TCDO << 8 ;
    }
    writeRegister32 (TDC_REGISTER, data32) ;
  //----------------------------------- Configure TXQ
    data8 = inSettings.mControllerTXQBufferRetransmissionAttempts ;
    data8 <<= 5 ;
    data8 |= inSettings.mControllerTXQBufferPriority ;
    writeRegister8 (TXQCON_REGISTER + 2, data8) ; // DS20005688B, page 48
  // Bit 5-7: Payload Size bits
  // Bit 4-0: TXQ size
    mUsesTXQ = inSettings.mControllerTXQSize > 0 ;
    data8 = inSettings.mControllerTXQSize - 1 ;
    data8 |= inSettings.mControllerTXQBufferPayload << 5 ; // Payload
    writeRegister8 (TXQCON_REGISTER + 3, data8) ; // DS20005688B, page 48
    mTXQBufferPayload = ACAN2517FDSettings::objectSizeForPayload (inSettings.mControllerTXQBufferPayload) ;
  //----------------------------------- Configure TXQ and TEF
  // Bit 4: Enable Transmit Queue bit ---> 1: Enable TXQ and reserves space in RAM
  // Bit 3: Store in Transmit Event FIFO bit ---> 0: Don’t save transmitted messages in TEF
  // Bit 0: RTXAT ---> 1: Enable CiFIFOCONm.TXAT to control retransmission attempts
    data8 = 0x01 ; // Enable RTXAT to limit retransmissions (Flole)
    data8 |= mUsesTXQ ? (1 << 4) : 0x00 ; // Bug fix in 1.1.4 (thanks to danielhenz)
    writeRegister8 (CON_REGISTER + 2, data8) ; // DS20005688B, page 24
  //----------------------------------- Configure RX FIFO (FIFOCON, DS20005688B, page 52)
    data8 = inSettings.mControllerReceiveFIFOSize - 1 ; // Set receive FIFO size
    data8 |= inSettings.mControllerReceiveFIFOPayload << 5 ; // Payload
    writeRegister8 (FIFOCON_REGISTER (RECEIVE_FIFO_INDEX) + 3, data8) ;
    data8  = 1 << 0 ; // Interrupt Enabled for FIFO not Empty (TFNRFNIE)
    data8 |= 1 << 3 ; // Interrupt Enabled for FIFO Overflow (RXOVIE)
    writeRegister8 (FIFOCON_REGISTER (RECEIVE_FIFO_INDEX), data8) ;
    mReceiveFIFOPayload = ACAN2517FDSettings::objectSizeForPayload (inSettings.mControllerReceiveFIFOPayload) ;
  //----------------------------------- Configure TX FIFO (FIFOCON, DS20005688B, page 52)
    data8 = inSettings.mControllerTransmitFIFORetransmissionAttempts ;
    data8 <<= 5 ;
    data8 |= inSettings.mControllerTransmitFIFOPriority ;
    writeRegister8 (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX) + 2, data8) ;
    data8 = inSettings.mControllerTransmitFIFOSize - 1 ; // Set transmit FIFO size
    data8 |= inSettings.mControllerTransmitFIFOPayload << 5 ; // Payload
    writeRegister8 (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX) + 3, data8) ;
    data8 = 1 << 7 ; // FIFO is a Tx FIFO
    data8 |= 1 << 4 ; // TXATIE ---> 1: Enable Transmit Attempts Exhausted Interrupt
    writeRegister8 (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX), data8) ;
    mTransmitFIFOPayload = ACAN2517FDSettings::objectSizeForPayload (inSettings.mControllerTransmitFIFOPayload) ;
  //----------------------------------- Configure receive filters
    uint8_t filterIndex = 0 ;
    ACAN2517FDFilters::Filter * filter = inFilters.mFirstFilter ;
    mCallBackFunctionArray = new ACANFDCallBackRoutine [inFilters.filterCount ()] ;
    while (NULL != filter) {
      mCallBackFunctionArray [filterIndex] = filter->mCallBackRoutine ;
      writeRegister32 (MASK_REGISTER (filterIndex), filter->mFilterMask) ; // DS20005688B, page 61
      writeRegister32 (FLTOBJ_REGISTER (filterIndex), filter->mAcceptanceFilter) ; // DS20005688B, page 60
      data8 = 1 << 7 ; // Filter is enabled
      data8 |= 1 ; // Message matching filter is stored in FIFO1
      writeRegister8 (FLTCON_REGISTER (filterIndex), data8) ; // DS20005688B, page 58
      filter = filter->mNextFilter ;
      filterIndex += 1 ;
    }
  //----------------------------------- Activate interrupts (INT, DS20005688B page 34)
    data8  = (1 << 1) ; // Receive FIFO Interrupt Enable
    data8 |= (1 << 0) ; // Transmit FIFO Interrupt Enable
    writeRegister8 (INT_REGISTER + 2, data8) ;
    data8  = (1 << 2) ; // TXATIE ---> 1: Transmit Attempt Interrupt Enable bit
    writeRegister8 (INT_REGISTER + 3, data8) ;
  //----------------------------------- Program nominal bit rate (NBTCFG register)
  //  bits 31-24: BRP - 1
  //  bits 23-16: TSEG1 - 1
  //  bit 15: unused
  //  bits 14-8: TSEG2 - 1
  //  bit 7: unused
  //  bits 6-0: SJW - 1
    uint32_t data = inSettings.mBitRatePrescaler - 1 ;
    data <<= 8 ;
    data |= inSettings.mArbitrationPhaseSegment1 - 1 ;
    data <<= 8 ;
    data |= inSettings.mArbitrationPhaseSegment2 - 1 ;
    data <<= 8 ;
    data |= inSettings.mArbitrationSJW - 1 ;
    writeRegister32 (NBTCFG_REGISTER, data);
  //----------------------------------- Program data bit rate (DBTCFG register)
  //  bits 31-24: BRP - 1
  //  bits 23-21: unused
  //  bits 20-16: TSEG1 - 1
  //  bits 15-12: unused
  //  bits 11-8: TSEG2 - 1
  //  bits 7-4: unused
  //  bits 3-0: SJW - 1
    mHasDataBitRate = inSettings.mDataBitRateFactor != ::DataBitRateFactor::x1 ;
    if (mHasDataBitRate) {
      data = inSettings.mBitRatePrescaler - 1 ;
      data <<= 8 ;
      data |= inSettings.mDataPhaseSegment1 - 1 ;
      data <<= 8 ;
      data |= inSettings.mDataPhaseSegment2 - 1 ;
      data <<= 8 ;
      data |= inSettings.mDataSJW - 1 ;
      writeRegister32 (DBTCFG_REGISTER, data) ;
    }
  //----------------------------------- Request mode (CON_REGISTER + 3, DS20005688B, page 24)
  //  bits 7-4: Transmit Bandwith Sharing Bits ---> 0
  //  bit 3: Abort All Pending Transmissions bit --> 0
    mTXBWS_RequestedMode = inSettings.mRequestedMode | (TXBWS << 4) ;
    writeRegister8 (CON_REGISTER + 3, mTXBWS_RequestedMode);
  //----------------------------------- Wait (10 ms max) until requested mode is reached
    bool wait = true ;
    const uint32_t startTime = millis () ;
    while (wait) {
     const uint8_t actualMode = (readRegister8 (CON_REGISTER + 2) >> 5) & 0x07 ;
      wait = actualMode != inSettings.mRequestedMode ;
      if (wait && ((millis () - startTime) > 10)) {
        errorCode |= kRequestedModeTimeOut ;
        wait = false ;
      }
    }
    #ifdef ARDUINO_ARCH_ESP32
      xTaskCreate (myESP32Task, "ACAN2517Handler", 1024, this, TASK_ACAN2517FD_PRIORITY, &mESP32TaskHandle) ;
    #endif
    if (mINT != 255) { // 255 means interrupt is not used
      #ifdef ARDUINO_ARCH_ESP32
        attachInterrupt (itPin, inInterruptServiceRoutine, FALLING) ;
      #else
        attachInterrupt (itPin, inInterruptServiceRoutine, LOW) ; // Thank to Flole998
        mSPI.usingInterrupt (itPin) ; // usingInterrupt is not implemented in Arduino ESP32
      #endif
    }
  // If you begin() multiple times without constructor,
  // mHardwareTxFIFOFull = true will block the transmitter.
    mHardwareTxFIFOFull = false ;
    mHardwareReceiveBufferOverflowCount = 0 ;
  }
//---
  return errorCode ;
}

//······················································································································
//   end method (resets the MCP2517FD, deallocate buffers, and detach interrupt pin)
//······················································································································

bool ACAN2517FD::end (void) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
  //--- Detach interrupt pin
    if (mINT != 255) { // 255 means interrupt is not used
      const int8_t itPin = digitalPinToInterrupt (mINT) ;
      detachInterrupt (itPin) ; // Available for ESP32 and Arduino
    }
  //--- Request configuration mode
    bool wait = true ;
    bool ok = false ;
    const uint32_t startTime = millis () ;
    while (wait) {
      writeRegister8Assume_SPI_transaction (CON_REGISTER + 3, 0x04 | (1 << 3)) ; // Request configuration mode, abort all transmissions
      const uint8_t actualMode = (readRegister8Assume_SPI_transaction (CON_REGISTER + 2) >> 5) & 0x07 ;
      ok = actualMode == 0x04 ;
      wait = !ok ;
      if (wait && ((millis () - startTime) > 2)) { // Wait (2 ms max) until the configuration mode is reached
        wait = false ;
      }
    }
  //--- Reset MCP2517FD
    assertCS () ;
      mSPI.transfer16 (0x00) ; // Reset instruction: 0x0000
    deassertCS () ;
  //--- ESP32: delete associated task
    #ifdef ARDUINO_ARCH_ESP32
      if (mESP32TaskHandle != nullptr) {
        vTaskDelete (mESP32TaskHandle) ;
        mESP32TaskHandle = nullptr ;
      }
    #endif
  //--- Deallocate buffers
    delete [] mCallBackFunctionArray ; mCallBackFunctionArray = nullptr ;
    mDriverReceiveBuffer.initWithSize (0) ;
    mDriverTransmitBuffer.initWithSize (0) ;
  //---
  turnOnInterrupts () ;
  mSPI.endTransaction () ;
//---
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------
//    SEND FRAME
//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::tryToSend (const CANFDMessage & inMessage) {
  bool ok = inMessage.isValid () ;
  if (ok) {
    mSPI.beginTransaction (mSPISettings) ;
        turnOffInterrupts () ;
        if (inMessage.idx == 0) {
          ok = inMessage.len <= mTransmitFIFOPayload ;
          if (ok) {
            ok = enterInTransmitBuffer (inMessage) ;
          }
        }else if (inMessage.idx == 255) {
          ok = inMessage.len <= mTXQBufferPayload ;
          if (ok) {
            ok = sendViaTXQ (inMessage) ;
          }
        }
    turnOnInterrupts();
    mSPI.endTransaction () ;
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::enterInTransmitBuffer (const CANFDMessage & inMessage) {
  bool result ;
  if (mHardwareTxFIFOFull) {
    result = mDriverTransmitBuffer.append (inMessage) ;
  }else{
    result = true ;
    appendInControllerTxFIFO (inMessage) ;
  //--- If controller FIFO is full, enable "FIFO not full" interrupt
    const uint8_t status = readRegister8Assume_SPI_transaction (FIFOSTA_REGISTER (TRANSMIT_FIFO_INDEX)) ;
    if ((status & 1) == 0) { // FIFO is full
      uint8_t data8 = 1 << 7 ;  // FIFO is a transmit FIFO
      data8 |= 1 ; // Enable "FIFO not full" interrupt
      data8 |= 1 << 4 ; // TXATIE ---> 1: Enable Transmit Attempts Exhausted Interrupt
      writeRegister8Assume_SPI_transaction (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX), data8) ;
      mHardwareTxFIFOFull = true ;
    }
  }
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t lengthCodeForLength (const uint8_t inLength) {
  uint32_t result = inLength & 0x0F ;
  switch (inLength) {
    case 12 : result =  9 ; break ;
    case 16 : result = 10 ; break ;
    case 20 : result = 11 ; break ;
    case 24 : result = 12 ; break ;
    case 32 : result = 13 ; break ;
    case 48 : result = 14 ; break ;
    case 64 : result = 15 ; break ;
  }
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::appendInControllerTxFIFO (const CANFDMessage & inMessage) {
  const uint16_t ramAddr = uint16_t (0x400 + readRegister32Assume_SPI_transaction (FIFOUA_REGISTER (TRANSMIT_FIFO_INDEX))) ;
//--- Write identifier: if an extended frame is sent, identifier bits sould be reordered (see DS20005678B, page 27)
  uint32_t idf = inMessage.id ;
  if (inMessage.ext) {
    idf = ((inMessage.id >> 18) & 0x7FF) | ((inMessage.id & 0x3FFFF) << 11) ;
  }
//--- Write DLC field, FDF, BRS, RTR, IDE bits
  uint32_t flags = lengthCodeForLength (inMessage.len) ;
  if (inMessage.ext) {
    flags |= 1 << 4 ; // Set EXT bit
  }
  switch (inMessage.type) {
  case CANFDMessage::CAN_REMOTE :
    flags |= 1 << 5 ; // Set RTR bit
    break ;
  case CANFDMessage::CAN_DATA :
   break ;
  case CANFDMessage::CANFD_NO_BIT_RATE_SWITCH :
    flags |= 1 << 7 ; // Set FDF bit
    break ;
  case CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH :
    flags |= 1 << 7 ; // Set FDF bit
    if (mHasDataBitRate) {
      flags |= 1 << 6 ; // Set BRS bit
    }
    break ;
  }
//--- Word count
  const uint32_t wordCount = (inMessage.len + 3) / 4 ;
//--- Write word register via 6-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [74] = {0} ;
//--- Enter command
  const uint16_t writeCommand = (ramAddr & 0x0FFF) | (0b0010 << 12) ;
  buffer [0] = writeCommand >> 8 ;
  buffer [1] = writeCommand & 0xFF ;
//--- Enter values
  enterU32InBufferAtIndex (idf, buffer, 2) ;
  enterU32InBufferAtIndex (flags, buffer, 6) ;
  for (uint32_t i=0 ; i < wordCount ; i++) {
    enterU32InBufferAtIndex (inMessage.data32 [i], buffer, 10 + 4 * i) ;
  }
//--- SPI transfer
  assertCS () ;
    mSPI.transfer (buffer, 10 + 4 * wordCount) ;
  deassertCS () ;
//--- Increment FIFO, send message (see DS20005688B, page 48)
  const uint8_t data8 = (1 << 0) | (1 << 1) ; // Set UINC bit, TXREQ bit
  writeRegister8Assume_SPI_transaction (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX) + 1, data8);
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::sendViaTXQ (const CANFDMessage & inMessage) {
  bool ok = mUsesTXQ ;
  if (ok) {
    uint8_t sta = readRegister8Assume_SPI_transaction (TXQSTA_REGISTER) ;
  //--- Check Transmit Attempts Exhausted Interrupt Pending bit
    ok = (sta & (1 << 4)) != 0 ;
    if (ok) {
      writeRegister8Assume_SPI_transaction (TXQSTA_REGISTER, ~ (1 << 4)) ;
    }else{
    //--- Enter message only if TXQ FIFO is not full (see DS20005688B, page 50)
      ok = (sta & 1) != 0 ;
    }
    if (ok) {
      const uint16_t ramAddress = (uint16_t) (0x400 + readRegister32Assume_SPI_transaction (TXQUA_REGISTER)) ;
    //--- Write identifier: if an extended frame is sent, identifier bits sould be reordered (see DS20005678B, page 27)
      uint32_t idf = inMessage.id ;
      if (inMessage.ext) {
        idf = ((inMessage.id >> 18) & 0x7FF) | ((inMessage.id & 0x3FFFF) << 11) ;
      }
    //--- Write DLC field, FDF, BRS, RTR, IDE bits
      uint32_t flags = lengthCodeForLength (inMessage.len) ;
      if (inMessage.ext) {
        flags |= 1 << 4 ; // Set EXT bit
      }
      switch (inMessage.type) {
      case CANFDMessage::CAN_REMOTE :
        flags |= 1 << 5 ; // Set RTR bit
        break ;
      case CANFDMessage::CAN_DATA :
       break ;
      case CANFDMessage::CANFD_NO_BIT_RATE_SWITCH :
        flags |= 1 << 7 ; // Set FDF bit
        break ;
      case CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH :
        flags |= 1 << 7 ; // Set FDF bit
        if (mHasDataBitRate) {
          flags |= 1 << 6 ; // Set BRS bit
        }
        break ;
      }
    //--- Word count
      const uint32_t wordCount = (inMessage.len + 3) / 4 ;
    //--- Transfer frame to the MCP2517FD
      uint8_t buffer [74] = {0} ;
    //--- Enter command
      const uint16_t writeCommand = (ramAddress & 0x0FFF) | (0b0010 << 12) ;
      buffer [0] = writeCommand >> 8 ;
      buffer [1] = writeCommand & 0xFF ;
    //--- Enter values
      enterU32InBufferAtIndex (idf, buffer, 2) ;
      enterU32InBufferAtIndex (flags, buffer, 6) ;
      for (uint32_t i=0 ; i < wordCount ; i++) {
        enterU32InBufferAtIndex (inMessage.data32 [i], buffer, 10 + 4 * i) ;
      }
    //--- SPI transfer
      assertCS () ;
        mSPI.transfer (buffer, 10 + 4 * wordCount) ;
      deassertCS () ;
    //--- Increment FIFO, send message (see DS20005688B, page 48)
      const uint8_t data8 = (1 << 0) | (1 << 1) ; // Set UINC bit, TXREQ bit
      writeRegister8Assume_SPI_transaction (TXQCON_REGISTER + 1, data8);
    }
  }
  return ok ;
}

//----------------------------------------------------------------------------------------------------------------------
//    RECEIVE FRAME
//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::available (void) {
  mSPI.beginTransaction (mSPISettings) ;
      turnOffInterrupts () ;
      const bool hasReceivedMessage = mDriverReceiveBuffer.count () > 0 ;
      turnOnInterrupts();
  mSPI.endTransaction () ;
  return hasReceivedMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::receive (CANFDMessage & outMessage) {
  mSPI.beginTransaction (mSPISettings) ;
      turnOffInterrupts () ;
      const bool hasReceivedMessage = mDriverReceiveBuffer.remove (outMessage) ;
    //--- If receive interrupt is disabled, enable it (added in release 2.17)
      if (mINT == 255) { // No interrupt pin
        mRxInterruptEnabled = true ;
        isr_poll_core () ; // Perform polling
      }else if (!mRxInterruptEnabled) {
        mRxInterruptEnabled = true ;
        uint8_t data8 = readRegister8Assume_SPI_transaction (INT_REGISTER + 2) ;
        data8 |= (1 << 1) ; // Receive FIFO Interrupt Enable
        writeRegister8Assume_SPI_transaction (INT_REGISTER + 2, data8) ;
      }
  turnOnInterrupts();
  mSPI.endTransaction () ;
  return hasReceivedMessage ;
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::dispatchReceivedMessage (const tFilterMatchCallBack inFilterMatchCallBack) {
  CANFDMessage receivedMessage ;
  const bool hasReceived = receive (receivedMessage) ;
  if (hasReceived) {
    const uint32_t filterIndex = receivedMessage.idx ;
    if (NULL != inFilterMatchCallBack) {
      inFilterMatchCallBack (filterIndex) ;
    }
    ACANFDCallBackRoutine callBackFunction = (mCallBackFunctionArray == NULL) ? NULL : mCallBackFunctionArray [filterIndex] ;
    if (NULL != callBackFunction) {
      callBackFunction (receivedMessage) ;
    }
  }
  return hasReceived ;
}

//----------------------------------------------------------------------------------------------------------------------
//    POLLING (ESP32)
//----------------------------------------------------------------------------------------------------------------------

#ifdef ARDUINO_ARCH_ESP32
  void ACAN2517FD::poll (void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE ;
    xSemaphoreGiveFromISR (mISRSemaphore, &xHigherPriorityTaskWoken) ;
    portYIELD_FROM_ISR () ;
  }
#endif

//----------------------------------------------------------------------------------------------------------------------
//    POLLING (other than ESP32)
//----------------------------------------------------------------------------------------------------------------------

#ifndef ARDUINO_ARCH_ESP32
  void ACAN2517FD::poll (void) {
    turnOffInterrupts () ;
      isr_poll_core () ;
    turnOnInterrupts () ;
  }
#endif

//----------------------------------------------------------------------------------------------------------------------
//   INTERRUPT SERVICE ROUTINE (ESP32)
// https://stackoverflow.com/questions/51750377/how-to-disable-interrupt-watchdog-in-esp32-or-increase-isr-time-limit
//----------------------------------------------------------------------------------------------------------------------

#ifdef ARDUINO_ARCH_ESP32
  void ACAN2517FD::isr (void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE ;
    xSemaphoreGiveFromISR (mISRSemaphore, &xHigherPriorityTaskWoken) ;
    portYIELD_FROM_ISR () ;
  }
#endif

//----------------------------------------------------------------------------------------------------------------------
//   INTERRUPT SERVICE ROUTINE (other than ESP32)
//----------------------------------------------------------------------------------------------------------------------

#ifndef ARDUINO_ARCH_ESP32
  void ACAN2517FD::isr (void) {
    isr_poll_core () ;
  }
#endif

//----------------------------------------------------------------------------------------------------------------------
//   INTERRUPT SERVICE ROUTINES (common)
//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::isr_poll_core (void) {
  mSPI.beginTransaction (mSPISettings) ;
    #ifdef ARDUINO_ARCH_ESP32
      taskDISABLE_INTERRUPTS () ;
    #endif
      bool handled = true ;
        while (handled) {
        handled = false ;
        const uint16_t it = readRegister16Assume_SPI_transaction (INT_REGISTER) ; // DS20005688B, page 34
        if (mRxInterruptEnabled && ((it & (1 << 1)) != 0)) { // Receive FIFO interrupt
          receiveInterrupt () ;
          handled = true ;
        }
        if ((it & (1 << 10)) != 0) { // Transmit Attempt interrupt
        //--- Clear Pending Transmit Attempt interrupt bit
          writeRegister8Assume_SPI_transaction (FIFOSTA_REGISTER (TRANSMIT_FIFO_INDEX), ~ (1 << 4)) ;
          transmitInterrupt () ;
          handled = true ;
        }else if ((it & (1 << 0)) != 0) { // Transmit FIFO interrupt
          transmitInterrupt () ;
          handled = true ;
        }
        if ((it & (1 << 2)) != 0) { // TBCIF interrupt
          writeRegister8Assume_SPI_transaction (INT_REGISTER, ~ (1 << 2)) ;
          handled = true ;
        }
        if ((it & (1 << 3)) != 0) { // MODIF interrupt
          writeRegister8Assume_SPI_transaction (INT_REGISTER, ~ (1 << 3)) ;
          handled = true ;
        }
        if ((it & (1 << 12)) != 0) { // SERRIF interrupt
          writeRegister8Assume_SPI_transaction (INT_REGISTER + 1, ~ (1 << 4)) ;
          handled = true ;
        }
        if ((it & (1 << 11)) != 0) { // RXOVIF interrupt
          handled = true ;
          if (mHardwareReceiveBufferOverflowCount < 255) {
            mHardwareReceiveBufferOverflowCount += 1 ;
          }
          writeRegister8Assume_SPI_transaction (FIFOSTA_REGISTER (RECEIVE_FIFO_INDEX), ~ (1 << 3)) ;
        }
      }
    #ifdef ARDUINO_ARCH_ESP32
      taskENABLE_INTERRUPTS () ;
    #endif
  mSPI.endTransaction () ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::transmitInterrupt (void) { // Generated if hardware transmit FIFO is not full
  CANFDMessage message ;
  const bool hasMessage = mDriverTransmitBuffer.remove (message) ;
  if (hasMessage) {
    appendInControllerTxFIFO (message) ;
  }else{ // No message in transmit FIFO: disable "FIFO not full" interrupt
    uint8_t data8 = 1 << 7 ;  // FIFO is a transmit FIFO
    data8 |= 1 << 4 ; // TXATIE ---> 1: Enable Transmit Attempts Exhausted Interrupt
    writeRegister8Assume_SPI_transaction (FIFOCON_REGISTER (TRANSMIT_FIFO_INDEX), data8) ;
    mHardwareTxFIFOFull = false ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::receiveInterrupt (void) {
  const uint16_t ramAddress = uint16_t (0x400 + readRegister32Assume_SPI_transaction (FIFOUA_REGISTER (RECEIVE_FIFO_INDEX))) ;
  CANFDMessage message ;
//--- Read word register via 6-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [74] = {0} ;
//--- Enter command
  const uint16_t readCommand = (ramAddress & 0x0FFF) | (0b0011 << 12) ;
  buffer [0] = readCommand >> 8 ;
  buffer [1] = readCommand & 0xFF ;
//--- SPI transfer
  assertCS () ;
    mSPI.transfer (buffer, 74) ;
  //--- Read identifier (see DS20005678A, page 42)
    message.id = u32FromBufferAtIndex (buffer, 2) ;
  //--- Read DLC, RTR, IDE bits, and match filter index
    const uint32_t flags = u32FromBufferAtIndex (buffer, 6) ;
    static const uint8_t kLength [16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64} ;
    message.len = kLength [flags & 0x0F] ;
  //--- Write data (Swap data if processor is big endian)
    const uint32_t wordCount = (message.len + 3) / 4 ;
    for (uint32_t i=0 ; i < wordCount ; i++) {
      message.data32 [i] = u32FromBufferAtIndex (buffer, 10 + 4 * i) ;
    }
  deassertCS () ;
//--- Increment FIFO
  const uint8_t data8 = 1 << 0 ; // Set UINC bit (DS20005688B, page 52)
  writeRegister8Assume_SPI_transaction (FIFOCON_REGISTER (RECEIVE_FIFO_INDEX) + 1, data8) ;
  message.idx = uint8_t ((flags >> 11) & 0x1F) ;
//--- Message type (DS20005678B, page 42)
  if ((flags & (1 << 5)) != 0 ) { // RTR bit
    message.type = CANFDMessage::CAN_REMOTE ;
  }else if ((flags & (1 << 7)) == 0) { // FDF bit
    message.type = CANFDMessage::CAN_DATA ;
  }else if ((flags & (1 << 6)) == 0) { // BRS bit
    message.type = CANFDMessage::CANFD_NO_BIT_RATE_SWITCH ;
  }else{
    message.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH ;
  }
//--- If an extended frame is received, identifier bits should be reordered (see DS20005678B, page 42)
  message.ext = (flags & (1 << 4)) != 0 ;
  if (message.ext) {
    const uint32_t tempID = message.id ;
    message.id = ((tempID >> 11) & 0x3FFFF) | ((tempID & 0x7FF) << 18) ;
  }
//--- Append message to driver receive FIFO
  mDriverReceiveBuffer.append (message) ;
//--- If mDriverReceiveBuffer is full, disable receive interrupt (added in release 2.17)
  if (mDriverReceiveBuffer.isFull ()) {
    mRxInterruptEnabled = false ;
    if (mINT != 255) {
      uint8_t data8 = readRegister8Assume_SPI_transaction (INT_REGISTER + 2) ;
      data8 &= ~ (1 << 1) ; // Receive FIFO Interrupt disable
      writeRegister8Assume_SPI_transaction (INT_REGISTER + 2, data8) ;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
//   MCP2517FD REGISTER ACCESS, SECOND LEVEL FUNCTIONS (HANDLE CS, ASSUME WITHIN SPI TRANSACTION)
//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::writeRegister32Assume_SPI_transaction (const uint16_t inRegisterAddress,
                                                        const uint32_t inValue) {
//--- Write word register via 6-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [6] = {0} ;
//--- Enter command
  const uint16_t writeCommand = (inRegisterAddress & 0x0FFF) | (0b0010 << 12) ;
  buffer [0] = writeCommand >> 8 ;
  buffer [1] = writeCommand & 0xFF ;
//--- Enter register value
  enterU32InBufferAtIndex (inValue, buffer, 2) ;
//--- SPI transfer
  assertCS () ;
    mSPI.transfer (buffer, 6) ;
  deassertCS () ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::writeRegister8Assume_SPI_transaction (const uint16_t inRegisterAddress,
                                                       const uint8_t inValue) {
//--- Write byte register via 3-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [3] = {0} ;
  const uint16_t writeCommand = (inRegisterAddress & 0x0FFF) | (0b0010 << 12) ;
  buffer [0] = writeCommand >> 8 ;
  buffer [1] = writeCommand & 0xFF ;
  buffer [2] = inValue ;
  assertCS () ;
    mSPI.transfer (buffer, 3) ;
  deassertCS () ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::readRegister32Assume_SPI_transaction (const uint16_t inRegisterAddress) {
//--- Read word register via 6-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [6] = {0} ;
//--- Enter command
  const uint16_t readCommand = (inRegisterAddress & 0x0FFF) | (0b0011 << 12) ;
  buffer [0] = readCommand >> 8 ;
  buffer [1] = readCommand & 0xFF ;
//--- SPI transfer
  assertCS () ;
    mSPI.transfer (buffer, 6) ;
  deassertCS () ;
//--- Get result
  const uint32_t result = u32FromBufferAtIndex (buffer, 2) ;
//---
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t ACAN2517FD::readRegister16Assume_SPI_transaction (const uint16_t inRegisterAddress) {
//--- Read half-word register via 4-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [4] = {0} ;
//--- Enter command
  const uint16_t readCommand = (inRegisterAddress & 0x0FFF) | (0b0011 << 12) ;
  buffer [0] = readCommand >> 8 ;
  buffer [1] = readCommand & 0xFF ;
//--- SPI transfer
  assertCS () ;
    mSPI.transfer (buffer, 4) ;
  deassertCS () ;
//--- Get result
  const uint16_t result = u16FromBufferAtIndex (buffer, 2) ;
//---
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

uint8_t ACAN2517FD::readRegister8Assume_SPI_transaction (const uint16_t inRegisterAddress) {
//--- Read byte register via 3-byte buffer (speed enhancement, thanks to thomasfla)
  uint8_t buffer [3] = {0} ;
  const uint16_t readCommand = (inRegisterAddress & 0x0FFF) | (0b0011 << 12) ;
  buffer [0] = readCommand >> 8;
  buffer [1] = readCommand & 0xFF;
  assertCS () ;
    mSPI.transfer(buffer, 3) ;
  deassertCS () ;
  return buffer [2] ;
}

//----------------------------------------------------------------------------------------------------------------------
//   MCP2517FD REGISTER ACCESS, THIRD LEVEL FUNCTIONS (HANDLE CS AND SPI TRANSACTION)
//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::writeRegister8 (const uint16_t inRegisterAddress, const uint8_t inValue) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
      writeRegister8Assume_SPI_transaction (inRegisterAddress, inValue) ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
}

//----------------------------------------------------------------------------------------------------------------------

uint8_t ACAN2517FD::readRegister8 (const uint16_t inRegisterAddress) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
      const uint8_t result = readRegister8Assume_SPI_transaction (inRegisterAddress) ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t ACAN2517FD::readRegister16 (const uint16_t inRegisterAddress) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
      const uint16_t result = readRegister16Assume_SPI_transaction (inRegisterAddress) ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
  return result ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::writeRegister32 (const uint16_t inRegisterAddress, const uint32_t inValue) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
      writeRegister32Assume_SPI_transaction (inRegisterAddress, inValue) ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::readRegister32 (const uint16_t inRegisterAddress) {
  mSPI.beginTransaction (mSPISettings) ;
    turnOffInterrupts () ;
      const uint32_t result = readRegister32Assume_SPI_transaction (inRegisterAddress) ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
  return result ;
}

//······················································································································
//    Current MCP2517FD Operation Mode
//······················································································································

ACAN2517FDSettings::OperationMode ACAN2517FD::currentOperationMode (void) {
  const uint8_t mode = readRegister8 (CON_REGISTER + 2) >> 5 ;
  return ACAN2517FDSettings::OperationMode (mode) ;
}

//······················································································································

bool ACAN2517FD::recoverFromRestrictedOperationMode (void) {
   bool recoveryDone = false ;
   if (currentOperationMode () == ACAN2517FDSettings::RestrictedOperation) { // In Restricted Operation Mode
  //----------------------------------- Request mode (CON_REGISTER + 3)
  //  bits 7-4: Transmit Bandwith Sharing Bits ---> 0
  //  bit 3: Abort All Pending Transmission bits --> 0
    writeRegister8 (CON_REGISTER + 3, mTXBWS_RequestedMode);
  //----------------------------------- Wait (10 ms max) until requested mode is reached
    bool wait = true ;
    const uint32_t startTime = millis () ;
    while (wait) {
      const uint8_t actualMode = (readRegister8 (CON_REGISTER + 2) >> 5) & 0x07 ;
      wait = actualMode != (mTXBWS_RequestedMode & 0x07) ;
      recoveryDone = !wait ;
      if (wait && ((millis () - startTime) > 10)) {
        wait = false ;
      }
    }
  }
  return recoveryDone ;
}

//······················································································································
//    Set MCP2517FD Operation Mode
//······················································································································

void ACAN2517FD::setOperationMode (const ACAN2517FDSettings::OperationMode inOperationMode) {
//  bits 7-4: Transmit Bandwith Sharing Bits ---> 0
//  bit 3: Abort All Pending Transmission bits --> 0
  writeRegister8 (CON_REGISTER + 3, uint8_t (inOperationMode));
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::reset2517FD (void) {
  mSPI.beginTransaction (mSPISettings) ; // Check RESET is performed with 800 kHz clock
    turnOffInterrupts () ;
      assertCS () ;
        mSPI.transfer16 (0x00) ; // Reset instruction: 0x0000
      deassertCS () ;
    turnOnInterrupts () ;
  mSPI.endTransaction () ;
}

//······················································································································
//    Sleep Mode to Configuration Mode
// (returns true if MCP2517FD was in sleep mode)
//······················································································································
// The device exits Sleep mode due to a dominant edge on RXCAN or by enabling the oscillator (clearing OSC.OSCDIS).
// The module will transition automatically to Configuration mode.

bool ACAN2517FD::performSleepModeToConfigurationMode (void) {
  uint8_t value = readRegister8 (OSC_REGISTER) ;
  const bool inSleepMode = (value & (1 << 2)) != 0 ;
  if (inSleepMode) {
    value &= ~ (1 << 2) ; // Reset OSCDIS bit
    writeRegister8 (OSC_REGISTER, value) ;
  //--- Wait Clock is ready, ie OSC.OSCRDY is 1
    bool wait = true ;
    while (wait) {
      wait = (readRegister8 (OSC_REGISTER + 1) & (1 << 2)) == 0 ;
    }
  }
  return inSleepMode ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::errorCounters (void) {
  return readRegister32 (TREC_REGISTER) ;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t ACAN2517FD::diagInfos (const int inIndex) { // thanks to Flole998 and turmary
  return readRegister32 (inIndex ? BDIAG1_REGISTER: BDIAG0_REGISTER) ;
}

//······················································································································
//    GPIO
//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::gpioSetMode (const uint8_t inPin, const uint8_t inMode) {
  if (inPin <= 1) {
    uint8_t value = readRegister8 (IOCON_REGISTER_00_07) ;
    if (inMode == INPUT) {
      value |=  (1 << inPin) ;
      if (inPin == 0) {
        value &= 3 ; // Clear XSBTYEN
      }
    }else if (inMode == OUTPUT) {
      value &= ~ (1 << inPin) ;
      if (inPin == 0) {
        value &= 3 ; // Clear XSBTYEN
      }
    }
    writeRegister8 (IOCON_REGISTER_00_07, value) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::gpioWrite (const uint8_t inPin, const uint8_t inLevel) {
  if (inPin <= 1) {
    uint8_t value = readRegister8 (IOCON_REGISTER_08_15) ;
    if (inLevel == 0) { // LOW
      value &= ~ (1 << inPin) ;
    }else{
      value |=   (1 << inPin) ;
    }
    writeRegister8 (IOCON_REGISTER_08_15, value) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool ACAN2517FD::gpioRead (const uint8_t inPin) {
  const uint8_t value = readRegister8 (IOCON_REGISTER_16_23) ;
  return (value >> inPin) & 1 ;
}

//----------------------------------------------------------------------------------------------------------------------

void ACAN2517FD::configureGPIO0AsXSTBY (void) {
  uint8_t value = readRegister8 (IOCON_REGISTER_00_07) ;
  value |= (1 << 6) ; // Enable XSBTYEN
  writeRegister8 (IOCON_REGISTER_00_07, value) ;
}

//----------------------------------------------------------------------------------------------------------------------
