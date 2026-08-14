#include "TWAI_ESP32.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//----------------------------------------------------------------------------------------
//  Bit timing
//
//  Mirror of the ESP-IDF TWAI driver's timing calculation (twai_node_timing_calc_param)
//  with the APB clock as source, so the values exposed by the query methods match
//  exactly what the driver programs into the controller. The HAL writes brp/2 - 1 into
//  the BRP field, which compensates for the hardware's fixed /2 on the APB clock.
//----------------------------------------------------------------------------------------

// Bit timing constraints of the TWAI controller on ESP32 / ESP32-S3 (TWAI_LL_* values)
static const uint32_t kBrpMin  = 2 ;
static const uint32_t kBrpMax  = 1024 ;
static const uint16_t kTseg1Max = 16 ;
static const uint16_t kTseg2Max = 8 ;
static const uint16_t kTseg1Min = 1 ;
static const uint16_t kTseg2Min = 1 ;
static const uint16_t kSjwMax   = 4 ;

bool TWAI_ESP32::computeBitTiming (const uint32_t inBitRate) {
  const uint32_t sourceFreq = getApbFrequency () ; // 80 MHz on ESP32 / ESP32-S3
  const uint32_t totalDiv = (sourceFreq + inBitRate / 2) / inBitRate ;
  uint32_t preDiv = kBrpMin ;
  uint16_t tseg = 0 ;
  for (; preDiv <= kBrpMax ; preDiv ++) {
    tseg = (uint16_t)(totalDiv / preDiv) ;
    if (totalDiv != (uint32_t)tseg * preDiv) {
      continue ; // Not an integer number of time quanta
    }
    if ((tseg <= (kTseg1Max + kTseg2Max + 1)) && (tseg >= (kTseg1Min + kTseg2Min))) {
      break ;
    }
  }
  if (preDiv > kBrpMax) { // Unsupported bit rate; begin() will fail as well
    mBitRatePrescaler = 0 ;
    mTimeSegment1 = 0 ;
    mTimeSegment2 = 0 ;
    mRJW = 0 ;
    return false ;
  }
  const uint16_t defaultPoint =
      (inBitRate >= 800000) ? 750 : ((inBitRate >= 500000) ? 800 : 875) ;
  uint16_t tseg1 = (uint16_t)((tseg * defaultPoint) / 1000) - 1 ;
  if (tseg1 < kTseg1Min) tseg1 = kTseg1Min ;
  if (tseg1 > kTseg1Max) tseg1 = kTseg1Max ;
  uint16_t tseg2 = (uint16_t)(tseg - tseg1 - 1) ;
  if (tseg2 < kTseg2Min) tseg2 = kTseg2Min ;
  if (tseg2 > kTseg2Max) tseg2 = kTseg2Max ;
  uint16_t prop = tseg1 / 4 ;
  if (prop < 1) prop = 1 ;
  tseg1 -= prop ;
  uint16_t sjw = tseg2 >> 1 ;
  if (sjw < 1) sjw = 1 ;
  if (sjw > kSjwMax) sjw = kSjwMax ;
  mBitRatePrescaler = (uint8_t) preDiv ;
  mTimeSegment1 = (uint8_t)(prop + tseg1) ; // Propagation + phase segment 1
  mTimeSegment2 = (uint8_t) tseg2 ;
  mRJW = (uint8_t) sjw ;
  return true ;
}

//----------------------------------------------------------------------------------------
//  TWAI_ESP32
//----------------------------------------------------------------------------------------

TWAI_ESP32::TWAI_ESP32 (void) :
mTxSlots (),
mRxBuffer () {
}

TWAI_ESP32 TWAI_ESP32::can ;

//----------------------------------------------------------------------------------------
//   Stop and delete the current node, if any
//----------------------------------------------------------------------------------------

void TWAI_ESP32::teardownNode (void) {
  if (mNode == nullptr) {
    return ;
  }
  esp_err_t err = twai_node_disable (mNode) ;
  if ((err == ESP_ERR_INVALID_STATE) && mNodeEnabled) {
    // The node reports bus-off. A disabled node also reports bus-off, but then
    // mNodeEnabled is false. So this is a node that is still enabled while the
    // controller is in bus-off: disable() refuses to stop it until it recovers.
    // Start recovery and wait for it to leave the bus-off state. We must not
    // delete an enabled node (its ISR could still be running), so only delete
    // once it has actually recovered and been disabled.
    twai_node_recover (mNode) ;
    twai_node_status_t status ;
    bool recovered = false ;
    for (int i = 0 ; i < 100 ; i ++) { // Up to ~100 ms
      if ((twai_node_get_info (mNode, & status, nullptr) == ESP_OK) &&
          (status.state != TWAI_ERROR_BUS_OFF)) {
        recovered = true ;
        break ;
      }
      vTaskDelay (pdMS_TO_TICKS (1)) ;
    }
    if (!recovered) {
      // Bus is still broken (e.g. shorted); keep the (bus-off) node so a later
      // begin() can retry. Any new node creation will fail until it recovers.
      return ;
    }
    twai_node_disable (mNode) ; // Should succeed once recovered
  }
  mNodeEnabled = false ;
  twai_node_delete (mNode) ; // Requires the node to be disabled (bus-off state)
  mNode = nullptr ;
  resetTxSlots () ;
  mRxHead = 0 ;
  mRxTail = 0 ;
  mRxCount = 0 ;
}

