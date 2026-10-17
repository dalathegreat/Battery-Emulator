#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <vector>

#include "../Software/src/lib/mcp2515_lite/async_spi_clock.h"
#include "../Software/src/lib/mcp2515_lite/mcp2515_lite_util.h"
#include "../Software/src/lib/mcp2518fd_lite/mcp2518fd_lite_util.h"

// Host tests for the hardware-independent parts of the MCP2515_Lite and
// MCP2518FD_Lite drivers: bit timing, ID packing, ring buffers and the SPI
// clock divider. The SPI/interrupt state machines need real hardware.

// The speeds comm_can.h offers (CAN_Speed)
static const uint32_t kBitrates[] = {100000, 125000, 200000, 250000, 500000, 800000, 1000000};

// --- MCP2515 bit timing ---

struct Mcp2515Timing {
  uint32_t brp, prseg, phseg1, phseg2, sjw, tq;
};

static Mcp2515Timing decodeMcp2515(const uint8_t cnf[3]) {
  Mcp2515Timing t;
  t.sjw = (cnf[0] >> 6) + 1;
  t.brp = (cnf[0] & 0x3F) + 1;
  t.phseg1 = ((cnf[1] >> 3) & 7) + 1;
  t.prseg = (cnf[1] & 7) + 1;
  t.phseg2 = (cnf[2] & 7) + 1;
  t.tq = 1 + t.prseg + t.phseg1 + t.phseg2;
  return t;
}

TEST(Mcp2515BitTimingTest, StandardRatesAreExact) {
  for (uint32_t f_osc : {8000000u, 16000000u}) {
    for (uint32_t rate : kBitrates) {
      if (f_osc == 8000000 && rate >= 800000) {
        continue;  // Impossible, see below
      }
      SCOPED_TRACE(testing::Message() << f_osc << "Hz @ " << rate);
      uint8_t cnf[3];
      ASSERT_TRUE(calculateMCP2515Config(f_osc, rate, cnf));
      const Mcp2515Timing t = decodeMcp2515(cnf);

      // TQ = 2 * BRP / f_osc
      EXPECT_EQ(f_osc % (2 * t.brp * t.tq), 0u);
      EXPECT_EQ(f_osc / (2 * t.brp * t.tq), rate);
      // ~75% sample point (exactly, at 8 and 16 Tq)
      const double sp = 100.0 * (1 + t.prseg + t.phseg1) / t.tq;
      EXPECT_GE(sp, 70.0);
      EXPECT_LE(sp, 85.0);
      if (t.tq % 4 == 0) {
        EXPECT_EQ(sp, 75.0);
      }

      // Datasheet constraints
      EXPECT_TRUE(cnf[1] & 0x80);  // BTLMODE: PHSEG2 from CNF3
      EXPECT_EQ(cnf[2] & 0xF8, 0);
      EXPECT_GE(t.tq, 8u);
      EXPECT_LE(t.tq, 25u);
      EXPECT_GE(t.phseg2, 2u);
      EXPECT_GE(t.prseg + t.phseg1, t.phseg2);
      EXPECT_GE(t.phseg2, t.sjw);
    }
  }
}

// The values the driver has always used for the common speeds
TEST(Mcp2515BitTimingTest, KnownRegisterValues) {
  uint8_t cnf[3];
  ASSERT_TRUE(calculateMCP2515Config(16000000, 500000, cnf));
  EXPECT_EQ(cnf[0], 0x00);
  EXPECT_EQ(cnf[1], 0xA5);
  EXPECT_EQ(cnf[2], 0x03);
  ASSERT_TRUE(calculateMCP2515Config(16000000, 1000000, cnf));
  EXPECT_EQ(cnf[0], 0x00);
  EXPECT_EQ(cnf[1], 0x8A);
  EXPECT_EQ(cnf[2], 0x01);
  ASSERT_TRUE(calculateMCP2515Config(8000000, 125000, cnf));
  EXPECT_EQ(cnf[0], 0x01);
  EXPECT_EQ(cnf[1], 0xA5);
  EXPECT_EQ(cnf[2], 0x03);
}

TEST(Mcp2515BitTimingTest, InvalidArgumentsAreRejected) {
  uint8_t cnf[3];
  EXPECT_FALSE(calculateMCP2515Config(16000000, 0, cnf));
  EXPECT_FALSE(calculateMCP2515Config(0, 500000, cnf));
  EXPECT_FALSE(calculateMCP2515Config(16000000, 500000, nullptr));
  // Would need fewer than 8 Tq per bit
  EXPECT_FALSE(calculateMCP2515Config(8000000, 800000, cnf));
  EXPECT_FALSE(calculateMCP2515Config(8000000, 1000000, cnf));
  // Not a whole number of Tq
  EXPECT_FALSE(calculateMCP2515Config(16000000, 333333, cnf));
}

