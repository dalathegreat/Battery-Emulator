#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ringbuf.h"

#if __has_include("esp_attr.h")
#include "esp_attr.h"
#endif
#ifndef DRAM_ATTR
#define DRAM_ATTR  // Host builds
#endif

/* Pure helpers for MCP2518FD_Lite. These have no ESP-IDF dependencies, so the
host unit tests (in test/) can use them directly.
*/

static inline uint32_t readLE32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void writeLE32(uint8_t* p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

// Payload bytes for each DLC code
DRAM_ATTR static const uint8_t kDlcToBytes[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

// Calculate the smallest DLC code whose payload holds `length` bytes, so
// lengths between the CAN FD sizes are rounded up (and zero padded by the
// caller).
static inline uint32_t lengthCodeForLength(uint8_t length) {
  if (length <= 8) {
    return length;
  }
  if (length <= 24) {
    return (length + 3) / 4 + 6;  // 12, 16, 20, 24 -> 9..12
  }
  return length <= 32 ? 13 : (length <= 48 ? 14 : 15);
}

// Calculate the bit timing for an 80% sample point, SJW = TSEG2, and one
// prescaler BRP = 2^k - 1 for both phases (equal BRPs as CiA 601-3 recommends,
// powers of two divide 20/40 MHz exactly at all the standard bitrates).

// Tq per bit at BRP = 2^k - 1, rounded.
static inline uint32_t tqPerBit(uint32_t sysClock, uint32_t bitrate, uint32_t k) {
  return ((sysClock >> k) + bitrate / 2) / bitrate;
}

// Smallest k at which a bit is at most maxTq Tq.
static inline uint32_t prescalerShift(uint32_t sysClock, uint32_t bitrate, uint32_t maxTq) {
  uint32_t k = 0;
  while (tqPerBit(sysClock, bitrate, k) > maxTq) {
    k++;
  }
  return k;
}

// CiNBTCFG/CiDBTCFG image for an n Tq bit: BRP[31:24] TSEG1[23:16]
// TSEG2[14:8] SJW[6:0], each stored as value-1. CiDBTCFG's fields start at
// the same bits but are narrower (TSEG1[20:16] TSEG2[11:8] SJW[3:0]); the
// 40 Tq data limit keeps the values within them.
static inline uint32_t encodeBitTiming(uint32_t n, uint32_t k) {
  const uint32_t tseg2 = (n * 205 - 614) >> 10;  // round(n/5) - 1 (20%)
  const uint32_t tseg1 = n - 3 - tseg2;          // n - SYNC - TSEG2 - 1
  return ((((1u << k) - 1) << 8 | tseg1) << 8 | tseg2) << 8 | tseg2;
}

// Fills regs with CiNBTCFG, CiDBTCFG and CiTDC, which are contiguous in chip
// address order (0x004, 0x008, 0x00C). sysClock: 20000000 or 40000000.
// Bitrates in bit/s.
static inline bool calculateBitTiming(uint32_t sysClock, uint32_t arbBitrate, uint32_t dataBitrate, uint32_t regs[3]) {
  // At 80%: nominal TSEG1 <= 256 Tq -> 320 Tq/bit, data TSEG1 <= 32 -> 40.
  uint32_t k = prescalerShift(sysClock, arbBitrate, 320);
  const uint32_t kData = prescalerShift(sysClock, dataBitrate, 40);
  if (kData > k) {
    k = kData;
  }
  const uint32_t nArb = tqPerBit(sysClock, arbBitrate, k);
  const uint32_t nData = tqPerBit(sysClock, dataBitrate, k);
  // BRP is 8 bits, and below 4 Tq per bit the segments don't fit
  if (k > 8 || nArb < 4 || nData < 4) {
    return false;
  }
  regs[0] = encodeBitTiming(nArb, k);
  regs[1] = encodeBitTiming(nData, k);
  uint32_t tdc = 1u << 25;  // EDGFLTEN: edge filtering during bus integration
  if (k == 0) {
    // Auto TDC (TDCMOD = 10, bits 17:16), TDCO = data sample point in
    // SYSCLKs = SYNC + TSEG1 = TSEG1 field + 2. With BRP = 0, regs[1] >> 16
    // is just the TSEG1 field.
    tdc |= 2u << 16 | ((regs[1] >> 16) + 2) << 8;
  }
  regs[2] = tdc;
  return true;
}

// The controller keeps SID in ID bits 0-10 and EID in 11-28 (for an extended
// ID, SID is its top 11 bits).
static inline uint32_t chipIdFromCanId(uint32_t id, bool ext) {
  return ext ? ((id >> 18) & 0x7FF) | ((id & 0x3FFFF) << 11) : id & 0x7FF;
}

static inline uint32_t canIdFromChipId(uint32_t raw, bool ext) {
  return ext ? ((raw >> 11) & 0x3FFFF) | ((raw & 0x7FF) << 18) : raw & 0x7FF;
}

// A command for `addr` into p[0..1]
static inline void cmd16(uint8_t* p, uint16_t instruction, uint16_t addr) {
  const uint16_t c = instruction | (addr & 0x0FFF);
  p[0] = c >> 8;
  p[1] = c;
}

// A frame as stored in the rings, although we only store the actual payload needed.
struct MCP2518FD_Lite_RingRecord {
  uint32_t id;
  uint8_t len;    // Payload bytes (0-64)
  uint8_t flags;  // EXT, FD
  uint8_t reserved[2];
  uint8_t payload[64];

  static constexpr uint8_t EXT = 0x01;
  static constexpr uint8_t FD = 0x02;
  static constexpr size_t HEADER_LEN = 8;
};

// Single producer, single consumer. Always inlined, so step()'s uses stay in
// IRAM with it.
template <size_t Size>
struct MCP2518FD_Lite_RecordRing {
  typedef MCP2518FD_Lite_RingRecord RingRecord;
  static constexpr size_t RECORD_HEADER_LEN = RingRecord::HEADER_LEN;

  struct ringbuf rb;
  alignas(uint32_t) uint8_t storage[Size];
  MCP2518FD_Lite_RecordRing() { ringbuf_init(&rb, storage, Size); }

  // Add a record to the ring buffer. Returns false if there isn't enough room.
  __attribute__((always_inline)) bool write(const RingRecord& r) {
    return ringbuf_write(&rb, reinterpret_cast<const uint8_t*>(&r), RECORD_HEADER_LEN + r.len) != 0;
  }
  // Check if the ring buffer is empty.
  __attribute__((always_inline)) bool empty() { return Size - ringbuf_free_space(&rb) < RECORD_HEADER_LEN; }
  // Read a record from the ring buffer. Returns false if the buffer is empty.
  __attribute__((always_inline)) bool read(RingRecord& r) {
    if (empty()) {
      return false;
    }
    ringbuf_read(&rb, reinterpret_cast<uint8_t*>(&r), RECORD_HEADER_LEN);
    ringbuf_read(&rb, r.payload, r.len);
    return true;
  }
};
