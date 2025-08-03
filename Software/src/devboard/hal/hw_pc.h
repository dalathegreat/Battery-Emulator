#ifndef __HW_PC_H__
#define __HW_PC_H__

#include "hal.h"

class PcHal : public Esp32Hal {
 public:
  const char* name() { return "PC"; }

  virtual gpio_num_t PIN_5V_EN() { return GPIO_NUM_16; }
  virtual gpio_num_t RS485_EN_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_22; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_21; }
  virtual gpio_num_t RS485_SE_PIN() { return GPIO_NUM_19; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_27; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_26; }
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_23; }

  // CAN_ADDON
  // SCK input of MCP2515
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_12; }
  // SDI input of MCP2515
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_5; }
  // SDO output of MCP2515
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_34; }
  // CS input of MCP2515
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_18; }
  // INT output of MCP2515
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_35; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_12; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_5; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_34; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_18; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_35; }

  // CHAdeMO support pin dependencies
  virtual gpio_num_t CHADEMO_PIN_2() { return GPIO_NUM_12; }
  virtual gpio_num_t CHADEMO_PIN_10() { return GPIO_NUM_5; }
  virtual gpio_num_t CHADEMO_PIN_7() { return GPIO_NUM_34; }
  virtual gpio_num_t CHADEMO_PIN_4() { return GPIO_NUM_35; }
  virtual gpio_num_t CHADEMO_LOCK() { return GPIO_NUM_18; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_32; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_33; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_25; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_18; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_15; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_25; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_32; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_5; }

  //        virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_NC; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_35; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_25; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_32; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative, comm_interface::CanAddonMcp2515,
            comm_interface::CanFdAddonMcp2518};
  }
};

#endif
