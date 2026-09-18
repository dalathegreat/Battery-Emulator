#include "TWAI_ESP32.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <new>


// We can't use a conventional constructor/destructor, since we can't reliably
// destruct nodes in all conditions, which would then leave dangling pointers
// and leak hardware allocations.

// Instead we have static pointers to the instances (one per controller),
// allocating them first access.

static TWAI_ESP32 *s_instances[SOC_TWAI_CONTROLLER_NUM];
static portMUX_TYPE s_instance_mux = portMUX_INITIALIZER_UNLOCKED;

// Return the instance corresponding to the given controller index, creating it
// if necessary.
TWAI_ESP32 &TWAI_ESP32::instance(uint8_t controller, gpio_num_t tx_pin, gpio_num_t rx_pin) {
  if (controller >= SOC_TWAI_CONTROLLER_NUM) {
    // Chip doesn't have this many controllers.
    abort();
  }
  portENTER_CRITICAL(&s_instance_mux);
  TWAI_ESP32 *can = s_instances[controller];
  if (can == nullptr) {
    // Each controller slot must be wired to its own pin pair (the GPIO matrix
    // would silently remap shared pins otherwise).
    for (uint8_t i = 0; i < SOC_TWAI_CONTROLLER_NUM; i++) {
      TWAI_ESP32 *other = s_instances[i];
      if (other != nullptr && (other->_tx_pin == tx_pin || other->_rx_pin == rx_pin)) {
        portEXIT_CRITICAL(&s_instance_mux);
        // Pins already in-use by another controller instance.
        abort();
      }
    }
    void *mem = heap_caps_malloc(sizeof(TWAI_ESP32), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (mem == nullptr) {
      portEXIT_CRITICAL(&s_instance_mux);
      // Failed to allocate, abort.
      abort();
    }
    can = new (mem) TWAI_ESP32(tx_pin, rx_pin); // Placement-new; never destroyed
    s_instances[controller] = can;
  } else if (can->_tx_pin != tx_pin || can->_rx_pin != rx_pin) {
    portEXIT_CRITICAL(&s_instance_mux);
    // Pins don't match the original allocation, abort.
    abort();
  }
  portEXIT_CRITICAL(&s_instance_mux);
  return *can;
}

TWAI_ESP32::TWAI_ESP32(gpio_num_t tx_pin, gpio_num_t rx_pin)
    : _rx_buffer(), _tx_pin(tx_pin), _rx_pin(rx_pin) {}

TWAI_ESP32::~TWAI_ESP32() = default; // Never invoked: instances are immortal

// Stop and delete the current node (used internally)
void TWAI_ESP32::teardownNode(void) {
  if (_node == nullptr) {
    return;
  }
  twai_node_status_t status;
  const bool bus_off = (twai_node_get_info(_node, &status, nullptr) == ESP_OK) &&
                       (status.state == TWAI_ERROR_BUS_OFF);
  if (bus_off) {
    // The driver cannot disable a bus-off node, so try to recover it first.
    if (recoverAndWait(_node) != ESP_OK) {
      return; // Bus still down - keep the node attached for later recovery
    }
  }
  if (twai_node_disable(_node) != ESP_OK) {
    return; // Cannot disable the node - leave it attached
  }
  twai_node_delete(_node); // Requires the node to be disabled first
  _node = nullptr;
  _node_enabled = false;
  resetTxSlots();
  _rx_head = 0;
  _rx_tail = 0;
  _rx_count = 0;
}

// Attempt bus-off recovery, blocking until success or timeout.
esp_err_t TWAI_ESP32::recoverAndWait(twai_node_handle_t node) {
  const esp_err_t err = twai_node_recover(node);
  if (err != ESP_OK) {
    return err;
  }
  const uint32_t start = millis();
  twai_node_status_t status;
  do {
    if (twai_node_get_info(node, &status, nullptr) != ESP_OK) {
      return ESP_ERR_INVALID_STATE;
    }
    if (status.state != TWAI_ERROR_BUS_OFF) {
      return ESP_OK;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  } while ((uint32_t)(millis() - start) < kRecoveryTimeoutMs);
  return ESP_ERR_TIMEOUT;
}

// Start the controller with the given bitrate, pins and mode.
uint32_t TWAI_ESP32::begin(const uint32_t bit_rate, const CANMode mode) {
  _bit_rate = bit_rate;
  _mode = mode;

  // If the node already exists, delete it first. On a bus-off node this tries
  // recovery first; if the bus is still down, the node is kept attached so a
  // later begin() / recoverFromBusOff() can retry.
  teardownNode();

  resetTxSlots();
  _rx_head = 0;
  _rx_tail = 0;
  _rx_count = 0;

  twai_onchip_node_config_t node_config = {};
  node_config.io_cfg.tx = _tx_pin;
  node_config.io_cfg.rx = _rx_pin;
  node_config.io_cfg.quanta_clk_out = GPIO_NUM_NC;
  node_config.io_cfg.bus_off_indicator = GPIO_NUM_NC;
  node_config.bit_timing.bitrate = _bit_rate;
  node_config.fail_retry_cnt = -1; // Retransmit forever
  node_config.tx_queue_depth = _tx_slot_count;
  switch (_mode) {
    case ListenOnlyMode:
      node_config.flags.enable_listen_only = 1;
      break;
    case LoopBackMode:
      node_config.flags.enable_loopback = 1;
      node_config.flags.enable_self_test = 1;
      break;
    case NormalMode:
    default:
      break;
  }

  // Create the new node
  twai_node_handle_t new_node = nullptr;
  esp_err_t err = twai_new_node_onchip(&node_config, &new_node);
  if (err != ESP_OK) {
    return (uint32_t) err; // Keep any existing (e.g. unrecovered bus-off) node
  }
  _node = new_node;

  // Setup callbacks and enable the node
  twai_event_callbacks_t callbacks = {};
  callbacks.on_tx_done = txDoneCallback;
  callbacks.on_rx_done = rxDoneCallback;
  esp_err_t setup_err = twai_node_register_event_callbacks(_node, &callbacks, this);
  if (setup_err == ESP_OK) {
    setup_err = twai_node_enable(_node);
  }
  if (setup_err != ESP_OK) {
    // The node is still stopped, so it can be discarded cleanly.
    twai_node_disable(_node); // May report "already disabled"; ignored
    twai_node_delete(_node);
    _node = nullptr;
    _node_enabled = false;
    return (uint32_t) setup_err;
  }

  _node_enabled = true;
  return 0; // ESP_OK
}

void TWAI_ESP32::end(void) {
  teardownNode();
}

uint32_t TWAI_ESP32::restart(void) {
  if (_bit_rate == 0) {
    return ESP_ERR_INVALID_STATE; // Never begun, so we have no settings to reapply
  }
  return begin(_bit_rate, _mode);
}

bool TWAI_ESP32::available (void) const {
  return _rx_count > 0;
}

// Read a frame from the receive buffer (non-blocking). Returns true if a frame was read.
bool TWAI_ESP32::receive (CANMessage &outMessage) {
  bool hasMessage = false;
  if (!available()) {
    return false;
  }
  portENTER_CRITICAL(&_mux);
  if (_rx_count > 0) {
    outMessage = _rx_buffer[_rx_tail];
    _rx_tail = (uint16_t)((_rx_tail + 1) % _rx_buffer_size);
    _rx_count = (uint16_t)(_rx_count - 1);
    hasMessage = true;
  }
  portEXIT_CRITICAL(&_mux);
  return hasMessage;
}

// Attempt to enqueue a message for transmission (non-blocking)
bool TWAI_ESP32::tryToSend(const CANMessage &message) {
  if (_node == nullptr) {
    return false;
  }
  // Grab a free transmit slot. These remain in use until the TX callback fires
  // (ESP-IDF references our frame memory).
  TxSlot *slot = nullptr;
  portENTER_CRITICAL(&_mux);
  for (uint8_t i = 0; i < _tx_slot_count; i++) {
    if (!_tx_slots[i].in_use) {
      slot = &_tx_slots[i];
      slot->in_use = true;
      break;
    }
  }
  portEXIT_CRITICAL(&_mux);
  if (slot == nullptr) {
    return false; // All slots busy
  }

  const uint8_t len = (message.len <= 8) ? message.len : 8;
  slot->frame.header = {};
  slot->frame.header.id = message.id;
  slot->frame.header.ide = message.ext ? 1 : 0;
  slot->frame.header.rtr = message.rtr ? 1 : 0;
  slot->frame.header.dlc = len;
  slot->frame.buffer = slot->data;
  slot->frame.buffer_len = len;
  for (uint8_t i = 0; i < len; i++) {
    slot->data[i] = message.data[i];
  }

  const esp_err_t err = twai_node_transmit(_node, &slot->frame, 0); // Non-blocking
  if (err != ESP_OK) {
    // If it failed, recover the slot immediately (the TX callback will not fire)
    portENTER_CRITICAL(&_mux);
    slot->in_use = false;
    portEXIT_CRITICAL(&_mux);
    return false;
  }
  return true;
}

uint32_t TWAI_ESP32::statusRegister(void) const {
  uint32_t result = 0;
  if (_node == nullptr) {
    return result;
  }
  twai_node_status_t status;
  if (twai_node_get_info(_node, &status, nullptr) == ESP_OK) {
    if (status.state != TWAI_ERROR_ACTIVE) {
      result |= TWAI_ERR_ST; // Warning, passive or bus-off
    }
    if (status.state == TWAI_ERROR_BUS_OFF) {
      result |= TWAI_BUS_OFF_ST;
    }
  }
  return result;
}

// Start a bus-off recovery (non-blocking).
// The ESP32's controller doesn't recover automatically.
bool TWAI_ESP32::recoverFromBusOff(void) {
  if (_node == nullptr) {
    return false;
  }
  return twai_node_recover(_node) == ESP_OK;
}

// Called from ESP-IDF ISR context when a TX completes.
// Finds the used transmit slot and marks it free.
TWAI_ISR_CALLBACK_ATTR bool TWAI_ESP32::txDoneCallback(twai_node_handle_t handle,
                                const twai_tx_done_event_data_t *edata,
                                void *user_data) {
  TWAI_ESP32 *self = (TWAI_ESP32*)user_data;
  if (edata->done_tx_frame != nullptr) {
    portENTER_CRITICAL(&self->_mux);
    for (uint8_t i = 0; i < _tx_slot_count; i++) {
      if (&self->_tx_slots [i].frame == edata->done_tx_frame) {
        self->_tx_slots [i].in_use = false;
        break;
      }
    }
    portEXIT_CRITICAL(&self->_mux);
  }
  return false;
}

// Called from ESP-IDF ISR context when a RX completes.
TWAI_ISR_CALLBACK_ATTR bool TWAI_ESP32::rxDoneCallback(twai_node_handle_t handle,
                                const twai_rx_done_event_data_t *edata,
                                void *user_data) {
  TWAI_ESP32 *self = (TWAI_ESP32*)user_data;
  twai_frame_t frame = {};
  uint8_t data[8];
  frame.buffer = data;
  frame.buffer_len = sizeof(data);
  if (twai_node_receive_from_isr(handle, &frame) == ESP_OK) {
    self->storeRxFrame(frame);
  }
  return false; // No task was unblocked
}

TWAI_ISR_CALLBACK_ATTR void TWAI_ESP32::storeRxFrame(const twai_frame_t &frame) {
  CANMessage message;
  message.id = frame.header.id;
  message.ext = frame.header.ide != 0;
  message.rtr = frame.header.rtr != 0;
  message.len = (frame.header.dlc <= 8) ? (uint8_t) frame.header.dlc : 8;
  for (uint8_t i = 0; i < message.len; i++) {
    message.data[i] = frame.buffer[i];
  }
  portENTER_CRITICAL(&_mux);
  if (_rx_count < _rx_buffer_size) {
    // There's room in the ring buffer, so store the new frame and advance the head.
    _rx_buffer[_rx_head] = message;
    _rx_head = (uint16_t)((_rx_head + 1) % _rx_buffer_size);
    _rx_count = (uint16_t)(_rx_count + 1);
  } // (if there was no room, we just drop the frame)
  portEXIT_CRITICAL(&_mux);
}

// Return all the transmit slots to the free state.
void TWAI_ESP32::resetTxSlots(void) {
  portENTER_CRITICAL(&_mux);
  for (uint8_t i = 0; i < _tx_slot_count; i++) {
    _tx_slots [i].in_use = false;
  }
  portEXIT_CRITICAL(&_mux);
}