//----------------------------------------------------------------------------------------
//   BEGIN / RESTART
//----------------------------------------------------------------------------------------

uint32_t TWAI_ESP32::begin (const uint32_t inBitRate,
                            const gpio_num_t inTxPin,
                            const gpio_num_t inRxPin,
                            const CANMode inMode) {
  if (!computeBitTiming (inBitRate)) {
    return ESP_ERR_NOT_SUPPORTED ; // Unsupported bit rate; leave any running node alone
  }
  mBitRate = inBitRate ;
  if (inTxPin != GPIO_NUM_NC) mTxPin = inTxPin ; // GPIO_NUM_NC keeps the previous pin
  if (inRxPin != GPIO_NUM_NC) mRxPin = inRxPin ;
  mMode = inMode ;

  teardownNode () ; // Re-init path (speed change, restart, bus-off recovery)

  resetTxSlots () ;
  mRxHead = 0 ;
  mRxTail = 0 ;
  mRxCount = 0 ;

  twai_onchip_node_config_t nodeConfig = {} ;
  nodeConfig.io_cfg.tx = mTxPin ;
  nodeConfig.io_cfg.rx = mRxPin ;
  nodeConfig.io_cfg.quanta_clk_out = GPIO_NUM_NC ;
  nodeConfig.io_cfg.bus_off_indicator = GPIO_NUM_NC ;
  nodeConfig.bit_timing.bitrate = mBitRate ;
  nodeConfig.fail_retry_cnt = -1 ; // Retransmit forever, like the old driver
  nodeConfig.tx_queue_depth = kTxSlotCount ;
  switch (mMode) {
    case ListenOnlyMode :
      nodeConfig.flags.enable_listen_only = 1 ;
      break ;
    case LoopBackMode : // Self-test: no ACK required + self-reception
      nodeConfig.flags.enable_loopback = 1 ;
      nodeConfig.flags.enable_self_test = 1 ;
      break ;
    case NormalMode :
    default :
      break ;
  }

  twai_node_handle_t newNode = nullptr ;
  esp_err_t err = twai_new_node_onchip (& nodeConfig, & newNode) ;
  if (err != ESP_OK) {
    return (uint32_t) err ; // Keep any existing (e.g. unrecovered bus-off) node
  }
  mNode = newNode ;

  twai_event_callbacks_t callbacks = {} ;
  callbacks.on_tx_done = txDoneCallback ;
  callbacks.on_rx_done = rxDoneCallback ;
  err = twai_node_register_event_callbacks (mNode, & callbacks, this) ;
  if (err != ESP_OK) {
    twai_node_disable (mNode) ; // No-op if the fresh node is still stopped
    twai_node_delete (mNode) ;
    mNode = nullptr ;
    return (uint32_t) err ;
  }

  err = twai_node_enable (mNode) ;
  if (err != ESP_OK) {
    twai_node_disable (mNode) ; // Stop again if enable partially succeeded
    twai_node_delete (mNode) ;
    mNode = nullptr ;
    return (uint32_t) err ;
  }
  mNodeEnabled = true ;
  return 0 ; // ESP_OK
}

//----------------------------------------------------------------------------------------
//   END / RESTART
//----------------------------------------------------------------------------------------

void TWAI_ESP32::end (void) {
  teardownNode () ;
}

uint32_t TWAI_ESP32::restart (void) {
  if (mBitRate == 0) {
    return ESP_ERR_INVALID_STATE ; // Never begun
  }
  return begin (mBitRate, mTxPin, mRxPin, mMode) ;
}

//----------------------------------------------------------------------------------------
//   RECEPTION
//----------------------------------------------------------------------------------------

bool TWAI_ESP32::available (void) const {
  return mRxCount > 0 ;
}

bool TWAI_ESP32::receive (CANMessage & outMessage) {
  bool hasMessage = false ;
  if (!available ()) {
    return false ;
  }
  portENTER_CRITICAL (& mMux) ;
  if (mRxCount > 0) {
    outMessage = mRxBuffer [mRxTail] ;
    mRxTail = (uint16_t)((mRxTail + 1) % kRxBufferSize) ;
    mRxCount = (uint16_t)(mRxCount - 1) ;
    hasMessage = true ;
  }
  portEXIT_CRITICAL (& mMux) ;
  return hasMessage ;
}

//----------------------------------------------------------------------------------------
//   TRANSMISSION
//----------------------------------------------------------------------------------------

