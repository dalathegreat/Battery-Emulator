#ifndef __HW_STARK06_H__
#define __HW_STARK06_H__

// Board boot-up time
#define BOOTUP_TIME 1000  // Time in ms it takes before system is considered fully started up

// Core assignment
#define CORE_FUNCTION_CORE 1
#define MODBUS_CORE 0
#define WIFI_CORE 0

// RS485
// #define PIN_5V_EN 16 	// No function, GPIO 16 used instead as MCP_SCK
// #define RS485_EN_PIN 17  // RE, No function, GPIO 17 is instead available as extra GPIO via pin header
#define RS485_TX_PIN 22
#define RS485_RX_PIN 21
// #define RS485_SE_PIN 19  // No function, GPIO 19 is instead available as extra GPIO via pin header

// CAN settings. CAN_2 is not defined as it can be either MCP2515 or MCP2517, defined by the user settings
#define CAN_1_TYPE ESP32CAN

// CAN1 PIN mappings, do not change these unless you are adding on extra hardware to the PCB
#define CAN_TX_PIN GPIO_NUM_27
#define CAN_RX_PIN GPIO_NUM_26
// #define CAN_SE_PIN 23 // (No function, GPIO 23 used instead as MCP_SCK)

// CAN2 defines below

// DUAL_CAN defines
//#define MCP2515_SCK 12   // SCK input of MCP2515
//#define MCP2515_MOSI 5   // SDI input of MCP2515
//#define MCP2515_MISO 34  // SDO output of MCP2515 | Pin 34 is input only, without pullup/down resistors
//#define MCP2515_CS 18    // CS input of MCP2515
//#define MCP2515_INT 35   // INT output of MCP2515 |  | Pin 35 is input only, without pullup/down resistors

// CAN_FD defines
#define MCP2517_SCK 16  // SCK input of MCP2517 (Changed from 12 to 16 since 12 can be used for JTAG TDI)
#define MCP2517_SDI 5   // SDI input of MCP2517
#define MCP2517_SDO 34  // SDO output of MCP2517
#define MCP2517_CS 18   // CS input of MCP2517
#define MCP2517_INT 35  // INT output of MCP2517

// Contactor handling
#define POSITIVE_CONTACTOR_PIN 32
#define NEGATIVE_CONTACTOR_PIN 33
#define PRECHARGE_PIN 25
#define BMS_POWER 23  // Also connected to MCP_SCK

// SD card
//#define SD_MISO_PIN 2
//#define SD_MOSI_PIN 15
//#define SD_SCLK_PIN 14
//#define SD_CS_PIN 13

// LED
#define LED_PIN 4
#define LED_MAX_BRIGHTNESS 40

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
