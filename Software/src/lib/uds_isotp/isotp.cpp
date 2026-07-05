#include <string.h>
#include "isotp.h"

/* PCI – Protocol Control Information */
#define N_PCI_SF 0x00 /* single frame      */
#define N_PCI_FF 0x10 /* first frame       */
#define N_PCI_CF 0x20 /* consecutive frame */
#define N_PCI_FC 0x30 /* flow control      */

#define N_PCI_SZ   1  /* size of PCI byte #1                */
#define SF_PCI_SZ  1  /* SingleFrame PCI including 4-bit SF_DL */
#define FF_PCI_SZ  2  /* FirstFrame  PCI including 12-bit FF_DL */
#define FC_CONTENT_SZ 3 /* flow control content size (FS/BS/STmin) */

/* Flow Status values in Flow Control frame */
#define ISOTP_FC_CTS   0  /* clear to send */
#define ISOTP_FC_WT    1  /* wait          */
#define ISOTP_FC_OVFLW 2  /* overflow      */

/* State machine states (stored in TpCon::state) */
enum {
  ISOTP_IDLE = 0,
  ISOTP_WAIT_FIRST_FC,
  ISOTP_WAIT_FC,
  ISOTP_WAIT_DATA,
  ISOTP_SENDING
};

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void IsoTp::send_fc(uint8_t flowstatus) {
  uint8_t off = addr_off();
  uint8_t can_data[8] = {0};
  if (off) can_data[0] = _tx_addr;
  can_data[off + 0] = N_PCI_FC | flowstatus;
  can_data[off + 1] = _rxfc.bs;
  can_data[off + 2] = _rxfc.stmin;
  memset(&can_data[off + 3], CONFIG_ISOTP_TX_PADDING_BYTE, 8 - off - 3);
  on_isotp_can_tx(_tx_id, can_data, 8);
  _rx.bs = 0;
  _rxtimer = CONFIG_ISOTP_CR_TIMEOUT;
}

void IsoTp::rcv_fc(uint8_t* can_data, uint8_t can_dlc) {
  uint8_t off = addr_off();
  if (_tx.state == ISOTP_WAIT_FC || _tx.state == ISOTP_WAIT_FIRST_FC) {
    _txtimer = 0;
    if (can_dlc >= FC_CONTENT_SZ + off) {
      if (_tx.state == ISOTP_WAIT_FIRST_FC) {
        _txfc.bs    = can_data[off + 1];
        _txfc.stmin = can_data[off + 2];
        if ((_txfc.stmin > 0x7F) && ((_txfc.stmin < 0xF1) || (_txfc.stmin > 0xF9))) {
          _txfc.stmin = 0x7F;
        }
        if (0xF1 <= _txfc.stmin && _txfc.stmin <= 0xF9) {
          /* 100 µs – 900 µs: use 1 ms resolution */
          _txfc.stmin = 1;
        }
        _tx.state = ISOTP_WAIT_FC;
      }
      switch (can_data[off] & 0x0F) {
        case ISOTP_FC_CTS:
          _tx.bs    = 0;
          _tx_wft   = 0;
          _tx.state = ISOTP_SENDING;
          _txtimer  = _txfc.stmin;
          break;
        case ISOTP_FC_WT:
          if (_tx_wft++ >= CONFIG_ISOTP_WFTMAX) {
            _tx.state = ISOTP_IDLE;
          } else {
            _txtimer = CONFIG_ISOTP_BS_TIMEOUT;
          }
          break;
        case ISOTP_FC_OVFLW:
        default:
          _tx.state = ISOTP_IDLE;
          break;
      }
    } else {
      _tx.state = ISOTP_IDLE;
    }
  }
}

void IsoTp::rcv_sf(uint8_t* can_data, uint8_t can_dlc) {
  uint8_t off = addr_off();
  uint8_t sf_pci_size = SF_PCI_SZ;
  _rxtimer  = 0;
  _rx.state = ISOTP_IDLE;
  _rx.len   = (can_data[off] & 0x0F);
  if (_rx.len == 0) {
    /* ISO 15765-2 CAN FD: PCI byte = 0x00, next byte = length */
    sf_pci_size = SF_PCI_SZ + 1;
    _rx.len     = can_data[off + 1];
  }
  if (0 < _rx.len && _rx.len <= can_dlc - sf_pci_size - off) {
    _rx.idx = 0;
    for (int i = off + sf_pci_size; i < off + sf_pci_size + _rx.len; i++) {
      _rx.buf[_rx.idx++] = can_data[i];
    }
    on_isotp_rx_complete(_rx.buf, _rx.len, _tatype);
  }
}

