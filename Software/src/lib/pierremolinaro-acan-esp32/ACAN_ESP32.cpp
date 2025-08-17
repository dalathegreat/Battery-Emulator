//------------------------------------------------------------------------------
//   Include files
//------------------------------------------------------------------------------

#include "ACAN_ESP32.h"

//------------------------------------------------------------------------------

//--- Header for periph_module_enable
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #include <esp_private/periph_ctrl.h>
#else
  #include <driver/periph_ctrl.h>
#endif

#include <hal/clk_gate_ll.h> // For ESP32 board manager

//------------------------------------------------------------------------------
//   ESP32 Critical Section
//------------------------------------------------------------------------------

// taskENTER_CRITICAL() of FREE-RTOS is deprecated as portENTER_CRITICAL() in ESP32
//--- https://esp32.com/viewtopic.php?t=1703
static portMUX_TYPE portMux = portMUX_INITIALIZER_UNLOCKED ;

//------------------------------------------------------------------------------
//   CONSTRUCTOR for ESP32C6 (2 TWAI controllers)
//------------------------------------------------------------------------------

#ifdef CONFIG_IDF_TARGET_ESP32C6
  ACAN_ESP32::ACAN_ESP32 (const uint32_t inBaseAddress,
                          const uint32_t inTxPinIndexSelector,
                          const uint32_t inRxPinIndexSelector,
                          const periph_module_t inPeriphModule,
                          const periph_interrupt_t inInterruptSource,
                          const uint32_t inClockEnableAddress) :
  twaiBaseAddress (inBaseAddress),
  twaiTxPinSelector (inTxPinIndexSelector),
  twaiRxPinSelector (inRxPinIndexSelector),
  twaiPeriphModule (inPeriphModule),
  twaiInterruptSource (inInterruptSource),
  twaiClockEnableAddress (inClockEnableAddress),
  mAcceptedFrameFormat (ACAN_ESP32_Filter::standardAndExtended),
  mDriverReceiveBuffer (),
  mDriverTransmitBuffer (),
  mDriverIsSending (false) {
}
#endif

//------------------------------------------------------------------------------
//   CONSTRUCTOR for others (1 TWAI controller)
//------------------------------------------------------------------------------

#ifndef CONFIG_IDF_TARGET_ESP32C6
  ACAN_ESP32::ACAN_ESP32 (void) :
  mAcceptedFrameFormat (ACAN_ESP32_Filter::standardAndExtended),
  mDriverReceiveBuffer (),
  mDriverTransmitBuffer (),
  mDriverIsSending (false) {
}
#endif

//------------------------------------------------------------------------------
//   Set the GPIO pins
//------------------------------------------------------------------------------

void ACAN_ESP32::setGPIOPins (const gpio_num_t inTXPin,
                              const gpio_num_t inRXPin) {
//--- Set TX pin
  pinMode (inTXPin, OUTPUT) ;
  pinMatrixOutAttach (inTXPin, twaiTxPinSelector, false, false) ;
//--- Set RX pin
  pinMode (inRXPin, INPUT) ;
  pinMatrixInAttach (inRXPin, twaiRxPinSelector, false) ;
}

//------------------------------------------------------------------------------
//   Set the Requested Mode
//------------------------------------------------------------------------------

