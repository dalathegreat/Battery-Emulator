#include "mcp2515_lite.h"
#include <Arduino.h>

#include "src/devboard/utils/logging.h"

// MCP2515 Opcodes and Registers
#define CMD_WRITE               0x02
#define CMD_READ                0x03
#define CMD_BIT_MODIFY          0x05
#define CMD_LOAD_TX_BUFFER      0x40
#define CMD_READ_RX_BUFFER      0x90
#define CMD_READ_STATUS         0xA0
#define CMD_RESET               0xC0

#define CANCTRL_REQOP_NORMAL    0x00
#define CANCTRL_REQOP_CONFIG    0x80

#define REG_CANCTRL     0x0F
#define REG_CNF1        0x2A
#define REG_CNF2        0x29
#define REG_CNF3        0x28
#define REG_CANINTE     0x2B
#define REG_CANINTF     0x2C
#define REG_RXB0CTRL    0x60
#define REG_RXB1CTRL    0x70

#define STATUS_TX0IF    0x08
#define STATUS_TX1IF    0x20
#define STATUS_TX2IF    0x80

#define CANINTF_TX0IF    0x04
#define CANINTF_TX1IF    0x08
#define CANINTF_TX2IF    0x10


static inline void packExtendedId(uint8_t* buffer, uint32_t id);
static inline uint32_t unpackExtendedId(const uint8_t* buffer);
static inline void packStandardId(uint8_t* buffer, uint32_t id);
static inline uint32_t unpackStandardId(const uint8_t* buffer);


MCP2515_Lite::MCP2515_Lite(SPIClass& spi, uint8_t cs, uint8_t int_pin) 
    : _spi(spi), _cs(cs), _int_pin(int_pin) {
    
    // Initialize queues (this will allocate memory for storing the frames)
    _tx_queue = xQueueCreate(MCP2515_LITE_TX_QUEUE_DEPTH, sizeof(MCP2515_Lite_Frame));
    _rx_queue = xQueueCreate(MCP2515_LITE_RX_QUEUE_DEPTH, sizeof(MCP2515_Lite_Frame));
}

MCP2515_Lite::~MCP2515_Lite() {
    if (_can_task_handle) {
        vTaskDelete(_can_task_handle);
        _can_task_handle = nullptr;
    }
    if (_tx_queue) {
        vQueueDelete(_tx_queue);
        _tx_queue = nullptr;
    }
    if (_rx_queue) {
        vQueueDelete(_rx_queue);
        _rx_queue = nullptr;
    }
    detachInterrupt(digitalPinToInterrupt(_int_pin));
}

static bool calculateMCP2515Config(uint32_t f_osc, uint32_t can_rate, uint8_t *cnf) {
    if (!cnf || can_rate == 0 || f_osc == 0) return false;

    // Calculate for TQ = 16 (will fail for 500kbit@8MHz)
    uint32_t div16 = 32 * can_rate;
    uint32_t brp16 = (f_osc + (div16 / 2)) / div16; // Integer rounding
    if (brp16 < 1) brp16 = 1; else if (brp16 > 64) brp16 = 64;
    uint32_t rate16 = f_osc / (32 * brp16);
    uint32_t err16 = (rate16 > can_rate) ? (rate16 - can_rate) : (can_rate - rate16);
    
    // Calculate for TQ = 8 (lower resolution)
    uint32_t div8 = 16 * can_rate;
    uint32_t brp8 = (f_osc + (div8 / 2)) / div8; // Integer rounding
    if (brp8 < 1) brp8 = 1; else if (brp8 > 64) brp8 = 64;
    uint32_t rate8  = f_osc / (16 * brp8);
    uint32_t err8  = (rate8 > can_rate)  ? (rate8 - can_rate)  : (can_rate - rate8);

    if (err8 < err16) {
        // TQ=8 has lower error, use that
        cnf[0] = (uint8_t)(brp8 - 1);
        cnf[1] = 0x8A; // BTLMODE=1, SAM=0, PHSEG1=1, PRSEG=2
        cnf[2] = 0x01; // PHSEG2=1
    } else {
        // otherwise use TQ=16
        cnf[0] = (uint8_t)(brp16 - 1);
        cnf[1] = 0xA5; // BTLMODE=1, SAM=0, PHSEG1=4, PRSEG=5
        cnf[2] = 0x03; // PHSEG2=3
    }

    return true;
}

