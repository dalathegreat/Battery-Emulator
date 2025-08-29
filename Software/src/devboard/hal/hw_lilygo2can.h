#ifndef __HW_LILYGO2CAN_H__
#define __HW_LILYGO2CAN_H__

#include "hal.h"

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

  // Built In MCP2515 CAN_ADDON
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_11; }
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_13; }
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_10; }
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_8; }
  virtual gpio_num_t MCP2515_RST() { return GPIO_NUM_9; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_17; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_16; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_18; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_15; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_14; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_38; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_39; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_40; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_41; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_41; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_40; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_14; }

  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_5; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_5; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_40; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_38; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::CanNative, comm_interface::CanAddonMcp2515};
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