void ACAN_ESP32::setRequestedCANMode (const ACAN_ESP32_Settings & inSettings,
                                      const ACAN_ESP32_Filter & inFilter) {
// ESP32 CAN Operation Mode Configuration
//
//    Supported Mode                 MODE Registers
//     - Normal Mode                  - Reset             -> bit(0)
//     - No ACK                       - ListenOnly        -> bit(1)
//     - Acceptance Filter            - SelfTest          -> bit(2)
//                                    - Acceptance Filter -> bit(3)

  uint8_t requestedMode = 0 ;
  switch (inSettings.mRequestedCANMode) {
    case ACAN_ESP32_Settings::NormalMode :
      break ;
    case ACAN_ESP32_Settings::ListenOnlyMode :
      requestedMode = TWAI_LISTEN_ONLY_MODE ;
      break ;
    case ACAN_ESP32_Settings::LoopBackMode :
      requestedMode = TWAI_SELF_TEST_MODE ;
      break ;
  }

  if (inFilter.mAMFSingle) {
    requestedMode |= TWAI_RX_FILTER_MODE ;
  }

  TWAI_MODE_REG () = requestedMode | TWAI_RESET_MODE ;
  uint32_t unusedResult __attribute__((unused)) = TWAI_MODE_REG () ;

  do{
    TWAI_MODE_REG () = requestedMode ;
  }while ((TWAI_MODE_REG () & TWAI_RESET_MODE) != 0) ;
}

//------------------------------------------------------------------------------
//   Set the Bus timing Registers
//------------------------------------------------------------------------------

inline void ACAN_ESP32::setBitTimingSettings (const ACAN_ESP32_Settings & inSettings) {
// BUS TIMING Configuration of ESP32 CAN
//     ACAN_ESP32_Settings calculates the best values for the desired bit Rate.

//--- Caution! TWAI_BUS_TIMING_0_REG is specific
#ifdef CONFIG_IDF_TARGET_ESP32
  //  BTR0 : bit (0 - 5) -> Baud Rate Prescaller (BRP)
  //         bit (6 - 7) -> Resynchronization Jump Width (RJW)
    TWAI_BUS_TIMING_0_REG () =
      ((inSettings.mRJW - 1) << 6) |            // SJW
      ((inSettings.mBitRatePrescaler - 1) << 0) // BRP
    ;
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
  //  BTR0 : bit (00 - 13) -> Baud Rate Prescaller (BRP)
  //         bit (14 - 15) -> Resynchronization Jump Width (RJW)
    TWAI_BUS_TIMING_0_REG () =
      ((inSettings.mRJW - 1) << 14) |           // SJW
      ((inSettings.mBitRatePrescaler - 1) << 0) // BRP
    ;
#elif defined (CONFIG_IDF_TARGET_ESP32S2)
  //  BTR0 : bit (00 - 13) -> Baud Rate Prescaller (BRP)
  //         bit (14 - 15) -> Resynchronization Jump Width (RJW)
    TWAI_BUS_TIMING_0_REG () =
      ((inSettings.mRJW - 1) << 14) |           // SJW
      ((inSettings.mBitRatePrescaler - 1) << 0) // BRP
    ;
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
  //  BTR0 : bit (00 - 13) -> Baud Rate Prescaller (BRP)
  //         bit (14 - 15) -> Resynchronization Jump Width (RJW)
    TWAI_BUS_TIMING_0_REG () =
      ((inSettings.mRJW - 1) << 14) |           // SJW
      ((inSettings.mBitRatePrescaler - 1) << 0) // BRP
    ;
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
  //  BTR0 : bit (00 - 13) -> Baud Rate Prescaller (BRP)
  //         bit (14 - 15) -> Resynchronization Jump Width (RJW)
    TWAI_BUS_TIMING_0_REG () =
      ((inSettings.mRJW - 1) << 14) |           // SJW
      ((inSettings.mBitRatePrescaler - 1) << 0) // BRP
    ;
#else
  #error "Unknown board"
#endif

//--- BTR1: bit (0 - 3) -> TimeSegment 1 (Tseg1)
//          bit (4 - 6) -> TimeSegment 2 (Tseg2)
//          bit (7)     -> TripleSampling? (SAM)
  TWAI_BUS_TIMING_1_REG () =
    ((inSettings.mTripleSampling) << 7)   | // Sampling
    ((inSettings.mTimeSegment2 - 1) << 4) | // Tseg2
    ((inSettings.mTimeSegment1 - 1) << 0)   // Tseg1
  ;
}

//------------------------------------------------------------------------------

