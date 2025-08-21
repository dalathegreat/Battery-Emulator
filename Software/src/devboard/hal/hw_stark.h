#ifndef __HW_STARK_H__
#define __HW_STARK_H__

#include "hal.h"

/*
Stark CMR v1 - DIN-rail module with 4 power outputs, 1 x rs485, 1 x can and 1 x can-fd channel.
For more information on this board visit the project discord or contact johan@redispose.se

GPIOs on extra header
* GPIO  2
* GPIO 17 (only available if can channel 2 is inactive)
* GPIO 19
* GPIO 14 (JTAG TMS)
* GPIO 12 (JTAG TDI)
* GPIO 13 (JTAG TCK)
* GPIO 15 (JTAG TDO)
*/

class StarkHal : public Esp32Hal {
 public:
  const char* name() { return "Stark CMR Module"; }

  // Not needed, GPIO 16 has hardware pullup for PSRAM compatibility
  virtual gpio_num_t PIN_5V_EN() { return GPIO_NUM_NC; }

  // Not needed, GPIO 17 is used as SCK input of MCP2517
  virtual gpio_num_t RS485_EN_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_22; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_21; }
  // Not needed, GPIO 19 is available as extra GPIO via pin header
  virtual gpio_num_t RS485_SE_PIN() { return GPIO_NUM_NC; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_27; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_26; }

  // (No function, GPIO 23 used instead as MCP_SCK)
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_NC; }

  // CANFD_ADDON defines for MCP2517
  // Stark CMR v1 has GPIO pin 16 for SCK, CMR v2 has GPIO pin 17. Only diff between the two boards
  bool isStarkVersion1() {
    size_t flashSize = ESP.getFlashChipSize();
    if (flashSize == 4 * 1024 * 1024) {
      return true;
    } else {  //v2
      return false;
    }
  }
  virtual gpio_num_t MCP2517_SCK() { return isStarkVersion1() ? GPIO_NUM_16 : GPIO_NUM_17; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_5; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_34; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_18; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_35; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_32; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_33; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_25; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_23; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_19; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_19; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_25; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_2; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_4; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_2; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_25; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_32; }

  std::vector<comm_interface> available_interfaces() {
    return {comm_interface::Modbus, comm_interface::RS485, comm_interface::CanNative, comm_interface::CanFdNative};
  }
};

#endif  // __HW_STARK_H__
