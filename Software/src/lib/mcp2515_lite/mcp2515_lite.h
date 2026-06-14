#pragma once

#include <stdint.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* MCP2515_Lite: Minimal MCP2515 library for Arduino+FreeRTOS.

Features:
 - Non-blocking send/receive APIs using preallocated FreeRTOS queues
 - High-priority background task performing blocking SPI transactions
 - Hardware triple-buffered transmit and double-buffered receive
 - Pause/resume functionality (suspends rx/tx and ACKs)
 - On-the-fly CAN bus speed changes
 - As few SPI transactions as possible for minimum overhead

*/

// A high priority will avoid drops during message bursts
#define MCP2515_LITE_TASK_PRIORITY 10
// Minimal stack size (+1500 if you want to do any printf debugging!)
#define MCP2515_LITE_TASK_STACK_SIZE 1100
// Queue depths (in messages)
#define MCP2515_LITE_TX_QUEUE_DEPTH 25
#define MCP2515_LITE_RX_QUEUE_DEPTH 25
// Poll this often if there are no interrupts
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

class MCP2515_Lite {
public:
    // Requires an initialized SPIClass (e.g. SPI or SPI2) passed by reference
    MCP2515_Lite(SPIClass& spi, uint8_t cs, uint8_t int_pin);
    ~MCP2515_Lite();

    bool begin(const MCP2515_Lite_Speed& speed);
    
    // Non-blocking: pushes message to the TX queue
    bool sendFrame(const MCP2515_Lite_Frame& msg);
    
    // Non-blocking: pops message from the RX queue
    bool receiveFrame(MCP2515_Lite_Frame& msg);

    // Non-blocking: signals the task to change CAN bus speed
    void changeSpeed(const MCP2515_Lite_Speed& new_speed);

    // Non-blocking: pauses all communication (and stops acknowledging messages)
    void pause(bool paused);

private:
    SPIClass& _spi;
    uint8_t _cs;
    uint8_t _int_pin;
    
    QueueHandle_t _tx_queue;
    QueueHandle_t _rx_queue;

    MCP2515_Lite_Speed _next_speed;
    volatile bool _speed_change_pending = false;
    volatile bool _pause_requested = false;
    volatile bool _paused = false;
    volatile bool _rx_overflow = false;

    // Background task for handling sequential blocking transfers
    TaskHandle_t _can_task_handle = nullptr;
    static void canTask(void* pvParameters);    

    // Internal SPI helpers
    void spiTransactionBlocking(const uint8_t* tx_data, uint8_t* rx_data, size_t length);
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void modifyRegister(uint8_t reg, uint8_t mask, uint8_t data);

    bool reset();
    void applySpeedConfig(const MCP2515_Lite_Speed& speed);

    // ISR handler for the CAN interrupt pin
    static void IRAM_ATTR mcp2515_isr_handler(void* arg);
};