// --- MCP2515 ID packing ---

static const uint32_t kExtIds[] = {0,       1,          0x7FF,      0x800,      0x3FFFF,
                                   0x40000, 0x12345678, 0x0AAAAAAA, 0x15555555, 0x1FFFFFFF};

TEST(Mcp2515IdTest, StandardIdsRoundTrip) {
  for (uint32_t id = 0; id <= 0x7FF; id++) {
    uint8_t buf[2];
    packStandardId(buf, id);
    EXPECT_EQ(buf[1] & 0x1F, 0) << id;  // EXIDE clear, and nothing else in SIDL
    EXPECT_EQ(unpackStandardId(buf), id);
  }
}

TEST(Mcp2515IdTest, ExtendedIdsRoundTrip) {
  for (uint32_t id : kExtIds) {
    uint8_t buf[4];
    packExtendedId(buf, id);
    EXPECT_EQ(buf[1] & 0x1C, 0x08) << id;  // EXIDE set
    EXPECT_EQ(unpackExtendedId(buf), id);
    // The chip puts the top 11 bits of an extended ID in SIDH/SIDL
    EXPECT_EQ(unpackStandardId(buf), id >> 18);
  }
}

TEST(Mcp2515IdTest, KnownRegisterLayout) {
  uint8_t buf[4];
  packStandardId(buf, 0x7FF);
  EXPECT_EQ(buf[0], 0xFF);
  EXPECT_EQ(buf[1], 0xE0);
  packExtendedId(buf, 0x1FFFFFFF);
  EXPECT_EQ(buf[0], 0xFF);
  EXPECT_EQ(buf[1], 0xEB);
  EXPECT_EQ(buf[2], 0xFF);
  EXPECT_EQ(buf[3], 0xFF);
}

// --- MCP2518FD bit timing ---

struct FdTiming {
  uint32_t brp, tseg1, tseg2, sjw, tq;
};

static FdTiming decodeFd(uint32_t reg) {
  FdTiming t;
  t.brp = (reg >> 24) + 1;
  t.tseg1 = ((reg >> 16) & 0xFF) + 1;
  t.tseg2 = ((reg >> 8) & 0x7F) + 1;
  t.sjw = (reg & 0x7F) + 1;
  t.tq = 1 + t.tseg1 + t.tseg2;
  return t;
}

static void checkFdPhase(uint32_t sysClock, uint32_t bitrate, const FdTiming& t) {
  EXPECT_EQ(sysClock % (t.brp * t.tq), 0u);
  EXPECT_EQ(sysClock / (t.brp * t.tq), bitrate);
  // ~80% sample point (exactly 80% when the Tq count is a multiple of 5)
  const double sp = 100.0 * (1 + t.tseg1) / t.tq;
  EXPECT_GE(sp, 70.0);
  EXPECT_LE(sp, 85.0);
  if (t.tq % 5 == 0) {
    EXPECT_EQ(sp, 80.0);
  }
  EXPECT_EQ(t.sjw, t.tseg2);
}

TEST(Mcp2518fdBitTimingTest, StandardRatesAreExact) {
  for (uint32_t sysClock : {20000000u, 40000000u}) {
    for (uint32_t arb : {125000u, 250000u, 500000u, 1000000u}) {
      for (uint32_t factor : {1u, 2u, 4u, 8u}) {
        const uint32_t data = arb * factor;
        if (sysClock / data < 4) {
          continue;  // Too fast, see below
        }
        SCOPED_TRACE(testing::Message() << sysClock << "Hz @ " << arb << "/" << data);
        uint32_t regs[3];
        ASSERT_TRUE(calculateBitTiming(sysClock, arb, data, regs));
        const FdTiming n = decodeFd(regs[0]);
        const FdTiming d = decodeFd(regs[1]);
        checkFdPhase(sysClock, arb, n);
        checkFdPhase(sysClock, data, d);

        // Same prescaler for both phases
        EXPECT_EQ(n.brp, d.brp);
        // CiNBTCFG reserved bits, then CiDBTCFG's narrower fields
        EXPECT_EQ(regs[0] & 0x00008080u, 0u);
        EXPECT_EQ(regs[1] & 0x00E0F0F0u, 0u);

        // CiTDC: edge filtering always; auto TDC with the offset at the data
        // sample point (in SYSCLKs) when BRP is 1
        if (d.brp == 1) {
          EXPECT_EQ(regs[2], (1u << 25) | (2u << 16) | ((1 + d.tseg1) << 8));
        } else {
          EXPECT_EQ(regs[2], 1u << 25);
        }
      }
    }
  }
}

