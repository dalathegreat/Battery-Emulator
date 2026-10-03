#include "mcp2515_lite.h"
#include <Arduino.h>

#include "driver/gpio.h"
#include "hal/gpio_ll.h"
#include "esp_timer.h"
#include "src/devboard/utils/logging.h"

// MCP2515 Opcodes and Registers
#define CMD_WRITE               0x02
#define CMD_READ                0x03
#define CMD_BIT_MODIFY          0x05
#define CMD_RESET               0xC0
#define CMD_READ_RXB0           0x90
#define CMD_LOAD_TXB0           0x40
#define CMD_RTS_TXB0            0x81

#define CANCTRL_REQOP_NORMAL    0x00
#define CANCTRL_REQOP_CONFIG    0x80
#define CANCTRL_REQOP_LOOPBACK  0x40
#define CANCTRL_REQOP_MASK      0xE0
#define CANCTRL_ABAT            0x10

#define REG_CANSTAT     0x0E
#define REG_CANCTRL     0x0F
#define REG_CNF1        0x2A
#define REG_CNF2        0x29
#define REG_CNF3        0x28
#define REG_CANINTE     0x2B
#define REG_CANINTF     0x2C
#define REG_RXB0CTRL    0x60

#define CANINTF_RX0IF    0x01
#define CANINTF_TX0IF    0x04
#define CANINTF_ERRIF    0x20

#define EFLG_TXBO        0x20
#define EFLG_EWARN       0x01


static inline void packExtendedId(uint8_t* buffer, uint32_t id);
static inline uint32_t unpackExtendedId(const uint8_t* buffer);
static inline void packStandardId(uint8_t* buffer, uint32_t id);
static inline uint32_t unpackStandardId(const uint8_t* buffer);


MCP2515_Lite::MCP2515_Lite(spi_host_device_t host, int sck, int mosi, int miso, int cs, int int_pin)
    : AsyncSpiDevice(stepDevice), _host(host), _sck(sck), _mosi(mosi), _miso(miso), _cs(cs), _int_pin(int_pin) {}

MCP2515_Lite::~MCP2515_Lite() {
    if (bus) {
        bus->detachInt(this);
        bus->remove(this);
    }
}

