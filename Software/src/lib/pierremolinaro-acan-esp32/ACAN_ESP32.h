
#pragma once

//------------------------------------------------------------------------------
//   Include files
//------------------------------------------------------------------------------

#include "ACAN_ESP32_TWAI_base_address.h"
#include "ACAN_ESP32_Settings.h"
#include "ACAN_ESP32_CANMessage.h"
#include "ACAN_ESP32_Buffer16.h"
#include "ACAN_ESP32_AcceptanceFilters.h"

//------------------------------------------------------------------------------
//   ESP32 CAN class
//------------------------------------------------------------------------------

class ACAN_ESP32 {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CONSTRUCTOR for ESP32C6 (2 TWAI controllers)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #ifdef CONFIG_IDF_TARGET_ESP32C6
    private: const uint32_t twaiBaseAddress ;
    private: const uint32_t twaiTxPinSelector ;
    private: const uint32_t twaiRxPinSelector ;
    private: const periph_module_t twaiPeriphModule ;
    private: const periph_interrupt_t twaiInterruptSource ;
    private: const uint32_t twaiClockEnableAddress ;
    private: ACAN_ESP32 (const uint32_t inBaseAddress,
                         const uint32_t inTxPinIndexSelector,
                         const uint32_t inRxPinIndexSelector,
                         const periph_module_t inPeriphModule,
                         const periph_interrupt_t inInterruptSource,
                         const uint32_t inClockEnableAddress) ;
  #endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //   CONSTRUCTOR for others (1 TWAI controller)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  #ifndef CONFIG_IDF_TARGET_ESP32C6
    private: ACAN_ESP32 (void) ;
  #endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Initialisation: returns 0 if ok, otherwise see error codes below
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: uint32_t begin (const ACAN_ESP32_Settings & inSettings,
                          const ACAN_ESP32_Filter & inFilterSettings = ACAN_ESP32_Filter::acceptAll ()) ;
 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Deinit: Stop CAN controller and uninstall ISR 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public: void end (void) ;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    CAN  Configuration Private Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: void setGPIOPins (const gpio_num_t inTXPin, const gpio_num_t inRXPin);
  private: void setBitTimingSettings(const ACAN_ESP32_Settings &inSettings) ;
  private: void setRequestedCANMode (const ACAN_ESP32_Settings &inSettings,
                                     const ACAN_ESP32_Filter & inFilter) ;
  private: void setAcceptanceFilter (const ACAN_ESP32_Filter & inFilter) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Receiving messages
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool available (void) const ;
  public: bool receive (CANMessage & outMessage) ;
  public: void getReceivedMessage (CANMessage & outFrame) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Receive buffer
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: ACAN_ESP32_Filter::Format mAcceptedFrameFormat ;

  private: ACAN_ESP32_Buffer16 mDriverReceiveBuffer ;

  public: inline uint16_t driverReceiveBufferSize (void) const { return mDriverReceiveBuffer.size () ;  }
  public: inline uint16_t driverReceiveBufferCount (void) const { return mDriverReceiveBuffer.count() ;  }
  public: inline uint16_t driverReceiveBufferPeakCount (void) const { return mDriverReceiveBuffer.peakCount () ; }

