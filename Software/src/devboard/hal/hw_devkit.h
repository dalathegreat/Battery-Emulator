#ifndef __HW_DEVKIT_H__
#define __HW_DEVKIT_H__

#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "../../../USER_SETTINGS_ESP32S3.h"
#endif

/*
ESP32 DevKit V1 development board with 30 pins.
For ESP32, see: https://lastminuteengineers.com/esp32-pinout-reference/.
For ESP32-S3, see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

The pin layout below supports the following:
- 1x RS485
- 2x CAN (1x via SN65HVD230 (UART), 1x via MCP2515 (SPI))
- 1x CANFD (via MCP2518FD (SPI))

For ESP32-S3: Pin configuration can be customized in USER_SETTINGS_ESP32S3.h
*/

class DevKitHal : public Esp32Hal {
 public:
#ifdef CONFIG_IDF_TARGET_ESP32S3
  const char* name() { return "ESP32-S3 DevKitC"; }
#else
  const char* name() { return "ESP32 DevKit V1"; }
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
  // ESP32-S3 pin configuration (from USER_SETTINGS_ESP32S3.h)
  virtual gpio_num_t RS485_TX_PIN() { return (gpio_num_t)ESP32S3_RS485_TX_PIN; }
  virtual gpio_num_t RS485_RX_PIN() { return (gpio_num_t)ESP32S3_RS485_RX_PIN; }

  virtual gpio_num_t CAN_TX_PIN() { return (gpio_num_t)ESP32S3_CAN_TX_PIN; }
  virtual gpio_num_t CAN_RX_PIN() { return (gpio_num_t)ESP32S3_CAN_RX_PIN; }

  // CAN_ADDON
  virtual gpio_num_t MCP2515_SCK() { return (gpio_num_t)ESP32S3_MCP2515_SCK; }
  virtual gpio_num_t MCP2515_MOSI() { return (gpio_num_t)ESP32S3_MCP2515_MOSI; }
  virtual gpio_num_t MCP2515_MISO() { return (gpio_num_t)ESP32S3_MCP2515_MISO; }
  virtual gpio_num_t MCP2515_CS() { return (gpio_num_t)ESP32S3_MCP2515_CS; }
  virtual gpio_num_t MCP2515_INT() { return (gpio_num_t)ESP32S3_MCP2515_INT; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return (gpio_num_t)ESP32S3_MCP2517_SCK; }
  virtual gpio_num_t MCP2517_SDI() { return (gpio_num_t)ESP32S3_MCP2517_SDI; }
  virtual gpio_num_t MCP2517_SDO() { return (gpio_num_t)ESP32S3_MCP2517_SDO; }
  virtual gpio_num_t MCP2517_CS() { return (gpio_num_t)ESP32S3_MCP2517_CS; }
  virtual gpio_num_t MCP2517_INT() { return (gpio_num_t)ESP32S3_MCP2517_INT; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return (gpio_num_t)ESP32S3_POSITIVE_CONTACTOR_PIN; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return (gpio_num_t)ESP32S3_NEGATIVE_CONTACTOR_PIN; }
  virtual gpio_num_t PRECHARGE_PIN() { return (gpio_num_t)ESP32S3_PRECHARGE_PIN; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return (gpio_num_t)ESP32S3_SECOND_BATTERY_CONTACTORS_PIN; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return (gpio_num_t)ESP32S3_HIA4V1_PIN; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return (gpio_num_t)ESP32S3_INVERTER_DISCONNECT_CONTACTOR_PIN; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return (gpio_num_t)ESP32S3_INVERTER_CONTACTOR_ENABLE_PIN; }
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return (gpio_num_t)ESP32S3_INVERTER_CONTACTOR_ENABLE_LED_PIN; }

  // LED
  virtual gpio_num_t LED_PIN() { return (gpio_num_t)ESP32S3_LED_PIN; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return ESP32S3_LED_MAX_BRIGHTNESS; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return (gpio_num_t)ESP32S3_EQUIPMENT_STOP_PIN; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return (gpio_num_t)ESP32S3_WUP_PIN1; }
  virtual gpio_num_t WUP_PIN2() { return (gpio_num_t)ESP32S3_WUP_PIN2; }
#else
  // ESP32 classic pin configuration
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_1; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_3; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_27; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_26; }

  // CAN_ADDON
  // SCK input of MCP2515
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_22; }
  // SDI input of MCP2515
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_21; }
  // SDO output of MCP2515
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_19; }
  // CS input of MCP2515
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_18; }
  // INT output of MCP2515
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_23; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_33; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_32; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_35; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_25; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_34; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_5; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_16; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_32; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_4; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_5; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_14; }

  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_2; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_12; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_25; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_32; }
#endif

  std::vector<comm_interface> available_interfaces() {
    return {
        comm_interface::Modbus,
        comm_interface::RS485,
        comm_interface::CanNative,
    };
  }
};

#endif  // __HW_DEVKIT_H__
