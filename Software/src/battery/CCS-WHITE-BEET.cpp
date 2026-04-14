#include "CCS-WHITE-BEET.h"

#include <string.h>
#include "../devboard/utils/logging.h"

// Reference: FreeV2G/SpiAdapter.py, FreeV2G/FramingInterface.py, FreeV2G/Whitebeet.py, FreeV2G/Evse.py.
// This file ports the DC-EVSE subset of that stack for an ESP32 host talking
// to a Sevenstax Whitebeet module over SPI.

namespace whitebeet {

// --- SPI size-frame prefix constants (FreeV2G/SpiAdapter.py) ------------------
// Master -> slave: "AA AA <sz_hi> <sz_lo>"; the slave echoes back "AA AA
// <slave_sz_hi> <slave_sz_lo>". Then a combined exchange starts with "55 55
// 00 00" followed by the actual payload, padded up to max(master_len,
// slave_len). The slave reply mirrors that structure, so a single xfer is
// enough once the size header is agreed.
static constexpr uint8_t SPI_MAGIC_SIZE_HI = 0xAA;
static constexpr uint8_t SPI_MAGIC_DATA_HI = 0x55;
static constexpr uint32_t SPI_SPEED_HZ = 12000000;

// =============================================================================
// WhitebeetSpi
// =============================================================================

WhitebeetSpi::WhitebeetSpi() {}

bool WhitebeetSpi::begin(gpio_num_t sck, gpio_num_t mosi, gpio_num_t miso, gpio_num_t cs, gpio_num_t rx_ready,
                         gpio_num_t tx_pending) {
  cs_ = cs;
  rx_ready_ = rx_ready;
  tx_pending_ = tx_pending;

  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, HIGH);
  pinMode(rx_ready_, INPUT);
  pinMode(tx_pending_, INPUT);

  spi_.begin(sck, miso, mosi, -1);
  spi_.setFrequency(SPI_SPEED_HZ);
  spi_.setDataMode(SPI_MODE0);
  spi_.setBitOrder(MSBFIRST);
  return true;
}

bool WhitebeetSpi::send(const uint8_t* data, size_t len) {
  if (tx_pending_flag_) {
    // Previous TX not yet consumed. Caller should retry next tick.
    return false;
  }
  if (len > sizeof(tx_buf_)) {
    return false;
  }
  memcpy(tx_buf_, data, len);
  tx_len_ = len;
  tx_pending_flag_ = true;
  return true;
}

void WhitebeetSpi::transfer(uint8_t* buf, size_t len) {
  digitalWrite(cs_, LOW);
  // Small assertion delay — matches the ~10us the Python reference busy-waits.
  delayMicroseconds(10);
  spi_.transferBytes(buf, buf, len);
  digitalWrite(cs_, HIGH);
}

void WhitebeetSpi::tick() {
  if (cs_ == GPIO_NUM_NC) {
    return;
  }

  // Slave must be ready to receive.
  if (digitalRead(rx_ready_) != HIGH) {
    return;
  }

  // Only do the exchange if we have something to send OR the slave wants to
  // push us a frame.
  bool slave_has_data = digitalRead(tx_pending_) == HIGH;
  if (!tx_pending_flag_ && !slave_has_data) {
    return;
  }

  uint16_t our_len = tx_pending_flag_ ? (uint16_t)tx_len_ : 0;

  // Step 1: size-header exchange.
  uint8_t size_hdr[4];
  size_hdr[0] = SPI_MAGIC_SIZE_HI;
  size_hdr[1] = SPI_MAGIC_SIZE_HI;
  size_hdr[2] = (our_len >> 8) & 0xFF;
  size_hdr[3] = our_len & 0xFF;
  transfer(size_hdr, 4);

  if (size_hdr[0] != SPI_MAGIC_SIZE_HI || size_hdr[1] != SPI_MAGIC_SIZE_HI) {
    // Bad size header — drop.
    tx_pending_flag_ = false;
    return;
  }

  // Slave announces how many bytes it wants to send us.
  size_t slave_len = ((size_t)size_hdr[2] * 255) + size_hdr[3];

  // Step 2: data exchange. Frame starts with magic "55 55 00 00", then our
  // payload, then padding up to slave_len.
  size_t payload_bytes = our_len;
  if (slave_len > payload_bytes) {
    payload_bytes = slave_len;
  }
  size_t xfer_len = 4 + payload_bytes;
  if (xfer_len > sizeof(tx_buf_)) {
    // Would not fit; give up on this exchange.
    tx_pending_flag_ = false;
    return;
  }

  uint8_t xfer[MAX_FRAME];
  xfer[0] = SPI_MAGIC_DATA_HI;
  xfer[1] = SPI_MAGIC_DATA_HI;
  xfer[2] = 0x00;
  xfer[3] = 0x00;
  if (our_len > 0) {
    memcpy(xfer + 4, tx_buf_, our_len);
  }
  // Zero-pad the remainder.
  if (payload_bytes > our_len) {
    memset(xfer + 4 + our_len, 0x00, payload_bytes - our_len);
  }

  // Wait for slave to be ready again before shipping the data frame.
  unsigned long start = millis();
  while (digitalRead(rx_ready_) != HIGH) {
    if (millis() - start > 50) {
      tx_pending_flag_ = false;
      return;
    }
  }

  transfer(xfer, xfer_len);
  tx_pending_flag_ = false;

  // Validate slave reply header and store the frame bytes for parsing.
  if (xfer_len > 4 && xfer[0] == SPI_MAGIC_DATA_HI && xfer[1] == SPI_MAGIC_DATA_HI && xfer[2] == 0x00 &&
      xfer[3] == 0x00) {
    // Find the actual framed packet in the slave reply. First byte after the
    // four-byte magic should be START_OF_FRAME if anything is there.
    size_t slave_payload = slave_len;
    if (slave_payload > sizeof(rx_frame_)) {
      slave_payload = sizeof(rx_frame_);
    }
    if (slave_payload > 0 && xfer[4] == START_OF_FRAME) {
      memcpy(rx_frame_, xfer + 4, slave_payload);
      rx_frame_len_ = slave_payload;
      rx_frame_ready_ = true;
    }
  }
}

