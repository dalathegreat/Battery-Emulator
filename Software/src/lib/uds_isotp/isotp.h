#ifndef ISOTP_H
#define ISOTP_H

#include <stdint.h>
#include "isotp_config.h"

enum isotp_tatype {
  ISOTP_TATYPE_PHYSICAL = 0,
  ISOTP_TATYPE_FUNCTIONAL,
};

// Addressing mode (ISO 15765-2 §9).
enum isotp_addrmode {
  ISOTP_ADDRMODE_NORMAL   = 0,  // normal addressing — no extra address byte
  ISOTP_ADDRMODE_EXTENDED = 1,  // extended addressing — N_TAext prepended to every frame
};

/**
 * @brief C++ ISO-TP transport layer mixin.
 *
 * Derive from this class and override on_isotp_can_tx() and
 * on_isotp_rx_complete() to couple the protocol to your CAN driver
 * and message handler — no C-style callback registration required.
 *
 * Typical usage in a CanBattery subclass that also inherits IsoTp:
 *   - setup():          call isotp_init(tx_id)
 *   - transmit_can():   call isotp_poll()   every 1 ms
 *   - incoming_can():   call isotp_receive() for the relevant CAN ID
 *   - on_isotp_can_tx():      forward to transmit_can_frame()
 *   - on_isotp_rx_complete(): process the assembled UDS response
 */
class IsoTp {
 public:
  /**
   * Initialise the instance. Must be called once before any other method.
   *
   * @param tx_id      CAN ID used for all transmitted frames.
   * @param addrmode   ISOTP_ADDRMODE_NORMAL (default) or ISOTP_ADDRMODE_EXTENDED.
   * @param tx_addr    N_TAext byte prepended to TX frames (extended mode only).
   * @param rx_addr    Expected N_TAext byte on RX frames; frames with a different
   *                   value are silently discarded (extended mode only).
   */
  void isotp_init(uint32_t tx_id,
                  isotp_addrmode addrmode = ISOTP_ADDRMODE_NORMAL,
                  uint8_t tx_addr = 0x00,
                  uint8_t rx_addr = 0x00);

  /** Enqueue a message for transmission (up to CONFIG_ISOTP_MAX_MSG_LENGTH bytes). */
  void isotp_send(const uint8_t* data, int len);

  /** Feed a raw received CAN frame into the state machine. */
  void isotp_receive(const uint8_t* can_data, uint8_t can_dlc, isotp_tatype tatype);

  /** Must be called every 1 ms to drive timers and retransmissions. */
  void isotp_poll();

  /** Check if the ISO-TP instance is currently busy (transmitting or receiving). */
  bool isotp_is_busy() const;

 protected:
  /** Called by the protocol layer when it needs to emit a raw CAN frame. */
  virtual void on_isotp_can_tx(uint32_t can_id, const uint8_t* can_data, uint8_t can_dlc) = 0;

  /** Called when a complete ISO-TP message has been assembled. */
  virtual void on_isotp_rx_complete(const uint8_t* data, int len, isotp_tatype tatype) = 0;

 private:
  struct TpCon {
    int idx = 0;
    int len = 0;
    int state = 0;  // ISOTP_IDLE
    uint8_t bs = 0;
    uint8_t sn = 0;
    uint8_t buf[CONFIG_ISOTP_MAX_MSG_LENGTH] = {};
  };

  struct FcOpts {
    uint8_t bs = 0;
    uint8_t stmin = 0;
  };

  TpCon _rx{}, _tx{};
  FcOpts _rxfc{}, _txfc{};
  uint8_t _tx_wft = 0;
  isotp_tatype _tatype = ISOTP_TATYPE_PHYSICAL;
  uint32_t _rxtimer = 0;
  uint32_t _txtimer = 0;
  uint32_t _tx_id = 0;
  isotp_addrmode _addrmode = ISOTP_ADDRMODE_NORMAL;
  uint8_t _tx_addr = 0;
  uint8_t _rx_addr = 0;

  uint8_t addr_off() const { return (_addrmode == ISOTP_ADDRMODE_EXTENDED) ? 1 : 0; }

  void send_fc(uint8_t flowstatus);
  void rcv_fc(const uint8_t* can_data, uint8_t can_dlc);
  void rcv_sf(const uint8_t* can_data, uint8_t can_dlc);
  void rcv_ff(const uint8_t* can_data, uint8_t can_dlc);
  void rcv_cf(const uint8_t* can_data, uint8_t can_dlc);
  void do_send_sf();
  void do_send_ff();
  void do_send_cf();
};

#endif  // ISOTP_H
