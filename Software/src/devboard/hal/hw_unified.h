#ifndef __HW_UNIFIED_H__
#define __HW_UNIFIED_H__

#include "board_config.h"
#include "hal.h"

// HAL for the unified ESP32-S3 image. Every accessor reads the board config
// that init_board_config() parsed, so one binary covers every S3 board.
//
// With no configuration loaded, board_config is default-constructed and every
// pin reads back as GPIO_NUM_NC. That is the same thing the base class returns
// for a peripheral a board does not have, so nothing needs a special case: the
// firmware simply comes up with no battery, no inverter and no contactors, and
// the web UI offers the upload page.
class UnifiedHal : public Esp32Hal {
 public:
  virtual const char* name() { return board_config.valid ? board_config.name.c_str() : "Unconfigured hardware"; }

  virtual bool always_enable_bms_power() { return board_config.bms_power_always_on; }

  // RS485
  virtual gpio_num_t PIN_5V_EN() { return board_config.pin_5v_en; }
  virtual gpio_num_t RS485_EN_PIN() { return board_config.rs485_en; }
  virtual gpio_num_t RS485_TX_PIN() { return board_config.rs485_tx; }
  virtual gpio_num_t RS485_RX_PIN() { return board_config.rs485_rx; }
  virtual gpio_num_t RS485_SE_PIN() { return board_config.rs485_se; }
  virtual gpio_num_t RS485_DE_PIN() { return board_config.rs485_de; }
  virtual bool RS485_DE_ACTIVE_HIGH() { return board_config.rs485_de_active_high; }

  // Native CAN
  virtual gpio_num_t CAN_TX_PIN() { return board_config.can_tx; }
  virtual gpio_num_t CAN_RX_PIN() { return board_config.can_rx; }
  virtual gpio_num_t CAN_SE_PIN() { return board_config.can_se; }

  // MCP2515
  virtual uint8_t MCP2515_BUS() { return board_config.mcp2515.bus; }
  virtual gpio_num_t MCP2515_SCK() { return board_config.mcp2515.sck; }
  virtual gpio_num_t MCP2515_MOSI() { return board_config.mcp2515.sdi; }
  virtual gpio_num_t MCP2515_MISO() { return board_config.mcp2515.sdo; }
  virtual gpio_num_t MCP2515_CS() { return board_config.mcp2515.cs; }
  virtual gpio_num_t MCP2515_INT() { return board_config.mcp2515.intr; }
  virtual gpio_num_t MCP2515_RST() { return board_config.mcp2515.rst; }
  virtual uint32_t MCP2515_FREQ() { return board_config.mcp2515.freq; }

  // MCP2517/2518FD, interface 1
  virtual uint8_t MCP2517_BUS() { return board_config.mcp2518fd[0].bus; }
  virtual gpio_num_t MCP2517_SCK() { return board_config.mcp2518fd[0].sck; }
  virtual gpio_num_t MCP2517_SDI() { return board_config.mcp2518fd[0].sdi; }
  virtual gpio_num_t MCP2517_SDO() { return board_config.mcp2518fd[0].sdo; }
  virtual gpio_num_t MCP2517_CS() { return board_config.mcp2518fd[0].cs; }
  virtual gpio_num_t MCP2517_INT() { return board_config.mcp2518fd[0].intr; }
  virtual uint32_t MCP2517_FREQ() { return board_config.mcp2518fd[0].freq; }
  virtual int MCP2517_CLKODIV() { return board_config.mcp2518fd[0].clkodiv; }

  // MCP2517/2518FD, interface 2
  virtual uint8_t MCP2517_BUS2() { return board_config.mcp2518fd[1].bus; }
  virtual gpio_num_t MCP2517_SCK2() { return board_config.mcp2518fd[1].sck; }
  virtual gpio_num_t MCP2517_SDI2() { return board_config.mcp2518fd[1].sdi; }
  virtual gpio_num_t MCP2517_SDO2() { return board_config.mcp2518fd[1].sdo; }
  virtual gpio_num_t MCP2517_CS2() { return board_config.mcp2518fd[1].cs; }
  virtual gpio_num_t MCP2517_INT2() { return board_config.mcp2518fd[1].intr; }
  virtual uint32_t MCP2517_FREQ2() { return board_config.mcp2518fd[1].freq; }

  // CHAdeMO
  virtual gpio_num_t CHADEMO_PIN_2() { return board_config.chademo_2; }
  virtual gpio_num_t CHADEMO_PIN_10() { return board_config.chademo_10; }
  virtual gpio_num_t CHADEMO_PIN_7() { return board_config.chademo_7; }
  virtual gpio_num_t CHADEMO_PIN_4() { return board_config.chademo_4; }
  virtual gpio_num_t CHADEMO_LOCK() { return board_config.chademo_lock; }
  virtual gpio_num_t CHADEMO_CT_PIN() { return board_config.chademo_ct; }

  // Contactors
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return board_config.positive; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return board_config.negative; }
  virtual gpio_num_t PRECHARGE_PIN() { return board_config.precharge; }
  virtual gpio_num_t BMS_POWER() { return board_config.bms_power; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return board_config.second_battery; }
  virtual gpio_num_t TRIPLE_BATTERY_CONTACTORS_PIN() { return board_config.third_battery; }

  virtual std::vector<gpio_num_t> reset_hold_pins() {
    if (board_config.bms_power_reset_hold && board_config.bms_power != GPIO_NUM_NC) {
      return {board_config.bms_power};
    }
    return {};
  }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return board_config.hia4v1; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return board_config.inverter_disconnect; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return board_config.sma_enable; }
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return board_config.sma_led; }

  // LED
  virtual gpio_num_t LED_PIN() { return board_config.led; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return board_config.led_max_brightness; }
  virtual uint8_t LED_COUNT() { return board_config.led_count; }

#ifndef SMALL_FLASH_DEVICE
  virtual gpio_num_t DISPLAY_SDA_PIN() { return board_config.display_sda; }
  virtual gpio_num_t DISPLAY_SCL_PIN() { return board_config.display_scl; }
#endif  // SMALL_FLASH_DEVICE

  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return board_config.equipment_stop; }

  virtual gpio_num_t WUP_PIN1() { return board_config.wup1; }
  virtual gpio_num_t WUP_PIN2() { return board_config.wup2; }

  // Falls back to GPIO0 so that a board with no configuration can still be put
  // into AP mode by holding the BOOT button. GPIO0 is the boot strapping pin on
  // every ESP32-S3 module, so it is the one pin that is safe to assume.
  virtual gpio_num_t AP_BUTTON_PIN() {
    return board_config.ap_button != GPIO_NUM_NC ? board_config.ap_button : GPIO_NUM_0;
  }

  virtual std::vector<comm_interface> available_interfaces() { return board_config.interfaces; }
};

#define HalClass UnifiedHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif  // __HW_UNIFIED_H__