void ACAN_ESP32::setAcceptanceFilter (const ACAN_ESP32_Filter & inFilter) {
//--- Write the Code and Mask Registers with Acceptance Filter Settings
  if (inFilter.mAMFSingle) {
    TWAI_MODE_REG () |= TWAI_RX_FILTER_MODE ;
  }
  mAcceptedFrameFormat = inFilter.mFormat ;

  TWAI_ACC_CODE_FILTER (0) = inFilter.mACR0 ;
  TWAI_ACC_CODE_FILTER (1) = inFilter.mACR1 ;
  TWAI_ACC_CODE_FILTER (2) = inFilter.mACR2 ;
  TWAI_ACC_CODE_FILTER (3) = inFilter.mACR3 ;

  TWAI_ACC_MASK_FILTER (0) = inFilter.mAMR0 ;
  TWAI_ACC_MASK_FILTER (1) = inFilter.mAMR1 ;
  TWAI_ACC_MASK_FILTER (2) = inFilter.mAMR2 ;
  TWAI_ACC_MASK_FILTER (3) = inFilter.mAMR3 ;
}

//------------------------------------------------------------------------------
//   BEGIN
//------------------------------------------------------------------------------

uint32_t ACAN_ESP32::begin (const ACAN_ESP32_Settings & inSettings,
                            const ACAN_ESP32_Filter & inFilterSettings) {
//   Serial.println (twaiBaseAddress, HEX) ;
  uint32_t errorCode = 0 ; // Ok by default
//--------------------------------- Enable CAN module clock (only for ESP32C6)
  #ifdef CONFIG_IDF_TARGET_ESP32C6
    #define ACAN_CLOCK_ENABLE_REG (*((volatile uint32_t *) twaiClockEnableAddress))
    ACAN_CLOCK_ENABLE_REG = 1 << 22 ;
  //--- Display enable settings (for twai0)
//     #define ACAN_PCR_TWAI0_CONF_REG (*((volatile uint32_t *) (0x60096000 + 0x05C)))
//     #define ACAN_PCR_TWAI0_FUNC_CLK_CONF_REG (*((volatile uint32_t *) (0x60096000 + 0x060)))
//     Serial.print ("Reset reg (should be 1): 0x") ;
//     Serial.println (ACAN_PCR_TWAI0_CONF_REG, HEX) ;
//     Serial.print ("Clock select reg (should be 0x400000): 0x") ;
//     Serial.println (ACAN_PCR_TWAI0_FUNC_CLK_CONF_REG, HEX) ;
  #endif
//--------------------------------- Enable CAN module
  periph_module_enable (twaiPeriphModule) ;
//--------------------------------- Set GPIO pins
  setGPIOPins (inSettings.mTxPin, inSettings.mRxPin);
//--------------------------------- Required: It is must to enter RESET Mode to write the Configuration Registers
  TWAI_CMD_REG () = TWAI_ABORT_TX ;
  TWAI_MODE_REG () = TWAI_RESET_MODE ;
  while ((TWAI_MODE_REG () & TWAI_RESET_MODE) == 0) {
    TWAI_MODE_REG () = TWAI_RESET_MODE ;
  }
  if ((TWAI_MODE_REG () & TWAI_RESET_MODE) == 0) {
    errorCode = kNotInResetModeInConfiguration ;
  }
//--------------------------------- Disable Interupts
  TWAI_INT_ENA_REG () = 0 ;
  if (mInterruptHandler != nullptr) {
    esp_intr_free (mInterruptHandler) ;
    mInterruptHandler = nullptr ;
  }
//--------------------------------- Use Pelican Mode
  TWAI_CLOCK_DIVIDER_REG () = TWAI_EXT_MODE ;
//---- Check the Register access and bit timing settings before writing to the Bit Timing Registers
  TWAI_BUS_TIMING_0_REG () = 0x55 ;
  bool ok = TWAI_BUS_TIMING_0_REG () == 0x55 ;
  if (ok) {
    TWAI_BUS_TIMING_0_REG () = 0xAA ;
    ok = TWAI_BUS_TIMING_0_REG () == 0xAA ;
  }
  if (!ok) {
    errorCode |= kCANRegistersError ;
  }
//----------------------------------- If ok, check the bit timing settings are correct
  if (!inSettings.mBitRateClosedToDesiredRate) {
    errorCode |= kTooFarFromDesiredBitRate;
  }
  errorCode |= inSettings.CANBitSettingConsistency ();
//----------------------------------- Allocate buffer
  if (!mDriverReceiveBuffer.initWithSize (inSettings.mDriverReceiveBufferSize)) {
    errorCode |= kCannotAllocateDriverReceiveBuffer ;
  }
  if (!mDriverTransmitBuffer.initWithSize (inSettings.mDriverTransmitBufferSize)) {
    errorCode |= kCannotAllocateDriverTransmitBuffer ;
  }
//--------------------------------- Set Bus timing Registers
  if (errorCode == 0) {
    setBitTimingSettings (inSettings) ;
  }
//--------------------------------- Set the Acceptance Filter
  setAcceptanceFilter (inFilterSettings) ;
//--------------------------------- Set and clear the error counters to default value
  TWAI_ERR_WARNING_LIMIT_REG () = 96 ;
  TWAI_RX_ERR_CNT_REG () = 0 ;
//--------------------------------- Clear the Interrupt Registers
  const uint32_t unusedVariable __attribute__((unused)) = TWAI_INT_RAW_REG () ;
//--------------------------------- Set Interrupt Service Routine
  esp_intr_alloc (twaiInterruptSource, 0, isr, this, & mInterruptHandler) ;
//--------------------------------- Enable Interupts
  TWAI_INT_ENA_REG () = TWAI_TX_INT_ENA | TWAI_RX_INT_ENA ;
//--------------------------------- Set to Requested Mode
  setRequestedCANMode (inSettings, inFilterSettings) ;
//---
  return errorCode ;
}