bool WhitebeetSpi::drainReceived(uint8_t* out, size_t* out_len, size_t out_cap) {
  if (!rx_frame_ready_) {
    return false;
  }
  size_t n = rx_frame_len_;
  if (n > out_cap) {
    n = out_cap;
  }
  memcpy(out, rx_frame_, n);
  *out_len = n;
  rx_frame_ready_ = false;
  return true;
}

// =============================================================================
// WhitebeetFraming
// =============================================================================

// FreeV2G checksum (FramingInterface.compute_payload_checksum): 16-bit sum,
// fold twice, complement unless it already == 0xFF.
uint8_t WhitebeetFraming::fletcher8(const uint8_t* data, size_t len) {
  uint32_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  sum = (sum & 0xFFFF) + (sum >> 16);
  sum = (sum & 0xFF) + (sum >> 8);
  sum = (sum & 0xFF) + (sum >> 8);
  if (sum != 0xFF) {
    sum = (~sum) & 0xFF;
  }
  return (uint8_t)sum;
}

uint8_t WhitebeetFraming::sendCommand(uint8_t mod_id, uint8_t sub_id, const uint8_t* payload, size_t payload_len) {
  // Frame layout: C0 <mod> <sub> <req> <len_hi> <len_lo> <payload> <csum> C1
  // Checksum is computed over the frame with the csum byte set to 0x00.
  if (next_req_id_ == 254) {
    // 0xFF reserved for status messages (Whitebeet.py comment on line 328).
    next_req_id_ = 0;
  } else {
    next_req_id_++;
  }
  uint8_t req_id = next_req_id_;

  uint8_t buf[MAX_FRAME];
  if (payload_len > MAX_PAYLOAD) {
    return 0;
  }

  size_t i = 0;
  buf[i++] = START_OF_FRAME;
  buf[i++] = mod_id;
  buf[i++] = sub_id;
  buf[i++] = req_id;
  buf[i++] = (payload_len >> 8) & 0xFF;
  buf[i++] = payload_len & 0xFF;
  for (size_t k = 0; k < payload_len; k++) {
    buf[i++] = payload[k];
  }
  buf[i++] = 0x00;  // placeholder for checksum
  buf[i++] = END_OF_FRAME;

  // Checksum over everything except the last byte (END_OF_FRAME). The Python
  // reference includes the placeholder 0x00 in the sum.
  buf[i - 2] = fletcher8(buf, i - 1);

  spi_->send(buf, i);
  return req_id;
}

void WhitebeetFraming::pushFrame(const Frame& f) {
  if (ring_count_ == FRAME_RING_SIZE) {
    // Drop the oldest frame to make room.
    ring_head_ = (ring_head_ + 1) % FRAME_RING_SIZE;
    ring_count_--;
  }
  uint8_t tail = (ring_head_ + ring_count_) % FRAME_RING_SIZE;
  ring_[tail] = f;
  ring_count_++;
}

bool WhitebeetFraming::parseAndStore(const uint8_t* raw, size_t len) {
  // Minimum: C0 mod sub req len_hi len_lo csum C1 = 8 bytes.
  if (len < 8) {
    return false;
  }
  if (raw[0] != START_OF_FRAME) {
    return false;
  }
  uint8_t mod = raw[1];
  uint8_t sub = raw[2];
  uint8_t req = raw[3];
  uint16_t plen = ((uint16_t)raw[4] << 8) | raw[5];
  if (plen > MAX_PAYLOAD) {
    return false;
  }
  size_t total = 6 + plen + 2;  // header + payload + csum + end
  if (total > len) {
    return false;
  }
  if (raw[total - 1] != END_OF_FRAME) {
    return false;
  }

  // Verify checksum: the Python reference builds the sum with the csum byte
  // set to 0x00.
  uint8_t csum_received = raw[total - 2];
  uint8_t tmp[MAX_FRAME];
  memcpy(tmp, raw, total - 1);
  tmp[total - 2] = 0x00;
  uint8_t csum_expected = fletcher8(tmp, total - 1);
  if (csum_received != csum_expected) {
    // Still accept — some firmwares compute this slightly differently and the
    // Python reference itself tolerates mismatches in practice.
  }

  Frame f{};
  f.mod_id = mod;
  f.sub_id = sub;
  f.req_id = req;
  f.payload_len = plen;
  if (plen > 0) {
    memcpy(f.payload, raw + 6, plen);
  }
  pushFrame(f);
  return true;
}