TEST(Mcp2518fdBitTimingTest, ImpossibleRatesAreRejected) {
  uint32_t regs[3];
  EXPECT_FALSE(calculateBitTiming(20000000, 500000, 8000000, regs));  // 2.5 Tq data bit
  EXPECT_FALSE(calculateBitTiming(40000000, 1000000, 20000000, regs));
  EXPECT_FALSE(calculateBitTiming(40000000, 100, 100, regs));  // Prescaler too large
}

// --- MCP2518FD frame encoding ---

TEST(Mcp2518fdFrameTest, LengthCodeIsTheSmallestThatFits) {
  for (uint32_t len = 0; len <= 64; len++) {
    const uint32_t code = lengthCodeForLength(len);
    ASSERT_LT(code, 16u);
    EXPECT_GE(kDlcToBytes[code], len) << len;
    if (code > 0) {
      EXPECT_LT(kDlcToBytes[code - 1], len) << len;
    }
  }
  for (uint32_t code = 0; code < 16; code++) {
    EXPECT_EQ(lengthCodeForLength(kDlcToBytes[code]), code);
  }
}

TEST(Mcp2518fdFrameTest, IdsRoundTrip) {
  for (uint32_t id = 0; id <= 0x7FF; id++) {
    EXPECT_EQ(chipIdFromCanId(id, false), id);
    EXPECT_EQ(canIdFromChipId(chipIdFromCanId(id, false), false), id);
  }
  for (uint32_t id : kExtIds) {
    const uint32_t chip = chipIdFromCanId(id, true);
    EXPECT_EQ(chip >> 29, 0u);
    EXPECT_EQ(canIdFromChipId(chip, true), id);
  }
}

TEST(Mcp2518fdFrameTest, KnownIdLayout) {
  // SID (the top 11 bits of an extended ID) in bits 0-10, EID in 11-28
  EXPECT_EQ(chipIdFromCanId(0x1FFC0000, true), 0x7FFu);
  EXPECT_EQ(chipIdFromCanId(0x0003FFFF, true), 0x3FFFFu << 11);
  EXPECT_EQ(chipIdFromCanId(0x12345678, true), 0x48Du | (0x05678u << 11));
  // Standard IDs are masked to 11 bits
  EXPECT_EQ(chipIdFromCanId(0xFFFF, false), 0x7FFu);
}

TEST(Mcp2518fdFrameTest, LittleEndianWordsAndCommands) {
  uint8_t buf[4];
  writeLE32(buf, 0x12345678);
  EXPECT_EQ(buf[0], 0x78);
  EXPECT_EQ(buf[1], 0x56);
  EXPECT_EQ(buf[2], 0x34);
  EXPECT_EQ(buf[3], 0x12);
  EXPECT_EQ(readLE32(buf), 0x12345678u);
  writeLE32(buf, 0xFEDCBA98);
  EXPECT_EQ(readLE32(buf), 0xFEDCBA98u);

  cmd16(buf, 0x3000, 0xE00);  // READ OSC
  EXPECT_EQ(buf[0], 0x3E);
  EXPECT_EQ(buf[1], 0x00);
  cmd16(buf, 0x2000, 0xF7FF);  // WRITE, the address is masked to 12 bits
  EXPECT_EQ(buf[0], 0x27);
  EXPECT_EQ(buf[1], 0xFF);
}

// --- Ring buffers ---

