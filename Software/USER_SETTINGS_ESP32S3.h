#ifndef __USER_SETTINGS_ESP32S3_H__
#define __USER_SETTINGS_ESP32S3_H__

/*
ESP32-S3 DevKitC Pin Configuration
For more information, see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

Note: ESP32-S3 has different GPIO availability than ESP32.
Valid GPIO pins: 0-21, 26-48 (with some restrictions on 26-32)
Pins 22-25 do not exist on ESP32-S3.

MODIFY THESE PIN DEFINITIONS TO MATCH YOUR HARDWARE SETUP
*/

// RS485 Communication
#define ESP32S3_RS485_TX_PIN 1
#define ESP32S3_RS485_RX_PIN 3

// Native CAN (via SN65HVD230 or similar transceiver)
#define ESP32S3_CAN_TX_PIN 17
#define ESP32S3_CAN_RX_PIN 18

// CAN Add-on via MCP2515 (SPI)
#define ESP32S3_MCP2515_SCK 12
#define ESP32S3_MCP2515_MOSI 11
#define ESP32S3_MCP2515_MISO 13
#define ESP32S3_MCP2515_CS 10
#define ESP32S3_MCP2515_INT 14

// CANFD Add-on via MCP2517/2518 (SPI)
#define ESP32S3_MCP2517_SCK 36
#define ESP32S3_MCP2517_SDI 35
#define ESP32S3_MCP2517_SDO 37
#define ESP32S3_MCP2517_CS 38
#define ESP32S3_MCP2517_INT 39

// Contactor Control
#define ESP32S3_POSITIVE_CONTACTOR_PIN 5
#define ESP32S3_NEGATIVE_CONTACTOR_PIN 6
#define ESP32S3_PRECHARGE_PIN 7
#define ESP32S3_SECOND_BATTERY_CONTACTORS_PIN 15

// Automatic Precharging
#define ESP32S3_HIA4V1_PIN 4
#define ESP32S3_INVERTER_DISCONNECT_CONTACTOR_PIN 5

// SMA CAN Contactor
#define ESP32S3_INVERTER_CONTACTOR_ENABLE_PIN 16
#define ESP32S3_INVERTER_CONTACTOR_ENABLE_LED_PIN 2

// LED (GPIO 48 is the RGB LED on ESP32-S3-DevKitC)
#define ESP32S3_LED_PIN 48
#define ESP32S3_LED_MAX_BRIGHTNESS 40

// Equipment Stop Button
#define ESP32S3_EQUIPMENT_STOP_PIN 9

// Battery Wake Up Pins
#define ESP32S3_WUP_PIN1 40
#define ESP32S3_WUP_PIN2 41

#endif  // __USER_SETTINGS_ESP32S3_H__
