#ifndef __HW_LILYGO2CAN_H__
#define __HW_LILYGO2CAN_H__

#include "hal.h"
#include "../utils/types.h"

/*
=========================================================
 LilyGo T-2CAN Pinout Reference
=========================================================
The 2CAN has four GPIOs on the top (43/44 and 1/2 on each of the SH-1mm
connectors) and 21 on the bottom (on the 2x13 header). GPIO35-37 aren't usable
if opi PSRAM is enabled.

[Top Connectors]
43:RS485_TX         1:WUP1/BMS_POWER/ESTOP/SDA(QWIIC)
44:RS485_RX         2:WUP2/LED/SCL(QWIIC)

[Bottom 2x13 Header]
 3.3V                 GND
 5V                   GND
x35:LED                39:INT
 38:SCK                42:SDI
x37:SDO                41:nCS
x36:ESTOP              40:REFRESH_BTN (E-Paper) / CHADEMO_LOCK
 16:EPD_SCK             4:BAT3_CTRS / EPD_BUSY
 15:EPD_MOSI            5:BAT2_CTRS
 45:EPD_DC             48:POS_CTR
 47:EPD_RST            21:PRECHARGE
 14:HV_INV_DIS/[WUP2]  17:NEG_CTR
 18:HV_PRE/[WUP1]     GND
 46:INV_CTR_EN / EPD_CS 3:BMS_POWER
=========================================================
*/

class LilyGo2CANHal : public Esp32Hal {
 public:
  const char* name() { return "LilyGo T_2CAN"; }

  virtual void set_default_configuration_values() {
    BatteryEmulatorSettingsStore settings;
    if (!settings.settingExists("CANFREQ")) {
      settings.saveUInt("CANFREQ", 16);
    }
  }

