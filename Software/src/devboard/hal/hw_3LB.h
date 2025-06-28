#ifndef __HW_3LB_H__
#define __HW_3LB_H__

// Board boot-up time
#define BOOTUP_TIME 1000  // Time in ms it takes before system is considered fully started up

// Core assignment
#define CORE_FUNCTION_CORE 1
#define MODBUS_CORE 0
#define WIFI_CORE 0

// RS485
//#define PIN_5V_EN 16
//#define RS485_EN_PIN 17  // 17 /RE
#define RS485_TX_PIN 1  // 21
#define RS485_RX_PIN 3  // 22
//#define RS485_SE_PIN 19  // 22 /SHDN

// CAN settings. CAN_2 is not defined as it can be either MCP2515 or MCP2517, defined by the user settings
#define CAN_1_TYPE ESP32CAN

// CAN1 PIN mappings, do not change these unless you are adding on extra hardware to the PCB
#define CAN_TX_PIN GPIO_NUM_27
#define CAN_RX_PIN GPIO_NUM_26
//#define CAN_SE_PIN 23

// CAN2 defines below

// CAN_ADDON defines
#define MCP2515_SCK 12   // SCK input of MCP2515
#define MCP2515_MOSI 5   // SDI input of MCP2515
#define MCP2515_MISO 34  // SDO output of MCP2515 | Pin 34 is input only, without pullup/down resistors
#define MCP2515_CS 18    // CS input of MCP2515
#define MCP2515_INT 35   // INT output of MCP2515 |  | Pin 35 is input only, without pullup/down resistors

// CANFD_ADDON defines
#define MCP2517_SCK 17  // SCK input of MCP2517
#define MCP2517_SDI 23  // SDI input of MCP2517
#define MCP2517_SDO 39  // SDO output of MCP2517
#define MCP2517_CS 21   // CS input of MCP2517 //21 or 22
#define MCP2517_INT 34  // INT output of MCP2517 //34 or 35

// CHAdeMO support pin dependencies
#define CHADEMO_PIN_2 12
#define CHADEMO_PIN_10 5
#define CHADEMO_PIN_7 34
#define CHADEMO_PIN_4 35
#define CHADEMO_LOCK 18

// Contactor handling
#define POSITIVE_CONTACTOR_PIN 32
#define NEGATIVE_CONTACTOR_PIN 33
#define PRECHARGE_PIN 25
#define BMS_POWER 2
#define SECOND_BATTERY_CONTACTORS_PIN 13

// SMA CAN contactor pins
#define INVERTER_CONTACTOR_ENABLE_PIN 36

// Automatic precharging
#define HIA4V1_PIN 25
#define INVERTER_DISCONNECT_CONTACTOR_PIN 32

// SD card
//#define SD_MISO_PIN 2
//#define SD_MOSI_PIN 15
//#define SD_SCLK_PIN 14
//#define SD_CS_PIN 13

// LED
#define LED_PIN 4
#define LED_MAX_BRIGHTNESS 40

// Equipment stop pin
#define EQUIPMENT_STOP_PIN 35

// BMW_I3_BATTERY wake up pin
#define WUP_PIN1 GPIO_NUM_25  // Wake up pin for battery 1
#define WUP_PIN2 GPIO_NUM_32  // Wake up pin for battery 2

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#ifdef CHADEMO_BATTERY
#ifdef CAN_ADDON
#error CHADEMO and CAN_ADDON cannot coexist due to overlapping GPIO pin usage
#endif
#endif

#ifdef EQUIPMENT_STOP_BUTTON
#ifdef CAN_ADDON
#error EQUIPMENT_STOP_BUTTON and CAN_ADDON cannot coexist due to overlapping GPIO pin usage
#endif
#ifdef CANFD_ADDON
#error EQUIPMENT_STOP_BUTTON and CANFD_ADDON cannot coexist due to overlapping GPIO pin usage
#endif
#ifdef CHADEMO_BATTERY
#error EQUIPMENT_STOP_BUTTON and CHADEMO_BATTERY cannot coexist due to overlapping GPIO pin usage
#endif
#endif

#ifdef BMW_I3_BATTERY
#if defined(CONTACTOR_CONTROL) && defined(WUP_PIN1)
#error GPIO PIN 25 cannot be used for both BMWi3 Wakeup and contactor control. Disable CONTACTOR_CONTROL
#endif
#if defined(CONTACTOR_CONTROL) && defined(WUP_PIN2)
#error GPIO PIN 32 cannot be used for both BMWi3 Wakeup and contactor control. Disable CONTACTOR_CONTROL
#endif
#endif

#endif
