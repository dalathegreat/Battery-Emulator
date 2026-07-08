#ifndef __HW_WAVESHARE_H__
#define __HW_WAVESHARE_H__

#include "hal.h"

class WaveshareS3Rs485CanHal : public Esp32Hal {
 public:
  const char* name() { return "Waveshare ESP32-S3-RS485-CAN"; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_15; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_16; }
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_NC; }

  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_18; }

  // SP3485 DE and /RE are tied together on this board.
  // HIGH = transmit, LOW = receive. Do not use RS485_EN_PIN for this signal.
  virtual gpio_num_t RS485_DE_PIN() { return GPIO_NUM_21; }

  virtual gpio_num_t RS485_EN_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_SE_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t PIN_5V_EN() { return GPIO_NUM_NC; }

  virtual gpio_num_t LED_PIN() {
    if (user_selected_gpioopt6 == GPIOOPT6::I2C_DISPLAY_SSD1306) {
      return GPIO_NUM_NC;
    }
    return GPIO_NUM_2;
  }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_3; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_4; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_5; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_6; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_8; }

  // Pins to be latched across a reset/OTA reboot (RTC-capable pins only): BMS_POWER is GPIO6
  virtual std::vector<gpio_num_t> reset_hold_pins() { return {GPIO_NUM_6}; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_7; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_8; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_9; }  //Collides with SMA contactor input

  // Momentary push-button that can be long-pressed at runtime to start the Wi-Fi AP.
  virtual gpio_num_t AP_BUTTON_PIN() { return GPIO_NUM_0; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_5; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_3; }

  // SMA CAN contactor pins (collides with WUP2)
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_9; }

  // i2c display
  virtual gpio_num_t DISPLAY_SDA_PIN() {
    if (user_selected_gpioopt6 == GPIOOPT6::I2C_DISPLAY_SSD1306) {
      return GPIO_NUM_1;
    }
    return GPIO_NUM_NC;
  }
  virtual gpio_num_t DISPLAY_SCL_PIN() {
    if (user_selected_gpioopt6 == GPIOOPT6::I2C_DISPLAY_SSD1306) {
      return GPIO_NUM_2;
    }
    return GPIO_NUM_NC;
  }

  // CANFD add-on defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_10; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_11; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_13; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_14; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative};
  }

  virtual const char* name_for_comm_interface(comm_interface comm) {
    switch (comm) {
      case comm_interface::CanNative:
        return "CAN (Native)";
      case comm_interface::CanFdNative:
        return "";
      case comm_interface::CanAddonMcp2515:
        return "";
      case comm_interface::CanFdAddonMcp2518:
        return "CAN FD (MCP2518 add-on)";
      case comm_interface::Modbus:
        return "Modbus";
      case comm_interface::RS485:
        return "RS485";
      case comm_interface::Highest:
        return "";
    }
    return Esp32Hal::name_for_comm_interface(comm);
  }
};

#define HalClass WaveshareS3Rs485CanHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
