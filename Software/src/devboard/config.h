#ifndef __CONFIG_H__
#define __CONFIG_H__

// PIN mappings, do not change these unless you are adding on extra hardware to the PCB
#define PIN_5V_EN 16
#define CAN_TX_PIN 27
#define CAN_RX_PIN 26
#define CAN_SE_PIN 23

#define RS485_EN_PIN 17  // 17 /RE
#define RS485_TX_PIN 22  // 21
#define RS485_RX_PIN 21  // 22
#define RS485_SE_PIN 19  // 22 /SHDN

#ifdef DUAL_CAN
#define MCP2515_SCK 12   // SCK input of MCP2515
#define MCP2515_MOSI 5   // SDI input of MCP2515
#define MCP2515_MISO 34  // SDO output of MCP2515 | Pin 34 is input only, without pullup/down resistors
#define MCP2515_CS 18    // CS input of MCP2515
#define MCP2515_INT 35   // INT output of MCP2515 |  | Pin 35 is input only, without pullup/down resistors
#endif

#ifdef CAN_FD
#define MCP2517_SCK 12  // SCK input of MCP2517
#define MCP2517_SDI 5   // SDI input of MCP2517
#define MCP2517_SDO 34  // SDO output of MCP2517
#define MCP2517_CS 18   // CS input of MCP2517
#define MCP2517_INT 35  // INT output of MCP2517
#endif

#ifdef CONTACTOR_CONTROL
#define POSITIVE_CONTACTOR_PIN 32
#define NEGATIVE_CONTACTOR_PIN 33
#define PRECHARGE_PIN 25
#endif

#ifdef BMW_I3_BATTERY
#define WUP_PIN 25
#endif

#define SD_MISO_PIN 2
#define SD_MOSI_PIN 15
#define SD_SCLK_PIN 14
#define SD_CS_PIN 13
#define WS2812_PIN 4

// LED definitions for the board, in order of "priority", DONT CHANGE!
#define GREEN 0
#define YELLOW 1
#define BLUE 2
#define RED 3
#define TEST_ALL_COLORS 10

// Inverter definitions
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5
#define DISCHARGING 1
#define CHARGING 2

// Common definitions
#define MAX_AMOUNT_CELLS 192
#define MAX_CAN_FAILURES 500  // Amount of malformed CAN messages to allow before raising a warning

#define INTERVAL_10_MS 10
#define INTERVAL_20_MS 20
#define INTERVAL_30_MS 30
#define INTERVAL_50_MS 50
#define INTERVAL_100_MS 100
#define INTERVAL_200_MS 200
#define INTERVAL_500_MS 500
#define INTERVAL_640_MS 640
#define INTERVAL_1_S 1000
#define INTERVAL_2_S 2000
#define INTERVAL_5_S 5000
#define INTERVAL_10_S 10000
#define INTERVAL_60_S 60000

#define INTERVAL_10_MS_DELAYED 15
#define INTERVAL_20_MS_DELAYED 30
#define INTERVAL_30_MS_DELAYED 40
#define INTERVAL_100_MS_DELAYED 120
#define INTERVAL_500_MS_DELAYED 550

#define BOOTUP_TIME 1000  // Time in ms it takes before system is considered fully started up

#endif
