#ifndef __HW_BECOM_H__
#define __HW_BECOM_H__

#include "hal.h"

class BEComHal : public Esp32Hal {
 public:
  const char* name() { return "BECom"; }

  virtual void set_default_configuration_values() {
    BatteryEmulatorSettingsStore settings;
    if (!settings.settingExists("CANFREQ")) {
      settings.saveUInt("CANFREQ", 40);
    }
  }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_8; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_18; }

  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_15; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_17; }
  // New Drive Enable Pin for RS485 Half Duplex communication
  virtual gpio_num_t RS485_DE_PIN() { return GPIO_NUM_16; }

  // 1st CANFD Interface: MCP2518
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_11; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_13; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_14; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_10; }

  // 2nd CANFD Interface: MCP2518
  // SPI Bus is shared with the 1st interface, only INT and CS pins are needed
  virtual gpio_num_t MCP2517_CS2() { return GPIO_NUM_21; }
  virtual gpio_num_t MCP2517_INT2() { return GPIO_NUM_9; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_47; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_48; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_45; }
  // BMS Power Pin is inverted, High Value shuts down the Battery
  // TODO: Adapt the rest of the code to invert the power control
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_1; }
  // SECOND_BATTERY_CONTACTORS_PIN mapped to the Battery 2 Negative Contactor Pin
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_37; }

  // Automatic precharging
  // HIA4V1_PIN mapped to Battery 1 Pre-Chage Contactor Pin
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_45; }
  // INVERTER_DISCONNECT_CONTACTOR_PIN mapped to Battery 1 Positive Contactor Pin
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_47; }

  // SMA CAN contactor pin
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_3; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_5; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  // Mapped to and internal allways low pin, in hardware v1
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_46; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_2; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_41; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative, comm_interface::CanFdNative};
  }

  virtual const char* name_for_comm_interface(comm_interface comm) {
    switch (comm) {
      case comm_interface::CanNative:
        return "Inverter CAN (Native)";
      case comm_interface::CanFdNative:
        return "Battery CANFD (Native)";
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

#define HalClass BEComHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