bool MCP2515_Lite::begin(const MCP2515_Lite_Speed& speed) {
    // 1. Set up GPIO pins (SCK/MOSI/MISO are already set up)

    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

    pinMode(_int_pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(_int_pin), mcp2515_isr_handler, this, FALLING);

    // 1. Reset and configure the MCP2515

    if(!reset()) {
        return false;
    }

    // Enter config mpde
    modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_CONFIG);

    // Turn off masks/filters to receive everything into both buffers
    writeRegister(REG_RXB0CTRL, 0x64); // enable rollover for double-buffered rx
    writeRegister(REG_RXB1CTRL, 0x60);

    // Enable interrupts for TX2, TX1, TX0, RX1 and RX0
    modifyRegister(REG_CANINTE, 0x07, 0b00011111);

    // Baudrate setup
    applySpeedConfig(speed);

    // Leave config mode and enter normal mode
    modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_NORMAL);

    // Start the background task
    xTaskCreate(canTask, "MCP2515_Lite", MCP2515_LITE_TASK_STACK_SIZE, this, MCP2515_LITE_TASK_PRIORITY, &_can_task_handle);

    return true;
}

bool MCP2515_Lite::sendFrame(const MCP2515_Lite_Frame& msg) {
    if (xQueueSend(_tx_queue, &msg, 0) == pdTRUE) {
        // Notify the task that there's a new message in the queue
        if (_can_task_handle) xTaskNotifyGive(_can_task_handle);
        return true;
    }
    return false;
}

bool MCP2515_Lite::receiveFrame(MCP2515_Lite_Frame& msg) {
    if(_rx_overflow) {
        DEBUG_PRINTF("MCP2515 RX queue overflow!\n");
        _rx_overflow = false;
    }
    // Grab a message from the RX queue if available
    return (xQueueReceive(_rx_queue, &msg, 0) == pdTRUE);
}

void MCP2515_Lite::changeSpeed(const MCP2515_Lite_Speed& new_speed) {
    _next_speed = new_speed;
    _speed_change_pending = true;
}

void MCP2515_Lite::pause(bool paused) {
    _pause_requested = paused;
    // Wake the task to apply the pause state
    xTaskNotifyGive(_can_task_handle);
}


static const SPISettings spiSettings(10000000, MSBFIRST, SPI_MODE0);

void MCP2515_Lite::spiTransactionBlocking(const uint8_t* tx_data, uint8_t* rx_data, size_t length) {
    // This should send as a single transaction, yielding to FreeRTOS and
    // returning after completion.

    _spi.beginTransaction(spiSettings);
    digitalWrite(_cs, LOW);
    _spi.transferBytes(tx_data, rx_data, length);
    digitalWrite(_cs, HIGH);
    _spi.endTransaction();
}