//----------------------------------------------------------------------------------------
//   Stop CAN controller and uninstall ISR
//----------------------------------------------------------------------------------------

void ACAN_ESP32::end (void) {
//--------------------------------- Abort any pending transfer, don't care about resetting
  TWAI_CMD_REG () = TWAI_ABORT_TX ;

//--------------------------------- Disable Interupts
  TWAI_INT_ENA_REG () = 0 ;
  if (mInterruptHandler != nullptr) {
    esp_intr_free (mInterruptHandler) ;
    mInterruptHandler = nullptr ;
  }

//--------------------------------- Disable CAN module
  periph_module_disable (PERIPH_TWAI_MODULE) ;
}

//------------------------------------------------------------------------------
//--- Status Flags (returns 0 if no error)
//  Bit 0 : hardware receive FIFO overflow
//  Bit 1 : driver receive FIFO overflow
//  Bit 2 : bus off
//  Bit 3 : reset mode

uint32_t ACAN_ESP32::statusFlags (void) const {
  uint32_t result = 0 ; // Ok
  const uint32_t status = TWAI_STATUS_REG () ;
//--- Hardware receive FIFO overflow ?
  if ((status & TWAI_OVERRUN_ST) != 0) {
    result |= 1U << 0 ;
  }
//--- Driver receive FIFO overflow ?
  if (mDriverReceiveBuffer.didOverflow ()) {
    result |= 1U << 1 ;
  }
//--- Bus off ?
  if ((status & TWAI_BUS_OFF_ST) != 0) {
    result |= 1U << 2 ;
  }
//--- Reset mode ?
  if ((TWAI_MODE_REG () & TWAI_RESET_MODE) != 0) {
    result |= 1U << 3 ;
  }
//---
  return result ;
}

//------------------------------------------------------------------------------

bool ACAN_ESP32::recoverFromBusOff (void) const {
  const bool isBusOff = (TWAI_STATUS_REG () & TWAI_BUS_OFF_ST) != 0 ;
  const bool inResetMode = (TWAI_MODE_REG () & TWAI_RESET_MODE) != 0 ;
  const bool recover = isBusOff && inResetMode ;
  if (recover) {
    TWAI_MODE_REG () &= ~ TWAI_RESET_MODE ;
  }
  return recover ;
}

