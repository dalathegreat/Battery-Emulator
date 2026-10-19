#pragma once

#include <stdint.h>

/* Pure helpers for MCP2515_Lite. These have no ESP-IDF dependencies, so the
host unit tests (in test/) can use them directly.
*/

// Calculate the CNF1..CNF3 register values (into cnf[0..2]) for a CAN bitrate
// from oscillator f_osc. Uses the most Tq per bit (16 down to 8) that gives the
// exact bitrate, with a ~75% sample point. Returns false if none does.
static inline bool calculateMCP2515Config(uint32_t f_osc, uint32_t can_rate, uint8_t *cnf) {
    if (!cnf || can_rate == 0) return false;

    for (uint32_t tq = 16; tq >= 8; tq--) {
        const uint32_t div = 2 * tq * can_rate;  // Tq = 2 * BRP / f_osc
        const uint32_t brp = f_osc / div;
        if (brp < 1 || brp > 64 || brp * div != f_osc) {
            continue;
        }
        const uint32_t phseg2 = tq / 4;
        const uint32_t phseg1 = (tq - 1 - phseg2) / 2;
        const uint32_t prseg = tq - 1 - phseg2 - phseg1;
        cnf[0] = (uint8_t)(brp - 1);                                      // SJW=1
        cnf[1] = (uint8_t)(0x80 | (phseg1 - 1) << 3 | (prseg - 1));       // BTLMODE=1, SAM=0
        cnf[2] = (uint8_t)(phseg2 - 1);
        return true;
    }
    return false;
}

// Pack/unpack CAN IDs to/from the chip's SIDH/SIDL/EID8/EID0 register layout.

static inline void packExtendedId(uint8_t* buffer, uint32_t id) {
    buffer[0] = id >> 21;
    buffer[1] = (((id >> 13) & 0xE0) | 0x08 | ((id >> 16) & 0x03));
    buffer[2] = id >> 8;
    buffer[3] = id;
}

static inline uint32_t unpackExtendedId(const uint8_t* buffer) {
    return ((uint32_t)buffer[0] << 21) |
           ((uint32_t)(buffer[1] & 0xE0) << 13) |
           ((uint32_t)(buffer[1] & 0x03) << 16) |
           ((uint32_t)buffer[2] << 8) |
           buffer[3];
}

static inline void packStandardId(uint8_t* buffer, uint32_t id) {
    buffer[0] = id >> 3;
    buffer[1] = (id & 0x07) << 5;
}

static inline uint32_t unpackStandardId(const uint8_t* buffer) {
    return ((uint32_t)buffer[0] << 3) | (buffer[1] >> 5);
}

// A ring buffer for storing frames (in the chip's format, so they can go
// straight to/from SPI). Single producer, single consumer.
struct MCP2515_Lite_FrameRing {
    static constexpr uint32_t SIZE = 32;  // Frames (power of two)
    uint8_t slots[SIZE][14];
    volatile uint32_t head = 0, tail = 0;  // Free-running

    // Get the next slot to write to, or nullptr if the ring is
    // full. Call push() after filling it to enqueue.
    uint8_t* writeSlot() { return head - tail < SIZE ? slots[head % SIZE] : nullptr; }
    void push() { __atomic_store_n(&head, head + 1, __ATOMIC_RELEASE); }

    // Get the next slot to write to, even if the ring is full (in which
    // case it reuses the last slot). pushKeepingSpare() won't push the last
    // slot so the frame effectively gets dropped. Used in the RX interrupt,
    // which always needs a slot to write the incoming frame.
    uint8_t* spareSlot() { return slots[head % SIZE]; }
    bool pushKeepingSpare() {
        if (head - tail >= SIZE - 1) {
            return false;
        }
        push();
        return true;
    }

    // Read the oldest frame in the ring, or nullptr if empty. Call pop()
    // after processing it to release the slot.
    uint8_t* readSlot() {
        return __atomic_load_n(&head, __ATOMIC_ACQUIRE) != tail ? slots[tail % SIZE] : nullptr;
    }
    void pop() { __atomic_store_n(&tail, tail + 1, __ATOMIC_RELEASE); }
};