void MCP2515_Lite::canTask(void* pvParameters) {
    MCP2515_Lite* self = static_cast<MCP2515_Lite*>(pvParameters);
    MCP2515_Lite_Frame can_frame;
    
    // Bits 0, 1, 2 represent TXB0, TXB1, TXB2. Start with all 3 free (0x07)
    uint8_t tx_free_mask = 0x07; 

    // Reusable buffers for SPI payloads
    uint8_t cmd_frame[16];
    uint8_t rx_frame[16];

    while(true) {
        // Sleep the task until ISR or `sendFrame` wakes us up. We also wake
        // after a timeout just in case we've missed an interrupt and there's
        // something pending to do.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MCP2515_LITE_POLL_TIMEOUT_MS));

        // 1. Pause/unpause if requested

        if(self->_paused && !self->_pause_requested) {
            // Enter normal mode to resume communication
            self->modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_NORMAL);
            self->_paused = false;
        } else if (!self->_paused && self->_pause_requested) {
            // Enter configuration mode to pause
            self->modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_CONFIG);
            self->_paused = true;
            continue;
        } else if (self->_paused) {
            // We're paused, skip the rest
            continue;
        }

        // Keep looping doing RX/TX until there's no more work to do (the notify
        // interrupt is edge triggered so won't retrigger until we clear all
        // pending work).
        bool work_done;
        do {
            work_done = false;

            // 2. Read the status register to see which interrupts are active

            cmd_frame[0] = CMD_READ_STATUS;
            self->spiTransactionBlocking(cmd_frame, rx_frame, 2);
            const uint8_t status = rx_frame[1];

            // 3. Process any RX interrupts sequentially (clears receive flags)

            for (int i = 0; i < 2; i++) {
                // Is there a frame in this slot to read?
                if (status & (1 << i)) {
                    cmd_frame[0] = CMD_READ_RX_BUFFER | (i * 4); // 0x90 (RXB0) or 0x94 (RXB1)
                    
                    // Write 1 command byte + 13 payload read bytes
                    self->spiTransactionBlocking(cmd_frame, rx_frame, 14);
                    
                    can_frame.flags = 0;
                    if (rx_frame[2] & 0x08) { // IDE bit
                        can_frame.ext = true;
                        can_frame.id = unpackExtendedId(&rx_frame[1]);
                    } else {
                        can_frame.ext = false;
                        can_frame.id = unpackStandardId(&rx_frame[1]);
                    }
                    can_frame.dlc = rx_frame[5] & 0x0F;
                    memcpy(can_frame.data, &rx_frame[6], can_frame.dlc);
                    
                    if(xQueueSend(self->_rx_queue, &can_frame, 0) != pdTRUE) {
                        self->_rx_overflow = true;
                    }
                    work_done = true;
                }
            }

            // 4. Check for free transmit buffers, and clear interrupt flags if needed

            int int_clear_mask = 0;
            if (status & STATUS_TX0IF) { int_clear_mask |= CANINTF_TX0IF; tx_free_mask |= 0x01; } // TXB0 free
            if (status & STATUS_TX1IF) { int_clear_mask |= CANINTF_TX1IF; tx_free_mask |= 0x02; } // TXB1 free
            if (status & STATUS_TX2IF) { int_clear_mask |= CANINTF_TX2IF; tx_free_mask |= 0x04; } // TXB2 free

            if (int_clear_mask) {
                self->modifyRegister(REG_CANINTF, int_clear_mask, 0x00);
            }

            // 5. Perform a speed change if requested

            if (self->_speed_change_pending) {
                if(tx_free_mask != 0x07) {
                    // There's still something being sent. If we're at the wrong
                    // speed, it probably won't ever send, so wait long enough
                    // to give a chance and then change speed anyway.
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                self->modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_CONFIG);
                self->applySpeedConfig(self->_next_speed);
                self->modifyRegister(REG_CANCTRL, 0xE0, CANCTRL_REQOP_NORMAL);
                self->_speed_change_pending = false;
            }

            // 6. Transmit any pending messages (if we have free buffers)

            uint8_t rts_mask = 0;
            if (tx_free_mask > 0) {
                // We have free tx buffers, do we have anything to send?
                for (int i = 0; i < 3; i++) {
                    if ((tx_free_mask & (1 << i)) && xQueueReceive(self->_tx_queue, &can_frame, 0)) {
                        // This slot is no longer free
                        tx_free_mask &= ~(1 << i); 
                        // Mark this slot as ready-to-send
                        rts_mask |= (1 << i);

                        cmd_frame[0] = CMD_LOAD_TX_BUFFER | (i * 2); // 0x40 (TXB0), 0x42 (TXB1), or 0x44 (TXB2)
                        
                        if (can_frame.ext) {
                            packExtendedId(&cmd_frame[1], can_frame.id);
                        } else {
                            packStandardId(&cmd_frame[1], can_frame.id);
                        }
                        // Limit to 8 bytes (in case someone tries to send a FD frame)
                        uint8_t payload_len = can_frame.dlc > 8 ? 8 : can_frame.dlc;
                        cmd_frame[5] = payload_len;
                        memcpy(&cmd_frame[6], can_frame.data, payload_len);

                        self->spiTransactionBlocking(cmd_frame, nullptr, 6 + payload_len);
                    }
                }
            }

            // 7. Trigger sending if required

            if (rts_mask > 0) {
                cmd_frame[0] = 0x80 | rts_mask;
                self->spiTransactionBlocking(cmd_frame, nullptr, 1);
                work_done = true;
            }
        } while(work_done);
    }
}

// SPI helper functions

void MCP2515_Lite::writeRegister(uint8_t reg, uint8_t value) {
    modifyRegister(reg, 0xFF, value);
}

uint8_t MCP2515_Lite::readRegister(uint8_t reg) {
    uint8_t cmd[] = {CMD_READ, reg, 0x00};
    uint8_t rx[3] = {0};
    spiTransactionBlocking(cmd, rx, 3);
    return rx[2];
}

void MCP2515_Lite::modifyRegister(uint8_t reg, uint8_t mask, uint8_t data) {
    uint8_t cmd[] = {CMD_BIT_MODIFY, reg, mask, data};
    spiTransactionBlocking(cmd, nullptr, 4);
}

bool MCP2515_Lite::reset() {
    const uint8_t cmd[] = {CMD_RESET};
    spiTransactionBlocking(cmd, nullptr, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t CANSTAT = readRegister(0x0E);
    if(CANSTAT != 0x80) {
        DEBUG_PRINTF("MCP2515 reset failed, CANSTAT=0x%02X\n", CANSTAT);
        return false;
    }
    return true;
}

void MCP2515_Lite::applySpeedConfig(const MCP2515_Lite_Speed& speed) {
    uint8_t cnf[3];
    if(calculateMCP2515Config(speed.f_osc, speed.bitrate, cnf)) {
        writeRegister(REG_CNF1, cnf[0]);
        writeRegister(REG_CNF2, cnf[1]);
        writeRegister(REG_CNF3, cnf[2]);
    }
}

// ISR called when MCP2515 signals an interrupt via the /INT pin.
void IRAM_ATTR MCP2515_Lite::mcp2515_isr_handler(void* arg) {
    MCP2515_Lite* instance = static_cast<MCP2515_Lite*>(arg);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Notify task that there's an interrupt to handle
    if (instance->_can_task_handle) {
        vTaskNotifyGiveFromISR(instance->_can_task_handle, &xHigherPriorityTaskWoken);
    }
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
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