// Calculate the CNF1..CNF3 register values (into cnf[0..2]) for a CAN bitrate
// from oscillator f_osc. Uses the most Tq per bit (16 down to 8) that gives the
// exact bitrate, with a ~75% sample point. Returns false if none does.
static bool calculateMCP2515Config(uint32_t f_osc, uint32_t can_rate, uint8_t *cnf) {
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

bool MCP2515_Lite::joinBus() {
    if (bus) {
        return true;
    }
    pinMode(_int_pin, INPUT_PULLUP);
    AsyncSpiBus* b = AsyncSpiBus::get(_host, _sck, _mosi, _miso);
    return b && b->add(this, _cs, MCP2515_LITE_SPI_HZ);
}

uint32_t MCP2515_Lite::autodetectOscillatorFrequency() {
    // 8000 baud at 8MHz is ~125us per bit. A 16MHz chip will take half the time.
    if (!joinBus() || !configure({8000, 8000000}, true)) {
        return 0;
    }

    uint8_t frame[14];
    frame[0] = CMD_LOAD_TXB0;
    packExtendedId(&frame[1], 0x12345678);
    frame[5] = 0x08; // DLC
    transferBlocking(frame, sizeof(frame));

    // Send it, and wait for INT (TX done / RX full)
    frame[0] = CMD_RTS_TXB0;
    const int64_t t1 = esp_timer_get_time();
    transferBlocking(frame, 1);
    int64_t t2 = t1;
    while (gpio_get_level((gpio_num_t)_int_pin) != 0 && t2 - t1 < 100000) {
        t2 = esp_timer_get_time();
    }

    const uint32_t elapsed_us = (uint32_t)(t2 - t1);
    DEBUG_PRINTF("MCP2515: autodetect=%uus\n", elapsed_us);
    reset();

    return elapsed_us < 13500 ? 16000000 : 8000000;
}

bool MCP2515_Lite::begin(const MCP2515_Lite_Speed& speed, bool loopback) {
    if (!joinBus() || !configure(speed, loopback)) {
        return false;
    }
    if (!_started) {
        if (!bus->attachInt(this, _int_pin)) {
            return false;
        }
        _started = true;
    }
    requestStep();  // Anything already pending
    return true;
}

// Disable the stepping (the interrupt-driven state machine that does the
// tx/rx). If we are mid tx/rx, wait for it to complete first.
void MCP2515_Lite::disableStepping() {
    _stepping_disabled = true;
    // Wait for any ongoing transmission to complete by trying to acquire the
    // bus (which will block).
    bus->hold(true);
    // And then release it again
    bus->hold(false);
}

// Re-enable stepping (the interrupt-driven state machine for tx/rx), and do a
// run in case there is something to tx/rx.
void MCP2515_Lite::enableStepping() {
    _stepping_disabled = false;
    requestStep();
}

// Change chip operating mode (between normal, loopback, and config). 
// Leaving normal mode waits for a pending transmission, which might never
// finish, so after ~10ms we abort.
bool MCP2515_Lite::setMode(uint8_t reqop) {
    modifyRegister(REG_CANCTRL, CANCTRL_REQOP_MASK, reqop);
    bool aborted = false;
    bool ok = false;
    for (int i = 0; i < 50 && !ok; i++) {
        // Check if the chip has entered the requested operating mode.
        ok = (readRegister(REG_CANSTAT) & CANCTRL_REQOP_MASK) == reqop;
        if (!ok) {
            if (i == 10 && !aborted) {
                // Abort after 10ms
                modifyRegister(REG_CANCTRL, CANCTRL_ABAT, CANCTRL_ABAT);
                aborted = true;
            }
            vTaskDelay(1);
        }
    }
    if (aborted) {
        // Clear the abort flag after aborting the pending transmission.
        modifyRegister(REG_CANCTRL, CANCTRL_ABAT, 0);
        _txb0_free = true;
    }
    return ok;
}

uint8_t MCP2515_Lite::runMode() const {
    return _loopback ? CANCTRL_REQOP_LOOPBACK : CANCTRL_REQOP_NORMAL;
}

// Reset and set up the chip
bool MCP2515_Lite::configure(const MCP2515_Lite_Speed& speed, bool loopback) {
    disableStepping();
    bool ok = setup(speed, loopback);
    enableStepping();
    return ok;
}

// Actually reset and set up the chip (must be called with stepping disabled)
bool MCP2515_Lite::setup(const MCP2515_Lite_Speed& speed, bool loopback) {
    bool ok = reset();  // (Leaves it in config mode)
    if (ok) {

        // Turn off masks/filters to receive everything into RXB0, with no rollover
        // into RXB1 (so RXB1 never gets anything)
        writeRegister(REG_RXB0CTRL, 0x60);

        // Interrupts for RX0 and TX0 (errors are picked up with them)
        writeRegister(REG_CANINTE, 0x05);

        ok = applySpeedConfig(speed);
        _loopback = loopback;
        _txb0_free = true;
        _paused = false;
        ok = ok && setMode(runMode());
    }
    return ok;
}

// Send a CAN frame (copying it into the TX ring buffer and starting
// transmission when able). Returns true if the frame was successfully queued.
bool MCP2515_Lite::sendFrame(const MCP2515_Lite_Frame& msg) {
    // The ring buffers are single-producer, and multiple tasks may send, so we
    // need to lock
    portENTER_CRITICAL(&_tx_lock);
    // Get a free slot in the TX ring to fill with the new frame
    uint8_t* frame = _tx.writeSlot();
    if (!frame) {
        // Transmit ring is full
        portEXIT_CRITICAL(&_tx_lock);
        return false;
    }
    // We don't set frame[0] as it gets populated later
    if (msg.ext) {
        packExtendedId(&frame[1], msg.id);
    } else {
        packStandardId(&frame[1], msg.id);
    }
    frame[5] = msg.dlc > 8 ? 8 : msg.dlc;
    memcpy(&frame[6], msg.data, frame[5]);
    // Indicate that the latest frame is ready to be sent
    _tx.push();
    portEXIT_CRITICAL(&_tx_lock);
    requestStep();
    return true;
}

// Receive a CAN frame from the RX ring buffer. Returns true if a frame was
// available.
bool MCP2515_Lite::receiveFrame(MCP2515_Lite_Frame& msg) {
    if (_rx_overflow) {
        DEBUG_PRINTF("MCP2515 RX queue overflow!\n");
        _rx_overflow = false;
    }
    // Safety net for a missed INT edge
    uint32_t now = millis();
    if (now - _last_poll_ms >= MCP2515_LITE_POLL_TIMEOUT_MS) {
        _last_poll_ms = now;
        requestStep();
    }
    // Grab the next available frame from the RX ring
    const uint8_t* frame = _rx.readSlot();  // frame[0] is the command echo
    if (!frame) {
        // No frame was available
        return false;
    }
    // Decode the chip's frame format
    msg.flags = 0;
    msg.ext = frame[2] & 0x08;
    msg.id = msg.ext ? unpackExtendedId(&frame[1]) : unpackStandardId(&frame[1]);
    const uint8_t dlc = frame[5] & 0x0F;  // (The byte's RTR bit isn't part of it)
    msg.dlc = dlc > 8 ? 8 : dlc;
    memcpy(msg.data, &frame[6], msg.dlc);
    // Release the slot
    _rx.pop();
    return true;
}

// Change the CAN bus speed. If a transmission is in progress, it waits briefly
// for it to complete before changing the speed.
void MCP2515_Lite::changeSpeed(const MCP2515_Lite_Speed& new_speed) {
    if (!bus) {
        return;
    }
    if (!_txb0_free) {
        // Give a frame in flight a chance at the old speed
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    disableStepping();
    if (setMode(CANCTRL_REQOP_CONFIG)) {
        applySpeedConfig(new_speed);
        if (!_paused) {
            setMode(runMode());
        }
    } else {
        // Transmit was still pending, so probably won't succeed, reset it
        // instead.
        const bool paused = _paused;
        setup(new_speed, _loopback);
        if (paused) {
            // Stay paused if we were paused before
            setMode(CANCTRL_REQOP_CONFIG);
            _paused = true;
        }
    }
    enableStepping();
}

// Pause the chip, which stops all communication and prevents ACKing.
void MCP2515_Lite::pause(bool paused) {
    if (!bus || paused == _paused) {
        return;
    }
    disableStepping();
    setMode(paused ? CANCTRL_REQOP_CONFIG : runMode());
    _paused = paused;
    enableStepping();
}

int MCP2515_Lite::debugState(char* buf, size_t n) {
    if (!bus) {
        return snprintf(buf, n, "2515: no bus");
    }
    uint8_t b[] = {CMD_READ, REG_CANSTAT, 0, 0};  // CANSTAT, CANCTRL
    transferBlocking(b, 4);
    uint8_t canstat = b[2], canctrl = b[3];
    uint8_t e[] = {CMD_READ, 0x1C, 0, 0};  // TEC, REC
    transferBlocking(e, 4);
    uint8_t f[] = {CMD_READ, REG_CANINTF, 0, 0};  // CANINTF, EFLG
    transferBlocking(f, 4);
    return snprintf(buf, n,
                    "2515: CANSTAT %02x CANCTRL %02x TEC %u REC %u CANINTF %02x EFLG %02x TXB0CTRL %02x CNF %02x%02x%02x | "
                    "txb0_free %d paused %d stepping_disabled %d started %d tx %lu/%lu rx %lu/%lu state %u pending %d int %d",
                    canstat, canctrl, e[2], e[3], f[2], f[3], readRegister(0x30), readRegister(REG_CNF1),
                    readRegister(REG_CNF2), readRegister(REG_CNF3), _txb0_free, _paused, _stepping_disabled, _started,
                    (unsigned long)_tx.head, (unsigned long)_tx.tail, (unsigned long)_rx.head, (unsigned long)_rx.tail,
                    state, pending, gpio_ll_get_level(&GPIO, _int_pin));
}

// Blocking register access

void MCP2515_Lite::writeRegister(uint8_t reg, uint8_t value) {
    uint8_t buf[] = {CMD_WRITE, reg, value};
    transferBlocking(buf, sizeof(buf));
}

uint8_t MCP2515_Lite::readRegister(uint8_t reg) {
    uint8_t buf[] = {CMD_READ, reg, 0x00};
    transferBlocking(buf, sizeof(buf));
    return buf[2];
}

void MCP2515_Lite::modifyRegister(uint8_t reg, uint8_t mask, uint8_t data) {
    uint8_t buf[] = {CMD_BIT_MODIFY, reg, mask, data};
    transferBlocking(buf, sizeof(buf));
}

bool MCP2515_Lite::reset() {
    uint8_t buf[] = {CMD_RESET};
    transferBlocking(buf, sizeof(buf));
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t canstat = readRegister(REG_CANSTAT);
    if (canstat != 0x80) {
        DEBUG_PRINTF("MCP2515 reset failed, CANSTAT=0x%02X\n", canstat);
        return false;
    }
    return true;
}

// Returns false (leaving the speed unchanged) if the bitrate isn't possible
bool MCP2515_Lite::applySpeedConfig(const MCP2515_Lite_Speed& speed) {
    uint8_t cnf[3];
    if (!calculateMCP2515Config(speed.f_osc, speed.bitrate, cnf)) {
        return false;
    }
    writeRegister(REG_CNF1, cnf[0]);
    writeRegister(REG_CNF2, cnf[1]);
    writeRegister(REG_CNF3, cnf[2]);
    return true;
}


enum { ST_START, ST_STATUS, ST_GOT_RX, ST_FLAGS, ST_TX, ST_RTS, ST_END };

// Step through the TX/RX state machine
bool IRAM_ATTR MCP2515_Lite::stepDevice(AsyncSpiDevice* dev) {
    return static_cast<MCP2515_Lite*>(dev)->step();
}

bool IRAM_ATTR ASYNC_SPI_BUS_HOT MCP2515_Lite::step() {
    uint8_t* cmd_buf = _cmd_buf;
    for (;;) {
        switch (state) {
            case ST_START:
                if (_paused || _stepping_disabled) {
                    return false;
                }
                _did_work = false;
                // Read the CANINTF register to check interrupt flags
                cmd_buf[0] = CMD_READ;
                cmd_buf[1] = REG_CANINTF;
                return spiTransfer(ST_STATUS, cmd_buf, cmd_buf, 4);  // CANINTF, then EFLG

            case ST_STATUS:
                _intf = cmd_buf[2];
                if ((_intf & CANINTF_ERRIF) && (cmd_buf[3] & (EFLG_TXBO | EFLG_EWARN))) {
                    // Uh oh, error flags were set
                    _errors = true;
                }
                if (_intf & CANINTF_RX0IF) {
                    // There's a message in RXB0!
                    // Grab a spare slot in the RX ring to put it in
                    uint8_t* frame = _rx.spareSlot();
                    // Reading the RXB0 frame (will clear RX0IF)
                    frame[0] = CMD_READ_RXB0;
                    return spiTransfer(ST_GOT_RX, frame, frame, 14);
                }
                state = ST_FLAGS;
                break;

            case ST_GOT_RX:
                if (!_rx.pushKeepingSpare()) {
                    _rx_overflow = true;
                }
                _did_work = true;
                state = ST_FLAGS;
                break;

            case ST_FLAGS:
                // Is TXB0 free?
                if (_intf & CANINTF_TX0IF) {
                    _txb0_free = true;
                }
                cmd_buf[2] = _intf & (CANINTF_TX0IF | CANINTF_ERRIF);
                if (cmd_buf[2]) {
                    // Clear the TX and error flags in CANINTF
                    cmd_buf[0] = CMD_BIT_MODIFY;
                    cmd_buf[1] = REG_CANINTF;
                    cmd_buf[3] = 0;
                    return spiTransfer(ST_TX, cmd_buf, nullptr, 4);
                }
                state = ST_TX;
                break;

            case ST_TX: {
                // If TXB0 is free, try to grab a frame awaiting sending
                uint8_t* frame = _txb0_free ? _tx.readSlot() : nullptr;
                if (!frame) {
                    // Nothing to send
                    state = ST_END;
                    break;
                }
                _txb0_free = false;
                // Load the frame into TXB0
                frame[0] = CMD_LOAD_TXB0;
                spiTransfer(ST_RTS, frame, nullptr, 6 + frame[5]);
                // Pop the frame now that we're sending it
                _tx.pop();
                return true;
            }

            case ST_RTS:
                // We've loaded a frame, now we request to send it
                cmd_buf[0] = CMD_RTS_TXB0;
                _did_work = true;
                return spiTransfer(ST_END, cmd_buf, nullptr, 1);

            default: {  // ST_END
                // RX/TX is done (or never happened), now check whether INT is
                // still asserted (low), or whether there's something still to
                // send. Set the pending flag accordingly so we go round again.

                // We tally the number of consecutive times INT has been low
                // while no work is pending, in case it is stuck low, to avoid
                // hogging the bus forever.

                bool int_low = gpio_ll_get_level(&GPIO, _int_pin) == 0;
                // Increment if we did no work but INT is still low
                _no_work_count = (_did_work || !int_low) ? 0 : _no_work_count + 1;
                if ((int_low && _no_work_count < 3) || (_txb0_free && _tx.readSlot())) {
                    pending = true;
                }
                return false;
            }
        }
    }
}

// Utility functions

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
