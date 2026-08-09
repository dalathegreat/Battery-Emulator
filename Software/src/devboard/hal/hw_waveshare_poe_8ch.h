#ifndef __HW_WAVESHARE_8CH_POE_H__
#define __HW_WAVESHARE_8CH_POE_H__

#include "hal.h"

#include "../utils/types.h"

/*
Pinout reference:

*/

class WavesharePoE8ChHal : public Esp32Hal {
 public:
  const char* name() { return "Waveshare ESP32S3-PoE-8CH"; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_7; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_6; }

  // Note: these are used by the bootloader UART, so there'll be some bootloader
  // chatter sent down the RS485 bus when the chip starts up. Hopefully this
  // won't matter (it'll be 115200 baud, so much higher than the normal Modbus
  // 9600 baud) - if it's a problem we can burn fuses to disable it.
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_18; }

  // CANFD_ADDON defines for MCP2518
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_48; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_47; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_45; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_21; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_3; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_38; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // i2c display
  virtual gpio_num_t DISPLAY_SDA_PIN() {
    if (user_selected_gpioopt1 == GPIOOPT1::I2C_DISPLAY_SSD1306) {
      return GPIO_NUM_42;
    }
    return GPIO_NUM_NC;
  }
  virtual gpio_num_t DISPLAY_SCL_PIN() {
    if (user_selected_gpioopt1 == GPIOOPT1::I2C_DISPLAY_SSD1306) {
      return GPIO_NUM_41;
    }
    return GPIO_NUM_NC;
  }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() {
    if (user_selected_gpioopt1 == GPIOOPT1::ESTOP_BMS_POWER) {
      return GPIO_NUM_1;
    }
    return GPIO_NUM_36;
  }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() {
    if (user_selected_gpioopt1 == GPIOOPT1::DEFAULT_OPT) {
      // WUP1 on top port
      return GPIO_NUM_1;
    }
    return GPIO_NUM_13;
  }
  virtual gpio_num_t WUP_PIN2() {
    if (user_selected_gpioopt1 == GPIOOPT1::DEFAULT_OPT) {
      // WUP2 on top port
      return GPIO_NUM_2;
    }
    return GPIO_NUM_14;
  }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative, comm_interface::CanFdAddonMcp2518};
  }

  virtual const char* name_for_comm_interface(comm_interface comm) {
    switch (comm) {
      case comm_interface::CanNative:
        return "CAN 1 (Native)";
      case comm_interface::CanFdNative:
        return "";
      case comm_interface::CanAddonMcp2515:
        return "";
      case comm_interface::CanFdAddonMcp2518:
        return "CAN FD (MCP2518 add-on)";
      case comm_interface::CanFdAddonMcp2518_2:
        return "";
      case comm_interface::Modbus:
        return "Modbus (Add-on)";
      case comm_interface::RS485:
        return "RS485 (Add-on)";
      case comm_interface::Highest:
        return "";
    }
    return Esp32Hal::name_for_comm_interface(comm);
  }
};

#define HalClass Waveshare8CHPOEHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