  // ==========================================
  // Communication Interfaces
  // ==========================================
  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_7; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_6; }

  // Note: Used by the bootloader UART (115200 baud). Bootloader chatter will be sent 
  // down the RS485 bus on startup. Usually harmless for Modbus (9600 baud).
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_43; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_44; }

  // Built-in MCP2515 (CAN Add-on)
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_11; }
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_13; }
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_10; }
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_8; }
  virtual gpio_num_t MCP2515_RST() { return GPIO_NUM_9; }

  // MCP2517 (CAN-FD Add-on)
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_38; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_42; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_37; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_41; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_39; }

  // ==========================================
  // Core Contactor Handling
  // ==========================================
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_48; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_21; }

  // Automatic precharging & Inverter disconnect
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_18; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_14; }

  // Second Battery Contactor
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_5; }

  // Triple Battery Contactor (Shared with EPD_BUSY)
  virtual gpio_num_t TRIPLE_BATTERY_CONTACTORS_PIN() { 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_NC; // Disable to free GPIO4 for E-Paper BUSY signal
    }
    return GPIO_NUM_4; 
  }

  // SMA Inverter Contactor (Shared with EPD_CS)
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_NC; // Disable to free GPIO46 for E-Paper CS signal
    }
    return GPIO_NUM_46; 
  }

  // ==========================================
  // Multi-Function Pins (Configurable via Web)
  // ==========================================

  // BMS Power Control
  virtual gpio_num_t BMS_POWER() {
    if (user_selected_display_type == DisplayType::OLED_I2C) {
      return GPIO_NUM_3; // Force default pin if QWIIC is used for OLED
    }
    if (user_selected_gpioopt1 == GPIOOPT1::ESTOP_BMS_POWER) {
      return GPIO_NUM_2;
    }
    return GPIO_NUM_3;
  }

  // Equipment Stop Pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() {
    if (user_selected_display_type == DisplayType::OLED_I2C) {
      return GPIO_NUM_36; // Force default pin if QWIIC is used for OLED
    }
    if (user_selected_gpioopt1 == GPIOOPT1::ESTOP_BMS_POWER) {
      return GPIO_NUM_1;
    }
    return GPIO_NUM_36;
  }

  // Wake Up Pins (WUP)
  virtual gpio_num_t WUP_PIN1() {
    if (user_selected_display_type == DisplayType::OLED_I2C) {
        return GPIO_NUM_18; // Force fallback pin to avoid I2C collision
    }
    if (user_selected_gpioopt1 == GPIOOPT1::DEFAULT_OPT) {
      return GPIO_NUM_1;
    }
    return GPIO_NUM_18;
  }
  
  virtual gpio_num_t WUP_PIN2() {
    if (user_selected_display_type == DisplayType::OLED_I2C) {
        return GPIO_NUM_14; // Force fallback pin to avoid I2C collision
    }
    if (user_selected_gpioopt1 == GPIOOPT1::DEFAULT_OPT) {
      return GPIO_NUM_2;
    }
    return GPIO_NUM_14;
  }

  // ==========================================
  // Displays & Indicators
  // ==========================================

  // WS2812B Status LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_35; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // I2C OLED (Uses QWIIC connector - GPIO1 & GPIO2)
  virtual gpio_num_t DISPLAY_SDA_PIN() {
    if (user_selected_display_type == DisplayType::OLED_I2C || 
        user_selected_gpioopt1 == GPIOOPT1::I2C_DISPLAY_SSD1306) {
        return GPIO_NUM_1;
    }
    return GPIO_NUM_NC;
  }
  
  virtual gpio_num_t DISPLAY_SCL_PIN() {
    if (user_selected_display_type == DisplayType::OLED_I2C || 
        user_selected_gpioopt1 == GPIOOPT1::I2C_DISPLAY_SSD1306) {
        return GPIO_NUM_2;
    }
    return GPIO_NUM_NC;
  }

  // SPI E-Paper 4.2" Display
  virtual gpio_num_t EPD_BUSY_PIN() { 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_4; // Overrides BAT3_CTRS & CHADEMO_PIN_4
    }
    return GPIO_NUM_NC; 
  } 
  
  virtual gpio_num_t EPD_CS_PIN() { return GPIO_NUM_46; }  // Overrides SMA Inverter Control
  virtual gpio_num_t EPD_DC_PIN() { return GPIO_NUM_45; }
  virtual gpio_num_t EPD_RST_PIN(){ return GPIO_NUM_47; }
  virtual gpio_num_t EPD_SCK_PIN(){ return GPIO_NUM_16; }  // Overrides CHADEMO_PIN_2
  virtual gpio_num_t EPD_MOSI_PIN(){ return GPIO_NUM_15; } // Overrides CHADEMO_PIN_10

  // Optional manual refresh button for E-Paper
  virtual gpio_num_t EPD_REFRESH_BTN_PIN() {
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_40; // Overrides CHADEMO_LOCK
    }
    return GPIO_NUM_NC;
  }

  // ==========================================
  // CHAdeMO Support
  // ==========================================
  virtual gpio_num_t CHADEMO_PIN_2() { return GPIO_NUM_16; }
  virtual gpio_num_t CHADEMO_PIN_10() { return GPIO_NUM_15; }
  virtual gpio_num_t CHADEMO_PIN_7() { return GPIO_NUM_47; }
  
  virtual gpio_num_t CHADEMO_PIN_4() { 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_NC; // Disabled when E-Paper is active
    }
    return GPIO_NUM_4; 
  }
  
  virtual gpio_num_t CHADEMO_LOCK() { 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C || 
        user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
        return GPIO_NUM_NC; // Disabled when E-Paper is active
    }
    return GPIO_NUM_40; 
  }

  // ==========================================
  // Interface Configuration
  // ==========================================
  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative, comm_interface::CanAddonMcp2515,
            comm_interface::CanFdAddonMcp2518};
  }

  virtual const char* name_for_comm_interface(comm_interface comm) {
    switch (comm) {
      case comm_interface::CanNative:
        return "CAN B (Native)";
      case comm_interface::CanFdNative:
        return "";
      case comm_interface::CanAddonMcp2515:
        return "CAN A (MCP2515)";
      case comm_interface::CanFdAddonMcp2518:
        return "CAN FD (MCP2518 add-on)";
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

#define HalClass LilyGo2CANHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif