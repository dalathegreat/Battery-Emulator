#ifndef __HW_3LB_H__
#define __HW_3LB_H__

#include "hal.h"

class ThreeLBHal : public Esp32Hal {
 public:
  const char* name() { return "3LB board"; }

  //are these backwards? I don't use RS485 so can't test
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_33; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_32; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_27; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_16; }

  //Want to support 4CAN interfaces? Have I got a deal for you.
  // CAN_ADDON
  // SCK input of MCP2515
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_14; }
  // SDI input of MCP2515
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_13; }
  // SDO output of MCP2515
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_12; }
  // CS input of MCP2515
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_15; }
  // INT output of MCP2515
  //FIXME virtual gpio_num_t MCP2515_INT() { return EXT1_GPIO_NUM_B6; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_18; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_23; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_19; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_5; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_34; }

  // 2nd CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_2_CS() { return GPIO_NUM_2; }
  virtual gpio_num_t MCP2517_2_INT() { return GPIO_NUM_35; }

  // Ethernet defines for W5500/W550IO module
  /*
  virtual gpio_num_t W5500_SCK() { return GPIO_NUM_14; }
  virtual gpio_num_t W5500_MISO() { return GPIO_NUM_12; }
  virtual gpio_num_t W5500_MOSI() { return GPIO_NUM_13; }
  virtual gpio_num_t W5500_CS() { return GPIO_NUM_15; }
  virtual gpio_num_t W5500_INT() { return EXT1_GPIO_NUM_B6; }
  virtual gpio_num_t W5500_RDY() { return GPIO_NUM_25; }
  virtual gpio_num_t W5500_RST() { return GPIO_NUM_26; }
*/

  //unused in 3lb
  // CHAdeMO support pin dependencies
  //FIXME virtual gpio_num_t CHADEMO_PIN_2() { return EXT1_GPIO_NUM_A0; }
  //FIXME virtual gpio_num_t CHADEMO_PIN_10() { return EXT1_GPIO_NUM_A1; }
  //FIXME virtual gpio_num_t CHADEMO_PIN_7() { return EXT1_GPIO_NUM_A2; }
  //FIXME virtual gpio_num_t CHADEMO_PIN_4() { return EXT1_GPIO_NUM_A3; }
  //FIXME virtual gpio_num_t CHADEMO_LOCK() { return EXT1_GPIO_NUM_A4; }

  // Contactor handling
  //FIXME virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return EXT1_GPIO_NUM_B3; }
  //FIXME virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return EXT1_GPIO_NUM_B2; }
  //FIXME virtual gpio_num_t PRECHARGE_PIN() { return EXT1_GPIO_NUM_B4; }
  //FIXME virtual gpio_num_t BMS_POWER() { return EXT1_GPIO_NUM_B5; }
  //FIXME virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return EXT1_GPIO_NUM_B1; }
  //FIXME virtual gpio_num_t THIRD_BATTERY_CONTACTORS_PIN() { return EXT1_GPIO_NUM_B0; }

  // Automatic precharging
  //conflicts with serial, but hopefully that won't be an issue
  //the other option is to use either the ethernet interface pins
  //which are 25,26 but I expect people won't be using serial and pre-charge
  //at the same time, so this allows "full functionality" if you ignore serial.
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_1; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_3; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_36; }
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_NC; }

  //unused in 3lb
  // SD card
  virtual gpio_num_t SD_MISO_PIN() { return GPIO_NUM_2; }
  virtual gpio_num_t SD_MOSI_PIN() { return GPIO_NUM_15; }
  virtual gpio_num_t SD_SCLK_PIN() { return GPIO_NUM_14; }
  virtual gpio_num_t SD_CS_PIN() { return GPIO_NUM_13; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  //FIXME virtual gpio_num_t EQUIPMENT_STOP_PIN() { return EXT1_GPIO_NUM_B7; }

  // Battery wake up pins
  //FIXME virtual gpio_num_t WUP_PIN1() { return EXT1_GPIO_NUM_A5; }
  //FIXME virtual gpio_num_t WUP_PIN2() { return EXT1_GPIO_NUM_A6; }

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
        return "CAN (MCP2515 add-on)";
      case comm_interface::CanFdAddonMcp2518:
        return "CAN FD 1 (MCP2518)";
      case comm_interface::CanFdAddonMcp2518_2:
        return "CAN FD 2 (MCP2518)";
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

#endif
