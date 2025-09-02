#ifndef __HW_C6_H__
#define __HW_C6_H__

#include "hal.h"

class C6Hal : public Esp32Hal {
 public:
  const char* name() { return "ESP32C6"; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_27; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_26; }
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_23; }

  virtual gpio_num_t CAN2_TX_PIN() { return GPIO_NUM_16; }
  virtual gpio_num_t CAN2_RX_PIN() { return GPIO_NUM_17; }

  // CAN_ADDON
  // SCK input of MCP2515
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_12; }
  // SDI input of MCP2515
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_5; }
  // SDO output of MCP2515
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_26; }
  // CS input of MCP2515
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_18; }
  // INT output of MCP2515
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_26; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_5; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_26; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_18; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_26; }

  // CHAdeMO support pin dependencies
  virtual gpio_num_t CHADEMO_PIN_2() { return GPIO_NUM_12; }
  virtual gpio_num_t CHADEMO_PIN_10() { return GPIO_NUM_5; }
  virtual gpio_num_t CHADEMO_PIN_7() { return GPIO_NUM_26; }
  virtual gpio_num_t CHADEMO_PIN_4() { return GPIO_NUM_26; }
  virtual gpio_num_t CHADEMO_LOCK() { return GPIO_NUM_18; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_26; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_26; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_25; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_18; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_15; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_25; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_26; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_5; }

  //        virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_NC; }

  // SD card
  virtual gpio_num_t SD_MISO_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_MOSI_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_SCLK_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_CS_PIN() { return GPIO_NUM_NC; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_26; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_25; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_26; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::CanNative, comm_interface::CanAddonMcp2515, comm_interface::CanFdAddonMcp2518,
            comm_interface::CanNative2};
  }
};

#define HalClass C6Hal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
