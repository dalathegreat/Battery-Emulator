#ifndef __HW_STARK_H__
#define __HW_STARK_H__

/*
Stark CMR v1 - DIN-rail module with 4 power outputs, 1 x rs485, 1 x can and 1 x can-fd channel.
For more information on this board visit the project discord or contact johan@redispose.se

GPIOs on extra header
* GPIO  2
* GPIO 17 (only available if can channel 2 is inactive)
* GPIO 19
* GPIO 14 (JTAG TMS)
* GPIO 12 (JTAG TDI)
* GPIO 13 (JTAG TCK)
* GPIO 15 (JTAG TDO)
*/

// Board boot-up time
#define BOOTUP_TIME 5000  // Time in ms it takes before system is considered fully started up

// Core assignment
#define CORE_FUNCTION_CORE 1
#define MODBUS_CORE 0
#define WIFI_CORE 0

// RS485
// #define PIN_5V_EN 16     // Not needed, GPIO 16 has hardware pullup for PSRAM compatibility
// #define RS485_EN_PIN 17  // Not needed, GPIO 17 is used as SCK input of MCP2517
#define RS485_TX_PIN 22
#define RS485_RX_PIN 21
// #define RS485_SE_PIN 19  // Not needed, GPIO 19 is available as extra GPIO via pin header

// CAN settings
#define CAN_1_TYPE ESP32CAN

// CAN1 PIN mappings, do not change these unless you are adding on extra hardware to the PCB
#define CAN_TX_PIN GPIO_NUM_27
#define CAN_RX_PIN GPIO_NUM_26
// #define CAN_SE_PIN 23 // (No function, GPIO 23 used instead as MCP_SCK)

// CANFD_ADDON defines
#define MCP2517_SCK 17  // SCK input  of MCP2517
#define MCP2517_SDI 5   // SDI input  of MCP2517
#define MCP2517_SDO 34  // SDO output of MCP2517
#define MCP2517_CS 18   // CS  input  of MCP2517
#define MCP2517_INT 35  // INT output of MCP2517

// Contactor handling
#define POSITIVE_CONTACTOR_PIN 32
#define NEGATIVE_CONTACTOR_PIN 33
#define PRECHARGE_PIN 25
#define BMS_POWER 23
#define SECOND_BATTERY_CONTACTORS_PIN 19  //Available as extra GPIO via pin header

// Automatic precharging
#define HIA4V1_PIN 19  //Available as extra GPIO via pin header
#define INVERTER_DISCONNECT_CONTACTOR_PIN 25

// SMA CAN contactor pins
#define INVERTER_CONTACTOR_ENABLE_PIN 2

// LED
#define LED_PIN 4
#define LED_MAX_BRIGHTNESS 40

// Equipment stop pin
#define EQUIPMENT_STOP_PIN 2

// BMW_I3_BATTERY wake up pin
#define WUP_PIN1 GPIO_NUM_25  // Wake up pin for battery 1
#define WUP_PIN2 GPIO_NUM_32  // Wake up pin for battery 2

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif  // HW_CONFIGURED

#ifdef BMW_I3_BATTERY
#if defined(CONTACTOR_CONTROL) && defined(WUP_PIN1)
#error GPIO PIN 25 cannot be used for both BMWi3 Wakeup and contactor control. Disable CONTACTOR_CONTROL
#endif
#if defined(CONTACTOR_CONTROL) && defined(WUP_PIN2)
#error GPIO PIN 32 cannot be used for both BMWi3 Wakeup and contactor control. Disable CONTACTOR_CONTROL
#endif
#endif  // BMW_I3_BATTERY

#endif  // __HW_STARK_H__