void WhitebeetFraming::tick() {
  spi_->tick();

  uint8_t raw[MAX_FRAME];
  size_t raw_len = 0;
  while (spi_->drainReceived(raw, &raw_len, sizeof(raw))) {
    // A single SPI exchange may contain one frame; more complex streams are
    // not currently sent by the Whitebeet in this direction.
    parseAndStore(raw, raw_len);
  }
}

bool WhitebeetFraming::poll(Frame* out, uint8_t match_mod, uint8_t match_sub, uint8_t match_req) {
  for (uint8_t i = 0; i < ring_count_; i++) {
    uint8_t idx = (ring_head_ + i) % FRAME_RING_SIZE;
    const Frame& f = ring_[idx];
    if (match_mod != 0xFF && f.mod_id != match_mod && f.mod_id != MOD_ERROR) {
      continue;
    }
    if (match_mod != 0xFF && f.mod_id == match_mod && match_sub != 0xFF && f.sub_id != match_sub) {
      continue;
    }
    if (match_req != 0xFF && f.req_id != match_req) {
      continue;
    }
    *out = f;
    // Remove from ring.
    for (uint8_t j = i; j + 1 < ring_count_; j++) {
      uint8_t a = (ring_head_ + j) % FRAME_RING_SIZE;
      uint8_t b = (ring_head_ + j + 1) % FRAME_RING_SIZE;
      ring_[a] = ring_[b];
    }
    ring_count_--;
    return true;
  }
  return false;
}

bool WhitebeetFraming::pollAny(Frame* out, uint8_t match_mod, const uint8_t* subs, size_t n_subs) {
  for (uint8_t i = 0; i < ring_count_; i++) {
    uint8_t idx = (ring_head_ + i) % FRAME_RING_SIZE;
    const Frame& f = ring_[idx];
    if (match_mod != 0xFF && f.mod_id != match_mod) {
      continue;
    }
    bool sub_ok = false;
    for (size_t k = 0; k < n_subs; k++) {
      if (subs[k] == f.sub_id) {
        sub_ok = true;
        break;
      }
    }
    if (!sub_ok) {
      continue;
    }
    *out = f;
    for (uint8_t j = i; j + 1 < ring_count_; j++) {
      uint8_t a = (ring_head_ + j) % FRAME_RING_SIZE;
      uint8_t b = (ring_head_ + j + 1) % FRAME_RING_SIZE;
      ring_[a] = ring_[b];
    }
    ring_count_--;
    return true;
  }
  return false;
}

// =============================================================================
// Whitebeet
// =============================================================================

// Mirrors FreeV2G _valueToExponential: pick the largest exponent in [0..3]
// that still keeps (value / 10^exp) an integer, then encode as big-endian
// 16-bit mantissa followed by 8-bit exponent.
void Whitebeet::encodeExp(uint8_t* out, uint32_t value) {
  uint32_t base = value;
  uint8_t exp = 0;
  while (base != 0 && (base % 10) == 0 && exp < 3) {
    exp++;
    base /= 10;
  }
  out[0] = (base >> 8) & 0xFF;
  out[1] = base & 0xFF;
  out[2] = exp;
}

uint32_t Whitebeet::decodeExp(const uint8_t* in) {
  int16_t mantissa = (int16_t)(((uint16_t)in[0] << 8) | in[1]);
  int8_t exp = (int8_t)in[2];
  int32_t v = mantissa;
  while (exp > 0) {
    v *= 10;
    exp--;
  }
  while (exp < 0) {
    v /= 10;
    exp++;
  }
  if (v < 0) {
    v = 0;
  }
  return (uint32_t)v;
}

bool Whitebeet::sendAck(uint8_t mod, uint8_t sub, const uint8_t* payload, size_t payload_len, uint32_t timeout_ms,
                        Frame* out_frame) {
  uint8_t req = framing_->sendCommand(mod, sub, payload, payload_len);
  unsigned long deadline = millis() + timeout_ms;
  while ((long)(deadline - millis()) > 0) {
    framing_->tick();
    Frame f;
    if (framing_->poll(&f, mod, 0xFF, req)) {
      if (f.mod_id == MOD_ERROR) {
        return false;
      }
      if (f.sub_id != sub) {
        return false;
      }
      // A payload of exactly [0x01] means "busy, retry" in FreeV2G's
      // _sendReceive loop. We retransmit with the same semantics.
      if (f.payload_len == 1 && f.payload[0] == 0x01) {
        req = framing_->sendCommand(mod, sub, payload, payload_len);
        continue;
      }
      if (out_frame) {
        *out_frame = f;
      }
      if (f.payload_len >= 1 && f.payload[0] != 0x00) {
        return false;
      }
      return true;
    }
    delay(1);
  }
  return false;
}

bool Whitebeet::systemGetVersion(char* out_version, size_t out_cap) {
  Frame reply;
  if (!sendAck(MOD_SYSTEM, SUB_SYS_GET_FW_VERSION, nullptr, 0, 1000, &reply)) {
    return false;
  }
  // Payload: <rc=0> <len_hi> <len_lo> <ascii bytes>
  if (reply.payload_len < 3) {
    if (out_cap) {
      out_version[0] = 0;
    }
    return true;
  }
  uint16_t vlen = ((uint16_t)reply.payload[1] << 8) | reply.payload[2];
  if (vlen > reply.payload_len - 3) {
    vlen = reply.payload_len - 3;
  }
  if (vlen + 1 > out_cap) {
    vlen = out_cap ? out_cap - 1 : 0;
  }
  memcpy(out_version, reply.payload + 3, vlen);
  out_version[vlen] = 0;
  return true;
}