void IsoTp::rcv_ff(uint8_t* can_data, uint8_t can_dlc) {
  uint8_t off = addr_off();
  _rxtimer  = 0;
  _rx.state = ISOTP_IDLE;
  _rx.len   = (can_data[off] & 0x0F) << 8;
  _rx.len  += can_data[off + 1];
  /* FF_DL must exceed the maximum SF payload for this addressing mode */
  if (_rx.len > 7 - off) {
    if (_rx.len <= CONFIG_ISOTP_MAX_MSG_LENGTH) {
      _rx.idx = 0;
      for (int i = off + FF_PCI_SZ; i < can_dlc; i++) {
        _rx.buf[_rx.idx++] = can_data[i];
      }
      _rx.sn    = 1;
      _rx.state = ISOTP_WAIT_DATA;
      send_fc(ISOTP_FC_CTS);
    } else {
      send_fc(ISOTP_FC_OVFLW);
    }
  }
}

void IsoTp::rcv_cf(uint8_t* can_data, uint8_t can_dlc) {
  uint8_t off = addr_off();
  if (_rx.state == ISOTP_WAIT_DATA) {
    _rxtimer = 0;
    if ((can_data[off] & 0x0F) == _rx.sn) {
      _rx.sn++;
      if (_rx.sn > 15) {
        _rx.sn = 0;
      }
      for (int i = off + N_PCI_SZ; i < can_dlc && _rx.idx < _rx.len; i++) {
        _rx.buf[_rx.idx++] = can_data[i];
      }
      if (_rx.idx < _rx.len) {
        if (!_rxfc.bs || ++_rx.bs < _rxfc.bs) {
          _rxtimer = CONFIG_ISOTP_CR_TIMEOUT;
        } else {
          send_fc(ISOTP_FC_CTS);
        }
      } else {
        on_isotp_rx_complete(_rx.buf, _rx.len, _tatype);
        _rx.state = ISOTP_IDLE;
      }
    } else {
      _rx.state = ISOTP_IDLE;
    }
  }
}

void IsoTp::do_send_sf() {
  uint8_t off = addr_off();
  uint8_t can_data[8] = {0};
  if (off) can_data[0] = _tx_addr;
  can_data[off] = N_PCI_SF | _tx.len;
  for (int i = 0; i < _tx.len; i++) {
    can_data[off + SF_PCI_SZ + i] = _tx.buf[_tx.idx++];
  }
  memset(&can_data[off + SF_PCI_SZ + _tx.len], CONFIG_ISOTP_TX_PADDING_BYTE,
         8 - off - SF_PCI_SZ - _tx.len);
  on_isotp_can_tx(_tx_id, can_data, 8);
  _tx.state = ISOTP_IDLE;
}

void IsoTp::do_send_ff() {
  uint8_t off = addr_off();
  uint8_t can_data[8] = {0};
  if (off) can_data[0] = _tx_addr;
  can_data[off + 0] = N_PCI_FF | ((_tx.len >> 8) & 0x0F);
  can_data[off + 1] = _tx.len & 0xFF;
  for (int i = 0; i < 6 - off; i++) {
    can_data[off + FF_PCI_SZ + i] = _tx.buf[_tx.idx++];
  }
  on_isotp_can_tx(_tx_id, can_data, 8);
  _tx.sn    = 1;
  _tx.state = ISOTP_WAIT_FIRST_FC;
  _txtimer  = CONFIG_ISOTP_BS_TIMEOUT;
}

