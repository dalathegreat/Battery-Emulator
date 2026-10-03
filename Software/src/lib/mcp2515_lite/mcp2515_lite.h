#pragma once

#include <stdint.h>

#include "async_spi_bus.h"

/* MCP2515_Lite: Minimal interrupt-driven MCP2515 driver.

 - Non-blocking send/receive through ring buffers
 - Pause/resume (suspends rx/tx and ACKs), on-the-fly speed changes
 - One hardware buffer each way (TXB0, RXB0), so frames are never reordered
 - Can share the bus with other AsyncSpiBus devices.
*/

#define MCP2515_LITE_SPI_HZ 10000000
// Always poll this often, in case an INT edge was missed
#define MCP2515_LITE_POLL_TIMEOUT_MS 1000

// This has the same layout as CAN_frame (with only 8 data bytes)
typedef struct {
  union {
    bool fd;
    uint8_t flags;
  };
  bool ext;
  uint8_t dlc;
  uint32_t id;
  uint8_t data[8];
} MCP2515_Lite_Frame;

typedef struct {
    uint32_t bitrate;
    uint32_t f_osc;
} MCP2515_Lite_Speed;

class MCP2515_Lite : public AsyncSpiDevice {
public:
    // Sets up `host` with these pins, unless another AsyncSpiBus device already did.
    MCP2515_Lite(spi_host_device_t host, int sck, int mosi, int miso, int cs, int int_pin);
    ~MCP2515_Lite();

    uint32_t autodetectOscillatorFrequency();

    bool begin(const MCP2515_Lite_Speed& speed, bool loopback = false);

    // Queue a frame to send (non-blocking)
    bool sendFrame(const MCP2515_Lite_Frame& msg);

    // Get the next received frame (non-blocking)
    bool receiveFrame(MCP2515_Lite_Frame& msg);

    // Change CAN speed (blocks briefly)
    void changeSpeed(const MCP2515_Lite_Speed& new_speed);

    // Pauses all communication (and stops acknowledging messages)
    void pause(bool paused);

    inline bool hasErrors() { auto ret = _errors; _errors = false; return ret; }

    // Debugging: the chip's key registers and the driver's state, as text
    int debugState(char* buf, size_t n);

private:
    spi_host_device_t _host;
    int _sck, _mosi, _miso;
    uint8_t _cs;
    uint8_t _int_pin;
    bool _started = false;  // True if begin() has attached the INT handler
    bool _loopback = false;

    volatile bool _paused = false;
    volatile bool _stepping_disabled = false;  // A task is reconfiguring the chip
    volatile bool _rx_overflow = false;
    volatile bool _errors = false;

    // A ring buffer for storing frames (in the chip's format, so they can go
    // straight to/from SPI). Single producer, single consumer.
    struct FrameRing {
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
    FrameRing _rx, _tx;
    portMUX_TYPE _tx_lock = portMUX_INITIALIZER_UNLOCKED;

    // State used by step()
    uint8_t _cmd_buf[4];  // Small commands and replies
    uint8_t _intf = 0, _no_work_count = 0;
    bool _txb0_free = true;
    bool _did_work = false;
    uint32_t _last_poll_ms = 0;

    bool joinBus();
    void requestStep() { if (_started) bus->requestStep(this); }
    bool step();
    static bool stepDevice(AsyncSpiDevice* dev);
    // From step(): start an SPI transfer and go to next_state when it's done.
    bool spiTransfer(uint8_t next_state, const uint8_t* tx, uint8_t* rx, uint8_t len) {
        state = next_state;
        bus->startTransfer(tx, rx, len);
        return true;  // (step()'s "more to do")
    }

    // Blocking register access, from tasks
    void transferBlocking(uint8_t* buf, size_t len) { bus->transferBlocking(this, buf, len); }
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void modifyRegister(uint8_t reg, uint8_t mask, uint8_t data);
    void disableStepping();
    void enableStepping();
    bool setMode(uint8_t reqop);
    uint8_t runMode() const;
    bool reset();
    bool configure(const MCP2515_Lite_Speed& speed, bool loopback);
    bool setup(const MCP2515_Lite_Speed& speed, bool loopback);
    bool applySpeedConfig(const MCP2515_Lite_Speed& speed);

};
