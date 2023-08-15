#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Select battery used */
#define BATTERY_TYPE_LEAF         // See NISSAN-LEAF-BATTERY.h for more LEAF battery settings
//#define TESLA_MODEL_3_BATTERY   // See TESLA-MODEL-3-BATTERY.h for more Tesla battery settings
//#define RENAULT_ZOE_BATTERY     // See RENAULT-ZOE-BATTERY.h for more Zoe battery settings
//#define IMIEV_ION_CZERO_BATTERY // See IMIEV-CZERO-ION-BATTERY.h for more triplet battery settings
//#define CHADEMO                 // See CHADEMO.h for more Chademo related settings

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
//#define MODBUS_BYD      //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define CAN_BYD       //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
#define SOLAX_CAN       //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus
//#define PYLON_CAN		//Enable this line to emulate a "Pylontech battery" over CAN bus

#define DUAL_CAN //Enable this line to activate an isolated secondary CAN Bus using MCP2515 controller (Needed for FoxESS inverts)

// PIN
#define PIN_5V_EN 16

#define CAN_TX_PIN 27
#define CAN_RX_PIN 26
#define CAN_SE_PIN 23

#define MCP2515_SCK  12 // SCK input of MCP2515
#define MCP2515_MOSI  5 // SDI input of MCP2515
#define MCP2515_MISO 34 // SDO output of MCP2515 | Pin 34 is input only, without pullup/down resistors
#define MCP2515_CS   18 // CS input of MCP2515
#define MCP2515_INT  35 // INT output of MCP2515 |  | Pin 35 is input only, without pullup/down resistors

#define RS485_EN_PIN 17 // 17 /RE
#define RS485_TX_PIN 22 // 21
#define RS485_RX_PIN 21 // 22
#define RS485_SE_PIN 19 // 22 /SHDN

#define POSITIVE_CONTACTOR_PIN 32
#define NEGATIVE_CONTACTOR_PIN 33
#define PRECHARGE_PIN 25

#define SD_MISO_PIN 2
#define SD_MOSI_PIN 15
#define SD_SCLK_PIN 14
#define SD_CS_PIN 13

#define WS2812_PIN 4

#endif