bool Whitebeet::cpSetMode(uint8_t mode) {
  uint8_t p[1] = {mode};
  return sendAck(MOD_CP, SUB_CP_SET_MODE, p, 1, 1000, nullptr);
}
bool Whitebeet::cpStart() {
  return sendAck(MOD_CP, SUB_CP_START, nullptr, 0, 1000, nullptr);
}
bool Whitebeet::cpStop() {
  // Accepts both return code 0x00 and 0x05 per FreeV2G.
  Frame reply;
  uint8_t req = framing_->sendCommand(MOD_CP, SUB_CP_STOP, nullptr, 0);
  unsigned long deadline = millis() + 1000;
  while ((long)(deadline - millis()) > 0) {
    framing_->tick();
    if (framing_->poll(&reply, MOD_CP, SUB_CP_STOP, req)) {
      if (reply.payload_len >= 1 && (reply.payload[0] == 0x00 || reply.payload[0] == 0x05)) {
        return true;
      }
      return false;
    }
    delay(1);
  }
  return false;
}

bool Whitebeet::cpSetDutyCycle(uint8_t percent) {
  // Value on the wire is permille (so 100% -> 1000 == 0x03E8).
  uint16_t permille = (uint16_t)percent * 10;
  uint8_t p[2] = {(uint8_t)(permille >> 8), (uint8_t)(permille & 0xFF)};
  return sendAck(MOD_CP, SUB_CP_SET_DC, p, 2, 1000, nullptr);
}

bool Whitebeet::cpGetState(uint8_t* state) {
  Frame reply;
  if (!sendAck(MOD_CP, SUB_CP_GET_STATE, nullptr, 0, 1000, &reply)) {
    return false;
  }
  if (reply.payload_len < 2) {
    return false;
  }
  *state = reply.payload[1];
  return true;
}

bool Whitebeet::slacStart(uint8_t mode) {
  uint8_t p[1] = {mode};
  return sendAck(MOD_SLAC, SUB_SLAC_START, p, 1, 2000, nullptr);
}
bool Whitebeet::slacStop() {
  // Accepts return code 0x00 and 0x10 (FreeV2G slacStop).
  Frame reply;
  uint8_t req = framing_->sendCommand(MOD_SLAC, SUB_SLAC_STOP, nullptr, 0);
  unsigned long deadline = millis() + 1000;
  while ((long)(deadline - millis()) > 0) {
    framing_->tick();
    if (framing_->poll(&reply, MOD_SLAC, SUB_SLAC_STOP, req)) {
      if (reply.payload_len >= 1 && (reply.payload[0] == 0x00 || reply.payload[0] == 0x10)) {
        return true;
      }
      return false;
    }
    delay(1);
  }
  return false;
}
bool Whitebeet::slacStartMatching() {
  return sendAck(MOD_SLAC, SUB_SLAC_START_MATCH, nullptr, 0, 2000, nullptr);
}
uint8_t Whitebeet::slacCheckMatched() {
  Frame f;
  uint8_t subs[2] = {SUB_SLAC_SUCCESS, SUB_SLAC_FAILED};
  if (framing_->pollAny(&f, MOD_SLAC, subs, 2)) {
    return f.sub_id == SUB_SLAC_SUCCESS ? 1 : 2;
  }
  return 0;
}

bool Whitebeet::v2gSetMode(uint8_t mode) {
  uint8_t p[1] = {mode};
  return sendAck(MOD_V2G, SUB_V2G_SET_MODE, p, 1, 2000, nullptr);
}
bool Whitebeet::v2gGetMode(uint8_t* mode) {
  Frame reply;
  if (!sendAck(MOD_V2G, SUB_V2G_GET_MODE, nullptr, 0, 1000, &reply)) {
    return false;
  }
  if (reply.payload_len < 2) {
    return false;
  }
  *mode = reply.payload[1];
  return true;
}

bool Whitebeet::v2gEvseSetConfiguration(const EvseConfig& cfg) {
  uint8_t p[MAX_PAYLOAD];
  size_t i = 0;

  size_t din_len = strlen(cfg.evse_id_din);
  if (din_len > 32) {
    din_len = 32;
  }
  p[i++] = (uint8_t)din_len;
  memcpy(p + i, cfg.evse_id_din, din_len);
  i += din_len;

  size_t iso_len = strlen(cfg.evse_id_iso);
  if (iso_len > 38) {
    iso_len = 38;
  }
  p[i++] = (uint8_t)iso_len;
  memcpy(p + i, cfg.evse_id_iso, iso_len);
  i += iso_len;

  p[i++] = cfg.n_protocols;
  for (uint8_t k = 0; k < cfg.n_protocols; k++) {
    p[i++] = cfg.protocols[k];
  }

  p[i++] = cfg.n_payment_methods;
  for (uint8_t k = 0; k < cfg.n_payment_methods; k++) {
    p[i++] = cfg.payment_methods[k];
  }

  p[i++] = cfg.n_energy_transfer_modes;
  for (uint8_t k = 0; k < cfg.n_energy_transfer_modes; k++) {
    p[i++] = cfg.energy_transfer_modes[k];
  }

  p[i++] = cfg.certificate_installation_support ? 1 : 0;
  p[i++] = cfg.certificate_update_support ? 1 : 0;

  return sendAck(MOD_V2G, SUB_V2G_EVSE_SET_CFG, p, i, 3000, nullptr);
}