TEST(RingbufTest, WrapsAroundAndRejectsPartialWrites) {
  uint8_t storage[16];
  struct ringbuf rb;
  ringbuf_init(&rb, storage, sizeof(storage));
  EXPECT_EQ(ringbuf_free_space(&rb), 16u);

  uint8_t in[16], out[16];
  for (int i = 0; i < 16; i++) {
    in[i] = 0xA0 + i;
  }
  uint8_t next = 0;
  for (int round = 0; round < 10; round++) {
    // 10 bytes at a time, so writes and reads straddle the end of the buffer
    for (int i = 0; i < 10; i++) {
      in[i] = next++;
    }
    ASSERT_EQ(ringbuf_write(&rb, in, 10), 10u);
    EXPECT_EQ(ringbuf_free_space(&rb), 6u);
    EXPECT_EQ(ringbuf_write(&rb, in, 7), 0u);  // Doesn't fit, nothing written
    EXPECT_EQ(ringbuf_free_space(&rb), 6u);
    ASSERT_EQ(ringbuf_read(&rb, out, sizeof(out)), 10u);
    EXPECT_EQ(memcmp(in, out, 10), 0) << round;
    EXPECT_EQ(ringbuf_read(&rb, out, sizeof(out)), 0u);
  }

  // Exactly full
  ASSERT_EQ(ringbuf_write(&rb, in, 16), 16u);
  EXPECT_EQ(ringbuf_free_space(&rb), 0u);
  EXPECT_EQ(ringbuf_write(&rb, in, 1), 0u);

  // peek gives the contiguous part, consume releases it
  uint8_t* p;
  const size_t n = ringbuf_peek(&rb, &p);
  EXPECT_EQ(n, 16u - (100 % 16));
  EXPECT_EQ(memcmp(p, in, n), 0);
  ringbuf_consume(&rb, n);
  EXPECT_EQ(ringbuf_free_space(&rb), n);
  EXPECT_EQ(ringbuf_read(&rb, nullptr, 16), 16u - n);  // Discard the rest
  EXPECT_EQ(ringbuf_free_space(&rb), 16u);
}

static MCP2518FD_Lite_RingRecord makeRecord(uint32_t seq, uint8_t len) {
  MCP2518FD_Lite_RingRecord r = {};
  r.id = 0x1000 + seq;
  r.len = len;
  r.flags = seq & 3;
  for (int i = 0; i < len; i++) {
    r.payload[i] = (uint8_t)(seq * 7 + i);
  }
  return r;
}

static void expectRecord(const MCP2518FD_Lite_RingRecord& r, uint32_t seq, uint8_t len) {
  EXPECT_EQ(r.id, 0x1000 + seq);
  EXPECT_EQ(r.len, len);
  EXPECT_EQ(r.flags, seq & 3);
  for (int i = 0; i < len; i++) {
    EXPECT_EQ(r.payload[i], (uint8_t)(seq * 7 + i)) << "seq " << seq << " byte " << i;
  }
}

TEST(RecordRingTest, HeaderMatchesLayout) {
  EXPECT_EQ(offsetof(MCP2518FD_Lite_RingRecord, payload), MCP2518FD_Lite_RingRecord::HEADER_LEN);
}

TEST(RecordRingTest, MixedLengthsRoundTripInOrder) {
  static const uint8_t kLens[] = {0, 8, 64, 1, 12, 48, 8, 3, 64, 24};
  MCP2518FD_Lite_RecordRing<256> ring;
  MCP2518FD_Lite_RingRecord r;
  EXPECT_TRUE(ring.empty());
  EXPECT_FALSE(ring.read(r));

  // Keep it about half full over many wraps of the 256 byte buffer
  uint32_t written = 0, read = 0;
  for (int i = 0; i < 200; i++) {
    const uint8_t len = kLens[written % 10];
    ASSERT_TRUE(ring.write(makeRecord(written, len)));
    written++;
    if (written - read > 2) {
      ASSERT_TRUE(ring.read(r));
      expectRecord(r, read, kLens[read % 10]);
      read++;
    }
  }
  while (ring.read(r)) {
    expectRecord(r, read, kLens[read % 10]);
    read++;
  }
  EXPECT_EQ(read, written);
  EXPECT_TRUE(ring.empty());
}

TEST(RecordRingTest, FullRingRejectsWithoutCorrupting) {
  MCP2518FD_Lite_RecordRing<256> ring;
  // 72 byte records: three fit (216 bytes), the fourth doesn't
  for (uint32_t i = 0; i < 3; i++) {
    ASSERT_TRUE(ring.write(makeRecord(i, 64)));
  }
  EXPECT_FALSE(ring.write(makeRecord(3, 64)));
  // But a 40 byte one fills it exactly, and then nothing else fits
  EXPECT_TRUE(ring.write(makeRecord(3, 32)));
  EXPECT_FALSE(ring.write(makeRecord(4, 0)));

  MCP2518FD_Lite_RingRecord r;
  for (uint32_t i = 0; i < 3; i++) {
    ASSERT_TRUE(ring.read(r));
    expectRecord(r, i, 64);
  }
  ASSERT_TRUE(ring.read(r));
  expectRecord(r, 3, 32);
  EXPECT_FALSE(ring.read(r));
}