  public: inline void resetDriverReceiveBufferPeakCount (void) { mDriverReceiveBuffer.resetPeakCount () ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Transmitting messages
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool tryToSend (const CANMessage & inMessage) ;
  private: void internalSendMessage (const CANMessage & inFrame) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Transmit buffer
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: ACAN_ESP32_Buffer16 mDriverTransmitBuffer ;
  private: bool mDriverIsSending ;

  public: inline uint16_t driverTransmitBufferSize (void) const { return mDriverTransmitBuffer.size () ; }
  public: inline uint16_t driverTransmitBufferCount (void) const { return mDriverTransmitBuffer.count () ; }
  public: inline uint16_t driverTransmitBufferPeakCount (void) const { return mDriverTransmitBuffer.peakCount () ; }

  public: inline void resetDriverTransmitBufferPeakCount (void) { mDriverTransmitBuffer.resetPeakCount () ; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Error codes returned by begin
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static const uint32_t kNotInResetModeInConfiguration      = 1 << 16 ;
  public: static const uint32_t kCANRegistersError                  = 1 << 17 ;
  public: static const uint32_t kTooFarFromDesiredBitRate           = 1 << 18 ;
  public: static const uint32_t kInconsistentBitRateSettings        = 1 << 19 ;
  public: static const uint32_t kCannotAllocateDriverReceiveBuffer  = 1 << 20 ;
  public: static const uint32_t kCannotAllocateDriverTransmitBuffer = 1 << 21 ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Interrupt Handler
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static void IRAM_ATTR isr (void * inUserArgument) ;
  private: intr_handle_t mInterruptHandler ;

  public: void handleTXInterrupt (void) ;
  public: void handleRXInterrupt (void) ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // STATUS FLAGS
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //--- Status Flags (returns 0 if no error)
  //  Bit 0 : hardware receive FIFO overflow
  //  Bit 1 : driver receive FIFO overflow
  //  Bit 2 : bus off
  //  Bit 3 : reset mode

  public: uint32_t statusFlags (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Recover from Bus-Off
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: bool recoverFromBusOff (void) const ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    No Copy
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  private: ACAN_ESP32 (const ACAN_ESP32 &) = delete ;
  private: ACAN_ESP32 & operator = (const ACAN_ESP32 &) = delete ;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Driver instances
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: static ACAN_ESP32 can ;

  #ifdef CONFIG_IDF_TARGET_ESP32C6
    public: static ACAN_ESP32 can1 ;
  #endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //    Register Access
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_MODE_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x000)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_CMD_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x004)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_STATUS_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x008)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_INT_RAW_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x00C)) ;
  }

  public: inline volatile uint32_t & TWAI_INT_ENA_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x010)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_BUS_TIMING_0_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x018)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_BUS_TIMING_1_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x01C)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_ARB_LOST_CAP_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x02C)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_ERR_CODE_CAP_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x030)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_ERR_WARNING_LIMIT_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x034)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_RX_ERR_CNT_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x038)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_TX_ERR_CNT_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x03C)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_FRAME_INFO (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x040)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //----- SFF : Standard Frame Format - array size: 2
  public: inline volatile uint32_t & TWAI_ID_SFF (const uint32_t inIndex) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x044 + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //----- EFF : Extended Frame Format - array size: 4
  public: inline volatile uint32_t & TWAI_ID_EFF (const uint32_t inIndex) const {
    return * ((volatile uint32_t *)(twaiBaseAddress + 0x044 + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //----- DATA array size: 8
  public: inline volatile uint32_t & TWAI_DATA_SFF (const uint32_t inIndex) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x04C + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //----- DATA array size: 8
  public: inline volatile uint32_t & TWAI_DATA_EFF (const uint32_t inIndex) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x054 + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // CAN Acceptance Filter Registers
  //----- CODE array size: 4
  public: inline volatile uint32_t & TWAI_ACC_CODE_FILTER (const uint32_t inIndex) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x040 + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  //----- MASK array size: 4
  public: inline volatile uint32_t & TWAI_ACC_MASK_FILTER (const uint32_t inIndex) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x050 + 4 * inIndex)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_RX_MESSAGE_COUNTER_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x074)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  public: inline volatile uint32_t & TWAI_CLOCK_DIVIDER_REG (void) const {
    return * ((volatile uint32_t *) (twaiBaseAddress + 0x07C)) ;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} ;

//------------------------------------------------------------------------------
// TWAI_MODE_REG bit definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_RESET_MODE       = 0x01 ;
static const uint32_t TWAI_LISTEN_ONLY_MODE = 0x02 ;
static const uint32_t TWAI_SELF_TEST_MODE   = 0x04 ;
static const uint32_t TWAI_RX_FILTER_MODE   = 0x08 ;

//------------------------------------------------------------------------------
// TWAI_CMD bit definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_TX_REQ      = 0x01 ;
static const uint32_t TWAI_ABORT_TX    = 0x02 ;
static const uint32_t TWAI_RELEASE_BUF = 0x04 ;
static const uint32_t TWAI_CLR_OVERRUN = 0x08 ;
static const uint32_t TWAI_SELF_RX_REQ = 0x10 ;

//------------------------------------------------------------------------------
// TWAI_STATUS_REG bit definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_RX_BUF_ST   = 0x01 ;
static const uint32_t TWAI_OVERRUN_ST  = 0x02 ;
static const uint32_t TWAI_TX_BUF_ST   = 0x04 ;
static const uint32_t TWAI_TX_COMPLETE = 0x08 ;
static const uint32_t TWAI_RX_ST       = 0x10 ;
static const uint32_t TWAI_TX_ST       = 0x20 ;
static const uint32_t TWAI_ERR_ST      = 0x40 ;
static const uint32_t TWAI_BUS_OFF_ST  = 0x80 ;

//------------------------------------------------------------------------------
// TWAI_INT_RAW_REG bit definitins
//------------------------------------------------------------------------------

static const uint32_t TWAI_RX_INT_ST          = 0x01;
static const uint32_t TWAI_TX_INT_ST          = 0x02;
static const uint32_t TWAI_ERR_WARN_INT_ST    = 0x04;
static const uint32_t TWAI_OVERRUN_INT_ST     = 0x08;
static const uint32_t TWAI_ERR_PASSIVE_INT_ST = 0x20;
static const uint32_t TWAI_ARB_LOST_INT_ST    = 0x40;
static const uint32_t TWAI_BUS_ERR_INT_ST     = 0x80;

//------------------------------------------------------------------------------
// TWAI_INT_ENA_REG bit definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_RX_INT_ENA = 0x01 ;
static const uint32_t TWAI_TX_INT_ENA = 0x02 ;

//------------------------------------------------------------------------------
// TWAI_FRAME_INFO bit definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_FRAME_FORMAT_SFF = 0x00 ;
static const uint32_t TWAI_FRAME_FORMAT_EFF = 0x80 ;
static const uint32_t TWAI_RTR              = 0x40 ;

//------------------------------------------------------------------------------
// CAN FRAME REGISTERS definitions
//------------------------------------------------------------------------------

static const uint32_t TWAI_MSG_STD_ID = 0x7FF ;
static const uint32_t TWAI_MSG_EXT_ID = 0x1FFFFFFF ;

//------------------------------------------------------------------------------
// TWAI_CLOCK_DIVIDER_REG bit definition
//------------------------------------------------------------------------------

static const uint32_t TWAI_EXT_MODE = 0x80 ;

//------------------------------------------------------------------------------