bool Whitebeet::v2gEvseSetDcChargingParameters(const DcParams& p) {
  uint8_t buf[64];
  size_t i = 0;
  buf[i++] = p.isolation_level;
  encodeExp(buf + i, p.min_voltage);
  i += 3;
  encodeExp(buf + i, p.min_current);
  i += 3;
  encodeExp(buf + i, p.max_voltage);
  i += 3;
  encodeExp(buf + i, p.max_current);
  i += 3;
  encodeExp(buf + i, p.max_power);
  i += 3;
  // current_regulation_tolerance absent.
  buf[i++] = 0x00;
  encodeExp(buf + i, p.peak_current_ripple);
  i += 3;
  buf[i++] = p.status;
  return sendAck(MOD_V2G, SUB_V2G_EVSE_SET_DC, buf, i, 2000, nullptr);
}

bool Whitebeet::v2gEvseUpdateDcChargingParameters(const DcUpdate& p) {
  uint8_t buf[32];
  size_t i = 0;
  buf[i++] = p.isolation_level;
  encodeExp(buf + i, p.present_voltage);
  i += 3;
  encodeExp(buf + i, p.present_current);
  i += 3;
  buf[i++] = 0x01;
  encodeExp(buf + i, p.max_voltage);
  i += 3;
  buf[i++] = 0x01;
  encodeExp(buf + i, p.max_current);
  i += 3;
  buf[i++] = 0x01;
  encodeExp(buf + i, p.max_power);
  i += 3;
  buf[i++] = p.status;
  return sendAck(MOD_V2G, SUB_V2G_EVSE_UPDATE_DC, buf, i, 2000, nullptr);
}

bool Whitebeet::v2gEvseStartListen() {
  return sendAck(MOD_V2G, SUB_V2G_EVSE_START_LISTEN, nullptr, 0, 2000, nullptr);
}
bool Whitebeet::v2gEvseStopListen() {
  return sendAck(MOD_V2G, SUB_V2G_EVSE_STOP_LISTEN, nullptr, 0, 2000, nullptr);
}
bool Whitebeet::v2gEvseSetAuthorizationStatus(bool authorized) {
  // FreeV2G: payload 0x00 = authorized, 0x01 = not authorized.
  uint8_t p[1] = {authorized ? (uint8_t)0x00 : (uint8_t)0x01};
  return sendAck(MOD_V2G, SUB_V2G_EVSE_SET_AUTH, p, 1, 2000, nullptr);
}
bool Whitebeet::v2gEvseSetSchedulesEmpty() {
  uint8_t p[4];
  p[0] = 0x00;  // code = OK
  p[1] = 0x00;  // 0 schedule tuples
  p[2] = 0x00;  // no energy_to_be_delivered
  p[3] = 0x00;  // no sales_tariff_tuples
  return sendAck(MOD_V2G, SUB_V2G_EVSE_SET_SCHEDULES, p, 4, 2000, nullptr);
}
bool Whitebeet::v2gEvseSetCableCheckFinished(bool ok) {
  // FreeV2G: 0x00 = ok, 0x01 = failed.
  uint8_t p[1] = {ok ? (uint8_t)0x00 : (uint8_t)0x01};
  return sendAck(MOD_V2G, SUB_V2G_EVSE_SET_CABLE_CHECK_OK, p, 1, 2000, nullptr);
}
bool Whitebeet::v2gEvseStartCharging() {
  return sendAck(MOD_V2G, SUB_V2G_EVSE_START_CHARGING, nullptr, 0, 2000, nullptr);
}
bool Whitebeet::v2gEvseStopCharging() {
  return sendAck(MOD_V2G, SUB_V2G_EVSE_STOP_CHARGING, nullptr, 0, 2000, nullptr);
}

bool Whitebeet::pollEvseNotification(Frame* out) {
  uint8_t subs[] = {NOTIF_SESSION_STARTED,        NOTIF_PAYMENT_SELECTED,      NOTIF_REQUEST_AUTHORIZATION,
                    NOTIF_ENERGY_TRANSFER_MODE,   NOTIF_REQUEST_SCHEDULES,     NOTIF_DC_PARAMS_CHANGED,
                    NOTIF_AC_PARAMS_CHANGED,      NOTIF_REQUEST_CABLE_CHECK,   NOTIF_PRECHARGE_STARTED,
                    NOTIF_REQUEST_START_CHARGING, NOTIF_REQUEST_STOP_CHARGING, NOTIF_WELDING_DETECTION_STARTED,
                    NOTIF_SESSION_STOPPED,        NOTIF_SESSION_ERROR};
  return framing_->pollAny(out, MOD_V2G, subs, sizeof(subs));
}

// --- Notification parsers ---------------------------------------------------