bool TWAI_ESP32::tryToSend (const CANMessage & inMessage) {
  if (mNode == nullptr) {
    return false ;
  }
  // Grab a free transmit slot
  TxSlot * slot = nullptr ;
  portENTER_CRITICAL (& mMux) ;
  for (uint8_t i = 0 ; i < kTxSlotCount ; i ++) {
    if (!mTxSlots [i].inUse) {
      slot = & mTxSlots [i] ;
      slot->inUse = true ;
      break ;
    }
  }
  portEXIT_CRITICAL (& mMux) ;
  if (slot == nullptr) {
    return false ; // All slots busy
  }

  const uint8_t len = (inMessage.len <= 8) ? inMessage.len : 8 ;
  slot->frame.header = {} ;
  slot->frame.header.id = inMessage.id ;
  slot->frame.header.ide = inMessage.ext ? 1 : 0 ;
  slot->frame.header.rtr = inMessage.rtr ? 1 : 0 ;
  slot->frame.header.dlc = len ;
  slot->frame.buffer = slot->data ;
  slot->frame.buffer_len = len ;
  for (uint8_t i = 0 ; i < len ; i ++) {
    slot->data [i] = inMessage.data [i] ;
  }

  const esp_err_t err = twai_node_transmit (mNode, & slot->frame, 0) ; // Non-blocking
  if (err != ESP_OK) {
    portENTER_CRITICAL (& mMux) ;
    slot->inUse = false ;
    portEXIT_CRITICAL (& mMux) ;
    return false ;
  }
  return true ;
}

//----------------------------------------------------------------------------------------
//   STATUS
//----------------------------------------------------------------------------------------

uint32_t TWAI_ESP32::statusRegister (void) const {
  uint32_t result = 0 ;
  if (mNode == nullptr) {
    return result ;
  }
  twai_node_status_t status ;
  if (twai_node_get_info (mNode, & status, nullptr) == ESP_OK) {
    if (status.state != TWAI_ERROR_ACTIVE) {
      result |= TWAI_ERR_ST ; // Warning, passive or bus-off
    }
    if (status.state == TWAI_ERROR_BUS_OFF) {
      result |= TWAI_BUS_OFF_ST ;
    }
  }
  return result ;
}

//----------------------------------------------------------------------------------------
//   EVENT CALLBACKS (invoked from ISR context)
//----------------------------------------------------------------------------------------

bool TWAI_ESP32::txDoneCallback (twai_node_handle_t handle,
                                 const twai_tx_done_event_data_t * edata,
                                 void * userCtx) {
  TWAI_ESP32 * self = (TWAI_ESP32 *) userCtx ;
  if (edata->done_tx_frame != nullptr) {
    portENTER_CRITICAL (& self->mMux) ;
    for (uint8_t i = 0 ; i < kTxSlotCount ; i ++) {
      if (& self->mTxSlots [i].frame == edata->done_tx_frame) {
        self->mTxSlots [i].inUse = false ;
        break ;
      }
    }
    portEXIT_CRITICAL (& self->mMux) ;
  }
  return false ; // No task was unblocked
}

bool TWAI_ESP32::rxDoneCallback (twai_node_handle_t handle,
                                 const twai_rx_done_event_data_t * edata,
                                 void * userCtx) {
  TWAI_ESP32 * self = (TWAI_ESP32 *) userCtx ;
  twai_frame_t frame = {} ;
  frame.buffer = self->mRxScratch ;
  frame.buffer_len = sizeof (self->mRxScratch) ;
  if (twai_node_receive_from_isr (handle, & frame) == ESP_OK) {
    self->storeRxFrame (frame) ;
  }
  return false ; // No task was unblocked
}

void TWAI_ESP32::storeRxFrame (const twai_frame_t & inFrame) {
  CANMessage message ;
  message.id = inFrame.header.id ;
  message.ext = inFrame.header.ide != 0 ;
  message.rtr = inFrame.header.rtr != 0 ;
  message.len = (inFrame.header.dlc <= 8) ? (uint8_t) inFrame.header.dlc : 8 ;
  for (uint8_t i = 0 ; i < message.len ; i ++) {
    message.data [i] = inFrame.buffer [i] ;
  }
  portENTER_CRITICAL (& mMux) ;
  if (mRxCount < kRxBufferSize) {
    mRxBuffer [mRxHead] = message ;
    mRxHead = (uint16_t)((mRxHead + 1) % kRxBufferSize) ;
    mRxCount = (uint16_t)(mRxCount + 1) ;
  } // else: ring full, drop the new frame (mirrors the old driver's overflow drop)
  portEXIT_CRITICAL (& mMux) ;
}

//----------------------------------------------------------------------------------------

void TWAI_ESP32::resetTxSlots (void) {
  portENTER_CRITICAL (& mMux) ;
  for (uint8_t i = 0 ; i < kTxSlotCount ; i ++) {
    mTxSlots [i].inUse = false ;
  }
  portEXIT_CRITICAL (& mMux) ;
}

//----------------------------------------------------------------------------------------
