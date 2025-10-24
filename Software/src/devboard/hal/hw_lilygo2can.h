#ifndef __HW_LILYGO2CAN_H__
#define __HW_LILYGO2CAN_H__

#include "hal.h"

/*
The 2CAN has four GPIOs on the top (43/44 and 1/2 on each of the SH-1mm
connectors) and 21 on the bottom (on the 2x13 header). GPIO35-37 aren't usable
if opi PSRAM is enabled.

43:RS485_TX
44:RS485_RX

1:WUP1/BMS_POWER/ESTOP
2:WUP2/LED

3.3V                   GND
  5V                   GND
 x35:LED                39:INT
  38:SCK                42:SDI
 x37:SDO                41:nCS
 x36:ESTOP              40
  16                     4
  15                     5:BAT2_CTRS
  45                    48:POS_CTR
  47:                   21:PRECHARGE
  14:HV_INV_DIS/[WUP2]  17:NEG_CTR
  18:HV_PRE/[WUP1]     GND
  46:INV_CTR_EN          3:BMS_POWER

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

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_7; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_6; }

  // Note: these are used by the bootloader UART, so there'll be some bootloader
  // chatter sent down the RS485 bus when the chip starts up. Hopefully this
  // won't matter (it'll be 115200 baud, so much higher than the normal Modbus
  // 9600 baud) - if it's a problem we can burn fuses to disable it.
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_43; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_44; }

  // Built In MCP2515 CAN_ADDON
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_11; }
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_13; }
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_10; }
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_8; }
  virtual gpio_num_t MCP2515_RST() { return GPIO_NUM_9; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_38; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_42; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_37; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_41; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_39; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_48; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_21; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_3; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_5; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_18; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_14; }

  // SMA CAN contactor pin
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_46; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_35; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_36; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_1; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_2; }

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