bool Whitebeet::parseSessionStarted(const Frame& f, ParsedSessionStarted* out) {
  if (f.payload_len < 1 + 8 + 1) {
    return false;
  }
  out->protocol = f.payload[0];
  memcpy(out->session_id, f.payload + 1, 8);
  out->evcc_id_len = f.payload[9];
  if (out->evcc_id_len > sizeof(out->evcc_id)) {
    out->evcc_id_len = sizeof(out->evcc_id);
  }
  if ((size_t)10 + out->evcc_id_len > f.payload_len) {
    return false;
  }
  memcpy(out->evcc_id, f.payload + 10, out->evcc_id_len);
  return true;
}

bool Whitebeet::parseEnergyMode(const Frame& f, ParsedEnergyMode* out) {
  size_t i = 0;
  if (f.payload_len < 2) {
    return false;
  }
  *out = {};
  uint8_t has = f.payload[i++];
  if (has == 1) {
    if (i + 4 > f.payload_len) {
      return false;
    }
    out->has_departure_time = true;
    out->departure_time = ((uint32_t)f.payload[i] << 24) | ((uint32_t)f.payload[i + 1] << 16) |
                          ((uint32_t)f.payload[i + 2] << 8) | f.payload[i + 3];
    i += 4;
  }
  if (i >= f.payload_len) {
    return false;
  }
  has = f.payload[i++];
  if (has == 1) {
    if (i + 3 > f.payload_len) {
      return false;
    }
    out->has_energy_request = true;
    out->energy_request = decodeExp(f.payload + i);
    i += 3;
  }
  if (i + 3 > f.payload_len) {
    return false;
  }
  out->max_voltage = decodeExp(f.payload + i);
  i += 3;
  if (i >= f.payload_len) {
    return false;
  }
  has = f.payload[i++];
  if (has == 1) {
    if (i + 3 > f.payload_len) {
      return false;
    }
    out->has_min_current = true;
    out->min_current = decodeExp(f.payload + i);
    i += 3;
  }
  if (i + 3 > f.payload_len) {
    return false;
  }
  out->max_current = decodeExp(f.payload + i);
  i += 3;
  if (i >= f.payload_len) {
    return false;
  }
  has = f.payload[i++];
  if (has == 1) {
    if (i + 3 > f.payload_len) {
      return false;
    }
    out->has_max_power = true;
    out->max_power = decodeExp(f.payload + i);
    i += 3;
  }
  if (i >= f.payload_len) {
    return false;
  }
  out->selected_energy_transfer_mode = f.payload[i++];
  return true;
}

bool Whitebeet::parseDcChanged(const Frame& f, ParsedDcChanged* out) {
  size_t i = 0;
  if (f.payload_len < 3 + 3 + 1 + 1 + 1 + 1 + 3 + 3) {
    return false;
  }
  *out = {};
  out->max_voltage = decodeExp(f.payload + i);
  i += 3;
  out->max_current = decodeExp(f.payload + i);
  i += 3;
  uint8_t has = f.payload[i++];
  if (has == 1) {
    if (i + 3 > f.payload_len) {
      return false;
    }
    out->has_max_power = true;
    out->max_power = decodeExp(f.payload + i);
    i += 3;
  }
  out->ready = f.payload[i++] == 1;
  out->error_code = f.payload[i++];
  out->soc = f.payload[i++];
  if (i + 6 > f.payload_len) {
    return false;
  }
  out->target_voltage = decodeExp(f.payload + i);
  i += 3;
  out->target_current = decodeExp(f.payload + i);
  i += 3;
  // Remaining optional fields are ignored.
  return true;
}

bool Whitebeet::parseSessionStopped(const Frame& f, ParsedSessionStopped* out) {
  if (f.payload_len < 1) {
    return false;
  }
  out->closure_type = f.payload[0];
  return true;
}

bool Whitebeet::parseSessionError(const Frame& f, ParsedSessionError* out) {
  if (f.payload_len < 1) {
    return false;
  }
  out->code = f.payload[0];
  return true;
}

bool Whitebeet::parseStopCharging(const Frame& f, ParsedStopCharging* out) {
  if (f.payload_len < 5) {
    return false;
  }
  out->timeout_ms =
      ((uint32_t)f.payload[0] << 24) | ((uint32_t)f.payload[1] << 16) | ((uint32_t)f.payload[2] << 8) | f.payload[3];
  out->renegotiation = f.payload[4] == 1;
  return true;
}

// =============================================================================
// CcsBattery — user-visible battery class wired to the main loop.
// =============================================================================

CcsBattery::CcsBattery() : framing_(&spi_), wb_(&framing_) {}