//------------------------------------------------------------------------------
//   Interrupt Handler
//------------------------------------------------------------------------------

void IRAM_ATTR ACAN_ESP32::isr (void * inUserArgument) {
  ACAN_ESP32 * myDriver = (ACAN_ESP32 *) inUserArgument ;

  portENTER_CRITICAL (&portMux) ;
  const uint32_t interrupt = myDriver->TWAI_INT_RAW_REG () ;
  if ((interrupt & TWAI_RX_INT_ST) != 0) {
     myDriver->handleRXInterrupt () ;
  }
  if ((interrupt & TWAI_TX_INT_ST) != 0) {
     myDriver->handleTXInterrupt () ;
  }
  portEXIT_CRITICAL (&portMux) ;

  portYIELD_FROM_ISR () ;
}

//------------------------------------------------------------------------------

void ACAN_ESP32::handleTXInterrupt (void) {
  CANMessage message ;
  const bool sendmsg = mDriverTransmitBuffer.remove (message) ;
  if (sendmsg) {
    internalSendMessage (message) ;
  }else {
    mDriverIsSending = false ;
  }
}

//------------------------------------------------------------------------------

void ACAN_ESP32::handleRXInterrupt (void) {
  CANMessage frame;
  getReceivedMessage (frame) ;
  switch (mAcceptedFrameFormat) {
  case ACAN_ESP32_Filter::standard :
    if (!frame.ext) {
      mDriverReceiveBuffer.append (frame) ;
    }
    break ;
  case ACAN_ESP32_Filter::extended :
    if (frame.ext) {
      mDriverReceiveBuffer.append (frame) ;
    }
    break ;
  case ACAN_ESP32_Filter::standardAndExtended :
    mDriverReceiveBuffer.append (frame) ;
    break ;
  }
}

//------------------------------------------------------------------------------
//   RECEPTION
//------------------------------------------------------------------------------

bool ACAN_ESP32::available (void) const {
  const bool hasReceivedMessage = mDriverReceiveBuffer.count () > 0 ;
  return hasReceivedMessage ;
}

//------------------------------------------------------------------------------

bool ACAN_ESP32::receive (CANMessage & outMessage) {
  portENTER_CRITICAL (&portMux) ;
    const bool hasReceivedMessage = mDriverReceiveBuffer.remove (outMessage) ;
  portEXIT_CRITICAL (&portMux) ;
  return hasReceivedMessage ;
}

//------------------------------------------------------------------------------

void ACAN_ESP32::getReceivedMessage (CANMessage & outFrame) {
  const uint32_t frameInfo = TWAI_FRAME_INFO () ;

  outFrame.len = frameInfo & 0xF;
  if (outFrame.len > 8) {
    outFrame.len = 8 ;
  }
  outFrame.rtr = (frameInfo & TWAI_RTR) != 0 ;
  outFrame.ext = (frameInfo & TWAI_FRAME_FORMAT_EFF) != 0 ;

  if (!outFrame.ext) { //--- Standard Frame
    outFrame.id  = uint32_t (TWAI_ID_SFF(0)) << 3 ;
    outFrame.id |= uint32_t (TWAI_ID_SFF(1)) >> 5 ;

    for (uint8_t i=0 ; i<outFrame.len ; i++) {
      outFrame.data[i] = uint8_t (TWAI_DATA_SFF (i)) ;
    }
  }else{ //--- Extended Frame
    outFrame.id  = uint32_t (TWAI_ID_EFF(0)) << 21 ;
    outFrame.id |= uint32_t (TWAI_ID_EFF(1)) << 13 ;
    outFrame.id |= uint32_t (TWAI_ID_EFF(2)) <<  5 ;
    outFrame.id |= uint32_t (TWAI_ID_EFF(3)) >>  3 ;
    for (uint8_t i=0 ; i<outFrame.len ; i++) {
      outFrame.data [i] = uint8_t (TWAI_DATA_EFF (i)) ;
    }
  }

  TWAI_CMD_REG () = TWAI_RELEASE_BUF ;
}