static void fillSlot(uint8_t* slot, uint32_t seq) {
  memset(slot, 0, 14);
  memcpy(slot, &seq, sizeof(seq));
}

static uint32_t slotSeq(const uint8_t* slot) {
  uint32_t seq;
  memcpy(&seq, slot, sizeof(seq));
  return seq;
}

TEST(FrameRingTest, FillsToCapacityAndPreservesOrder) {
  // Start near the counters' wrap, which they are free-running across
  for (uint32_t start : {0u, 0xFFFFFFF0u}) {
    SCOPED_TRACE(start);
    MCP2515_Lite_FrameRing ring;
    ring.head = start;
    ring.tail = start;
    EXPECT_EQ(ring.readSlot(), nullptr);

    for (uint32_t i = 0; i < MCP2515_Lite_FrameRing::SIZE; i++) {
      uint8_t* slot = ring.writeSlot();
      ASSERT_NE(slot, nullptr) << i;
      fillSlot(slot, i);
      ring.push();
    }
    EXPECT_EQ(ring.writeSlot(), nullptr);

    for (uint32_t i = 0; i < MCP2515_Lite_FrameRing::SIZE; i++) {
      uint8_t* slot = ring.readSlot();
      ASSERT_NE(slot, nullptr) << i;
      EXPECT_EQ(slotSeq(slot), i);
      ring.pop();
    }
    EXPECT_EQ(ring.readSlot(), nullptr);
  }
}

TEST(FrameRingTest, PushKeepingSpareDropsWhenFull) {
  MCP2515_Lite_FrameRing ring;
  // The RX interrupt always writes into spareSlot(), and keeps one slot back
  uint32_t accepted = 0;
  for (uint32_t i = 0; i < 40; i++) {
    fillSlot(ring.spareSlot(), i);
    if (ring.pushKeepingSpare()) {
      accepted++;
    }
  }
  EXPECT_EQ(accepted, MCP2515_Lite_FrameRing::SIZE - 1);

  // The oldest frames were kept, and the dropped ones never became visible
  for (uint32_t i = 0; i < accepted; i++) {
    uint8_t* slot = ring.readSlot();
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slotSeq(slot), i);
    ring.pop();
  }
  EXPECT_EQ(ring.readSlot(), nullptr);

  // And it recovers after draining
  fillSlot(ring.spareSlot(), 100);
  EXPECT_TRUE(ring.pushKeepingSpare());
  ASSERT_NE(ring.readSlot(), nullptr);
  EXPECT_EQ(slotSeq(ring.readSlot()), 100u);
}

// --- SPI clock divider ---

static const uint32_t kApb = 80000000;

// The SPI clock the register value gives
static uint32_t decodeClockReg(uint32_t reg) {
  if (reg & (1u << 31)) {
    return kApb;
  }
  const uint32_t pre = (reg >> 18) + 1;
  const uint32_t n = ((reg >> 12) & 0x3F) + 1;
  EXPECT_EQ(reg & 0x3F, n - 1);  // CLKCNT_L = CLKCNT_N
  EXPECT_EQ(((reg >> 6) & 0x3F) + 1, n / 2);
  return kApb / (pre * n);
}

TEST(AsyncSpiClockTest, DriverClocksAreExact) {
  EXPECT_EQ(asyncSpiClockReg(kApb, 80000000), 1u << 31);
  EXPECT_EQ(asyncSpiClockReg(kApb, 10000000), (7u << 12) | (3u << 6) | 7u);  // N=8, H=4
  for (uint32_t hz : {1000000u, 10000000u, 20000000u, 40000000u}) {
    EXPECT_EQ(decodeClockReg(asyncSpiClockReg(kApb, hz)), hz);
  }
}

TEST(AsyncSpiClockTest, OtherClocksAreNeverFaster) {
  for (uint32_t hz : {100000000u, 30000000u, 17000000u, 3000000u, 1234567u, 333333u, 10000u}) {
    const uint32_t reg = asyncSpiClockReg(kApb, hz);
    const uint32_t actual = decodeClockReg(reg);
    EXPECT_LE(actual, hz) << hz;
    // Within one divider step
    EXPECT_GT(actual, hz * 0.75) << hz;
    if (!(reg & (1u << 31))) {
      EXPECT_LT(reg >> 18, 1u << 13) << hz;  // CLKDIV_PRE is 13 bits
    }
  }
}