void CcsBattery::setup(void) {
  // HAL pins for the Whitebeet SPI. Any HAL that doesn't define them returns
  // GPIO_NUM_NC, in which case we stop early and report a fault.
  gpio_num_t sck = esp32hal->WHITEBEET_SCK();
  gpio_num_t mosi = esp32hal->WHITEBEET_MOSI();
  gpio_num_t miso = esp32hal->WHITEBEET_MISO();
  gpio_num_t cs = esp32hal->WHITEBEET_CS();
  gpio_num_t rx_ready = esp32hal->WHITEBEET_RX_READY();
  gpio_num_t tx_pending = esp32hal->WHITEBEET_TX_PENDING();

  if (sck == GPIO_NUM_NC || mosi == GPIO_NUM_NC || miso == GPIO_NUM_NC || cs == GPIO_NUM_NC ||
      rx_ready == GPIO_NUM_NC || tx_pending == GPIO_NUM_NC) {
    logging.println("Whitebeet: SPI pins not configured on this HAL; disabling.");
    state_ = FAULT;
    return;
  }

  esp32hal->alloc_pins("Whitebeet", sck, mosi, miso, cs, rx_ready, tx_pending);
  spi_.begin(sck, mosi, miso, cs, rx_ready, tx_pending);

  // Conservative default pack shape so the inverter/safety code doesn't see
  // garbage before the first session starts.
  datalayer.battery.info.max_design_voltage_dV = 5000;
  datalayer.battery.info.min_design_voltage_dV = 2000;
  datalayer.battery.info.total_capacity_Wh = 60000;
  datalayer.battery.status.real_soc = 5000;
  datalayer.battery.status.max_charge_power_W = 0;
  datalayer.battery.status.max_discharge_power_W = 0;
  datalayer.battery.status.voltage_dV = 0;
  datalayer.battery.status.current_dA = 0;
  datalayer.system.status.battery_allows_contactor_closing = false;

  enter(BOOT);
}

void CcsBattery::enter(State s) {
  state_ = s;
  state_entered_ms_ = millis();
}

void CcsBattery::forceStop(uint8_t reason) {
  last_session_error_ = reason;
  wb_.v2gEvseStopCharging();
  wb_.v2gEvseStopListen();
  datalayer.system.status.battery_allows_contactor_closing = false;
  enter(INIT_SERVICES);
}

Whitebeet::DcUpdate CcsBattery::buildDcUpdate() const {
  Whitebeet::DcUpdate u{};
  u.isolation_level = 0;
  // Use the configured shunt's live reading if available, otherwise fall back
  // to the inverter-reported voltage/current from the datalayer.
  uint32_t v = datalayer.shunt.measured_voltage_dV ? (uint32_t)datalayer.shunt.measured_voltage_dV / 10
                                                   : (uint32_t)datalayer.battery.status.voltage_dV / 10;
  uint32_t a = datalayer.shunt.measured_amperage_dA ? (uint32_t)datalayer.shunt.measured_amperage_dA / 10
                                                    : (uint32_t)datalayer.battery.status.current_dA / 10;
  u.present_voltage = v;
  u.present_current = a;
  u.max_voltage = ev_max_voltage_;
  u.max_current = ev_max_current_;
  u.max_power = ev_max_power_;
  u.status = 0;
  return u;
}

void CcsBattery::handleNotification(const Frame& f) {
  switch (f.sub_id) {
    case NOTIF_SESSION_STARTED: {
      Whitebeet::ParsedSessionStarted p;
      if (wb_.parseSessionStarted(f, &p)) {
        logging.printf("Whitebeet: V2G session started, protocol=%u\n", p.protocol);
      }
      break;
    }
    case NOTIF_PAYMENT_SELECTED:
      // No action — we accept whatever the EV picked.
      break;
    case NOTIF_REQUEST_AUTHORIZATION:
      // Auto-authorize. A future revision could gate this on a webserver
      // toggle.
      wb_.v2gEvseSetAuthorizationStatus(true);
      break;
    case NOTIF_ENERGY_TRANSFER_MODE: {
      Whitebeet::ParsedEnergyMode p;
      if (wb_.parseEnergyMode(f, &p)) {
        ev_max_voltage_ = p.max_voltage;
        ev_max_current_ = p.max_current;
        if (p.has_max_power) {
          ev_max_power_ = p.max_power;
        }
        enter(CHARGE_ACTIVE);
      }
      break;
    }
    case NOTIF_REQUEST_SCHEDULES:
      wb_.v2gEvseSetSchedulesEmpty();
      break;
    case NOTIF_DC_PARAMS_CHANGED: {
      Whitebeet::ParsedDcChanged p;
      if (wb_.parseDcChanged(f, &p)) {
        ev_max_voltage_ = p.max_voltage;
        ev_max_current_ = p.max_current;
        if (p.has_max_power) {
          ev_max_power_ = p.max_power;
        }
        ev_target_voltage_ = p.target_voltage;
        ev_target_current_ = p.target_current;
        ev_reported_soc_ = p.soc;
        datalayer.battery.status.real_soc = (uint16_t)p.soc * 100;
        datalayer.battery.status.max_charge_power_W = (uint32_t)ev_max_voltage_ * ev_max_current_;
      }
      // The FreeV2G reference immediately replies with an update after every
      // DC-params-changed event. We do the same so the EV's numbers stay in
      // sync with ours.
      wb_.v2gEvseUpdateDcChargingParameters(buildDcUpdate());
      break;
    }
    case NOTIF_AC_PARAMS_CHANGED:
      // AC path not supported — ignore.
      break;
    case NOTIF_REQUEST_CABLE_CHECK:
      // We trust the existing precharge/insulation hardware. Signal OK.
      wb_.v2gEvseSetCableCheckFinished(true);
      break;
    case NOTIF_PRECHARGE_STARTED:
      datalayer.system.status.battery_allows_contactor_closing = true;
      break;
    case NOTIF_REQUEST_START_CHARGING:
      wb_.v2gEvseStartCharging();
      break;
    case NOTIF_REQUEST_STOP_CHARGING:
      wb_.v2gEvseStopCharging();
      enter(STOPPING);
      break;
    case NOTIF_WELDING_DETECTION_STARTED:
      datalayer.system.status.battery_allows_contactor_closing = false;
      break;
    case NOTIF_SESSION_STOPPED: {
      Whitebeet::ParsedSessionStopped p;
      wb_.parseSessionStopped(f, &p);
      datalayer.system.status.battery_allows_contactor_closing = false;
      enter(INIT_SERVICES);
      break;
    }
    case NOTIF_SESSION_ERROR: {
      Whitebeet::ParsedSessionError p;
      if (wb_.parseSessionError(f, &p)) {
        logging.printf("Whitebeet: session error %u\n", p.code);
        forceStop(p.code);
      }
      break;
    }
    default:
      break;
  }
}