//------------------------------------------------------------------------------
//   TRANSMISSION
//------------------------------------------------------------------------------

bool ACAN_ESP32::tryToSend (const CANMessage & inMessage) {
  bool sendMessage ;
//--- Bug fixed in 1.0.2 (thanks to DirkMeintjies)
  portENTER_CRITICAL (&portMux) ;
    if (mDriverIsSending) {
      sendMessage = mDriverTransmitBuffer.append (inMessage);
    }else{
      internalSendMessage (inMessage) ;
      mDriverIsSending = true ;
      sendMessage = true ;
    }
  portEXIT_CRITICAL (&portMux) ;
  return sendMessage ;
}

//------------------------------------------------------------------------------

void ACAN_ESP32::internalSendMessage (const CANMessage & inFrame) {
//--- DLC
  const uint8_t dlc = (inFrame.len <= 8) ? inFrame.len : 8 ;
//--- RTR
  const uint8_t rtr = inFrame.rtr ? TWAI_RTR : 0 ;
//--- Format
  const uint8_t format = (inFrame.ext) ? TWAI_FRAME_FORMAT_EFF : TWAI_FRAME_FORMAT_SFF ;
//--- Set Frame Information
  TWAI_FRAME_INFO () = format | rtr | dlc ;
//--- Identifier and data
  if (!inFrame.ext) { //--- Standard Frame
  //--- Set ID
    TWAI_ID_SFF(0) = (inFrame.id >> 3) & 255 ;
    TWAI_ID_SFF(1) = (inFrame.id << 5) & 255 ;
  //--- Set data
    for (uint8_t i=0 ; i<dlc ; i++) {
      TWAI_DATA_SFF (i) = inFrame.data [i] ;
    }
  }else{ //--- Extended Frame
  //--- Set ID
    TWAI_ID_EFF(0) = (inFrame.id >> 21) & 255 ;
    TWAI_ID_EFF(1) = (inFrame.id >> 13) & 255 ;
    TWAI_ID_EFF(2) = (inFrame.id >> 5) & 255 ;
    TWAI_ID_EFF(3) = (inFrame.id << 3) & 255 ;
  //--- Set data
    for (uint8_t i=0 ; i<dlc ; i++) {
      TWAI_DATA_EFF (i) = inFrame.data [i] ;
    }
  }
//--- Send command
  TWAI_CMD_REG () = ((TWAI_MODE_REG () & TWAI_SELF_TEST_MODE) != 0)
    ? TWAI_SELF_RX_REQ
    : TWAI_TX_REQ
  ;
}

//------------------------------------------------------------------------------
//    Driver instances for ESP32C6
//------------------------------------------------------------------------------

#ifdef CONFIG_IDF_TARGET_ESP32C6
  ACAN_ESP32 ACAN_ESP32::can  (DR_REG_TWAI0_BASE,
                               TWAI0_TX_IDX,
                               TWAI0_RX_IDX,
                               PERIPH_TWAI0_MODULE,
                               ETS_TWAI0_INTR_SOURCE,
                               PCR_TWAI0_FUNC_CLK_CONF_REG) ;
  ACAN_ESP32 ACAN_ESP32::can1 (DR_REG_TWAI1_BASE,
                               TWAI1_TX_IDX,
                               TWAI1_RX_IDX,
                               PERIPH_TWAI1_MODULE,
                               ETS_TWAI1_INTR_SOURCE,
                               PCR_TWAI1_FUNC_CLK_CONF_REG) ;
#endif

//------------------------------------------------------------------------------
//    Driver instance for others
//------------------------------------------------------------------------------

#ifndef CONFIG_IDF_TARGET_ESP32C6
  ACAN_ESP32 ACAN_ESP32::can ;
#endif

//------------------------------------------------------------------------------