void IsoTp::do_send_cf() {
  uint8_t off    = addr_off();
  uint8_t remain = static_cast<uint8_t>(_tx.len - _tx.idx);
  uint8_t num    = (remain < 7u - off) ? remain : 7u - off;
  uint8_t can_data[8] = {0};
  if (off) can_data[0] = _tx_addr;
  can_data[off] = N_PCI_CF | _tx.sn++;
  if (_tx.sn > 15) {
    _tx.sn = 0;
  }
  _tx.bs++;
  for (int i = 0; i < num; i++) {
    can_data[off + N_PCI_SZ + i] = _tx.buf[_tx.idx++];
  }
  if (num < 7u - off) {
    memset(&can_data[off + N_PCI_SZ + num], CONFIG_ISOTP_TX_PADDING_BYTE, 7u - off - num);
  }
  on_isotp_can_tx(_tx_id, can_data, 8);
  if (_tx.idx < _tx.len) {
    if (_txfc.bs && _tx.bs >= _txfc.bs) {
      _tx.state = ISOTP_WAIT_FC;
      _txtimer  = CONFIG_ISOTP_BS_TIMEOUT;
    } else {
      _txtimer = _txfc.stmin;
    }
  } else {
    _tx.state = ISOTP_IDLE;
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void IsoTp::isotp_init(uint32_t tx_id, isotp_addrmode addrmode, uint8_t tx_addr, uint8_t rx_addr) {
  _rx.state   = ISOTP_IDLE;
  _tx.state   = ISOTP_IDLE;
  _rxfc.bs    = CONFIG_ISOTP_BS;
  _rxfc.stmin = CONFIG_ISOTP_STMIN;
  _rxtimer    = 0;
  _txtimer    = 0;
  _tx_id      = tx_id;
  _addrmode   = addrmode;
  _tx_addr    = tx_addr;
  _rx_addr    = rx_addr;
}

void IsoTp::isotp_send(uint8_t* data, int len) {
  uint8_t off = addr_off();
  if (_tx.state == ISOTP_IDLE && len <= CONFIG_ISOTP_MAX_MSG_LENGTH) {
    memcpy(_tx.buf, data, len);
    _tx.state = ISOTP_SENDING;
    _tx.len   = len;
    _tx.idx   = 0;
    if (len <= 7 - off) {
      do_send_sf();
    } else {
      do_send_ff();
    }
  }
}

void IsoTp::isotp_receive(uint8_t* can_data, uint8_t can_dlc, isotp_tatype tatype) {
  uint8_t off = addr_off();
  /* in extended mode, silently discard frames not addressed to us */
  if (off && can_data[0] != _rx_addr) return;
  uint8_t n_pci_type = can_data[off] & 0xF0;
  /* half-duplex: only handle FC when RX is idle, other frames when TX is idle */
  if ((n_pci_type == N_PCI_FC && _rx.state == ISOTP_IDLE) ||
      (n_pci_type != N_PCI_FC && _tx.state == ISOTP_IDLE)) {
    /* functional addressing only allowed for single frames */
    if (tatype == ISOTP_TATYPE_PHYSICAL || n_pci_type == N_PCI_SF) {
      _tatype = tatype;
      switch (n_pci_type) {
        case N_PCI_FC: rcv_fc(can_data, can_dlc); break;
        case N_PCI_SF: rcv_sf(can_data, can_dlc); break;
        case N_PCI_FF: rcv_ff(can_data, can_dlc); break;
        case N_PCI_CF: rcv_cf(can_data, can_dlc); break;
      }
    }
  }
}

void IsoTp::isotp_poll() {
  switch (_tx.state) {
    case ISOTP_WAIT_FC:
    case ISOTP_WAIT_FIRST_FC:
      if (_txtimer > 0) {
        _txtimer--;
        if (_txtimer == 0) {
          _tx.state = ISOTP_IDLE;
        }
      }
      break;
    case ISOTP_SENDING:
      if (_txfc.stmin) {
        if (_txtimer > 0) {
          _txtimer--;
          if (_txtimer == 0) {
            do_send_cf();
          }
        }
      } else {
        do_send_cf();
      }
      break;
    default:
      break;
  }

  switch (_rx.state) {
    case ISOTP_WAIT_DATA:
      if (_rxtimer > 0) {
        _rxtimer--;
        if (_rxtimer == 0) {
          _rx.state = ISOTP_IDLE;
        }
      }
      break;
    default:
      break;
  }
}

bool IsoTp::isotp_is_busy() const{
  return _rx.state != ISOTP_IDLE || _tx.state != ISOTP_IDLE;
}
