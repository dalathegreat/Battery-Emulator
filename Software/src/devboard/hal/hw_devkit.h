#ifndef __HW_DEVKIT_H__
#define __HW_DEVKIT_H__

/*
ESP32 DevKit V1 development board with 30 pins.
For more information, see: https://lastminuteengineers.com/esp32-pinout-reference/.

The pin layout below supports the following:
- 1x RS485
- 2x CAN (1x via SN65HVD230 (UART), 1x via MCP2515 (SPI))
- 1x CANFD (via MCP2518FD (SPI))
*/

// Board boot-up time
#define BOOTUP_TIME 1000  // Time in ms it takes before system is considered fully started up

// Core assignment
#define CORE_FUNCTION_CORE 1
#define MODBUS_CORE 0
#define WIFI_CORE 0

// RS485
#define RS485_TX_PIN GPIO_NUM_1
#define RS485_RX_PIN GPIO_NUM_3

// CAN settings
#define CAN_1_TYPE ESP32CAN
//#define CAN_2_TYPE MCP2515
//#define CAN_3_TYPE MCP2518FD

// CAN1 PIN mappings, do not change these unless you are adding on extra hardware to the PCB
#define CAN_TX_PIN GPIO_NUM_27
#define CAN_RX_PIN GPIO_NUM_26

// CAN_ADDON defines
#define MCP2515_SCK GPIO_NUM_22   // SCK input of MCP2515
#define MCP2515_MOSI GPIO_NUM_21  // SDI input of MCP2515
#define MCP2515_MISO GPIO_NUM_19  // SDO output of MCP2515
#define MCP2515_CS GPIO_NUM_18    // CS input of MCP2515
#define MCP2515_INT GPIO_NUM_23   // INT output of MCP2515

// CANFD_ADDON defines
#define MCP2517_SCK GPIO_NUM_33  // SCK input of MCP2517
#define MCP2517_SDI GPIO_NUM_32  // SDI input of MCP2517
#define MCP2517_SDO GPIO_NUM_35  // SDO output of MCP2517 | Pin 35 is input only, without pullup/down resistors
#define MCP2517_CS GPIO_NUM_25   // CS input of MCP2517
#define MCP2517_INT GPIO_NUM_34  // INT output of MCP2517 | Pin 34 is input only, without pullup/down resistors

// Contactor handling
#define POSITIVE_CONTACTOR_PIN GPIO_NUM_5
#define NEGATIVE_CONTACTOR_PIN GPIO_NUM_16
#define PRECHARGE_PIN GPIO_NUM_17
#define SECOND_BATTERY_CONTACTORS_PIN GPIO_NUM_32
// SMA CAN contactor pins
#define INVERTER_CONTACTOR_ENABLE_PIN GPIO_NUM_14

// LED
#define LED_PIN GPIO_NUM_4
#define LED_MAX_BRIGHTNESS 40
#define INVERTER_CONTACTOR_ENABLE_LED_PIN GPIO_NUM_2

// Equipment stop pin
#define EQUIPMENT_STOP_PIN GPIO_NUM_12

// Automatic precharging
#define HIA4V1_PIN GPIO_NUM_17
#define INVERTER_DISCONNECT_CONTACTOR_PIN GPIO_NUM_5

// BMW_I3_BATTERY wake up pin
#define WUP_PIN1 GPIO_NUM_25  // Wake up pin for battery 1
#define WUP_PIN2 GPIO_NUM_32  // Wake up pin for battery 2

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif  // HW_CONFIGURED

#ifdef CHADEMO_BATTERY
#error CHADEMO pins are not defined for this hardware.
#endif  // CHADEMO_BATTERY

#ifdef BMW_I3_BATTERY
#if defined(WUP_PIN1) && defined(CANFD_ADDON)
#error GPIO PIN 25 cannot be used for both BMWi3 Wakeup and a CANFD addon board using these pins. Choose between BMW_I3_BATTERY and CANFD_ADDON
#endif  // defined(WUP_PIN1) && defined(CANFD_ADDON)
#if defined(WUP_PIN2) && defined(CANFD_ADDON)
#error GPIO PIN 32 cannot be used for both BMWi3 Wakeup and a CANFD addon board using these pins. Choose between BMW_I3_BATTERY and CANFD_ADDON
#endif  // defined(WUP_PIN2) && defined(CANFD_ADDON)
#endif  // BMW_I3_BATTERY

#endif  // __HW_DEVKIT_H__
