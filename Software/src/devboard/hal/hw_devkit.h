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

class DevKitHal : public Esp32Hal {
 public:
  const char* name() { return "ESP32 DevKit V1"; }

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

  std::vector<comm_interface> available_interfaces() {
    return {
        comm_interface::Modbus,
        comm_interface::RS485,
        comm_interface::CanNative,
    };
  }
};

#endif  // __HW_DEVKIT_H__