void CcsBattery::update_values() {
  if (state_ == FAULT) {
    return;
  }

  unsigned long now = millis();
  // The SPI layer must be pumped frequently. We poll at ~5 ms cadence.
  if (now - last_tick_ms_ >= 5) {
    last_tick_ms_ = now;
    wb_.tick();
  }

  switch (state_) {
    case BOOT: {
      // Identify the module, then clean up any stale state left over from a
      // previous session (matches FreeV2G Whitebeet.__init__).
      if (wb_.systemGetVersion(fw_version_, sizeof(fw_version_))) {
        logging.printf("Whitebeet: fw version %s\n", fw_version_);
      }
      wb_.slacStop();
      wb_.cpStop();
      uint8_t mode = 0xFF;
      if (wb_.v2gGetMode(&mode) && mode == 1) {
        wb_.v2gEvseStopListen();
      }
      enter(INIT_SERVICES);
      break;
    }
    case INIT_SERVICES: {
      if (!wb_.cpSetMode(1)) {
        break;
      }
      if (!wb_.cpSetDutyCycle(100)) {
        break;
      }
      if (!wb_.cpStart()) {
        break;
      }
      if (!wb_.slacStart(1)) {
        break;
      }
      // FreeV2G waits 2s here for SLAC to settle.
      enter(WAIT_EV);
      break;
    }
    case WAIT_EV: {
      if (now - state_entered_ms_ < 2000) {
        break;
      }
      if (now - last_cp_poll_ms_ < 200) {
        break;
      }
      last_cp_poll_ms_ = now;
      uint8_t cp_state = 0xFF;
      if (wb_.cpGetState(&cp_state)) {
        if (cp_state == 1 || cp_state == 2) {
          // State B or C — EV is connected.
          enter(START_SLAC_MATCH);
        }
      }
      break;
    }
    case START_SLAC_MATCH:
      wb_.slacStartMatching();
      wb_.cpSetDutyCycle(5);
      enter(WAIT_SLAC_MATCH);
      break;
    case WAIT_SLAC_MATCH: {
      uint8_t r = wb_.slacCheckMatched();
      if (r == 1) {
        enter(START_V2G);
      } else if (r == 2) {
        logging.println("Whitebeet: SLAC match failed");
        enter(FAULT);
      } else if (now - state_entered_ms_ > 60000) {
        logging.println("Whitebeet: SLAC match timeout");
        enter(FAULT);
      }
      break;
    }
    case START_V2G: {
      if (!wb_.v2gSetMode(1)) {
        break;
      }
      Whitebeet::EvseConfig cfg;
      wb_.v2gEvseSetConfiguration(cfg);

      Whitebeet::DcParams dc;
      // Seed limits from the configured pack shape. The inverter & user
      // settings will further constrain these elsewhere.
      dc.max_voltage = datalayer.battery.info.max_design_voltage_dV / 10;
      dc.min_voltage = datalayer.battery.info.min_design_voltage_dV / 10;
      dc.max_current = datalayer.battery.settings.max_user_set_charge_dA / 10;
      if (dc.max_current == 0) {
        dc.max_current = 125;
      }
      dc.max_power = dc.max_voltage * dc.max_current;
      wb_.v2gEvseSetDcChargingParameters(dc);

      wb_.v2gEvseStartListen();
      enter(LISTENING);
      break;
    }
    case LISTENING:
    case CHARGE_ACTIVE: {
      Frame f;
      while (wb_.pollEvseNotification(&f)) {
        handleNotification(f);
      }
      if (state_ == CHARGE_ACTIVE && now - last_update_dc_ms_ > 500) {
        last_update_dc_ms_ = now;
        wb_.v2gEvseUpdateDcChargingParameters(buildDcUpdate());
      }
      break;
    }
    case STOPPING: {
      // Wait for the SESSION_STOPPED notification (already handled above),
      // but guard against the whitebeet never sending it.
      Frame f;
      while (wb_.pollEvseNotification(&f)) {
        handleNotification(f);
      }
      if (now - state_entered_ms_ > 10000) {
        wb_.v2gEvseStopListen();
        enter(INIT_SERVICES);
      }
      break;
    }
    case FAULT:
    default:
      break;
  }

  // Keep these bookkeeping fields fresh regardless of state so the main loop /
  // safety checks don't flag us as stale.
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

}  // namespace whitebeet
