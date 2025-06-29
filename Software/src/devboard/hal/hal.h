#ifndef _HAL_H_
#define _HAL_H_

#include <soc/gpio_num.h>
#include <chrono>
#include <unordered_map>
#include "../../../src/devboard/utils/events.h"
#include "../../../src/devboard/utils/types.h"

// Hardware Abstraction Layer base class.
// Derive a class to define board-specific parameters such as GPIO pin numbers
// This base class implements a mechanism for allocating GPIOs.
class Esp32Hal {
 public:
  virtual const char* name() = 0;

  // Time it takes before system is considered fully started up.
  virtual duration BOOTUP_TIME() = 0;
  virtual bool system_booted_up();

  // Core assignment
  virtual int CORE_FUNCTION_CORE() { return 1; }
  virtual int MODBUS_CORE() { return 0; }
  virtual int WIFICORE() { return 0; }

  template <typename... Pins>
  bool alloc_pins(const char* name, Pins... pins) {
    std::vector<gpio_num_t> requested_pins = {static_cast<gpio_num_t>(pins)...};

    for (gpio_num_t pin : requested_pins) {
      if (pin < 0) {
        set_event(EVENT_GPIO_NOT_DEFINED, (int)pin);
        // Event: {name} attempted to allocate pin that wasn't defined for the selected HW.
        return false;
      }

      auto it = allocated_pins.find(pin);
      if (it != allocated_pins.end()) {
        // Event: GPIO conflict for pin {pin} between name and it->second.
        //std::cerr << "Pin " << pin << " already allocated to \"" << it->second << "\".\n";
        set_event(EVENT_GPIO_CONFLICT, (int)pin);
        return false;
      }
    }

    for (gpio_num_t pin : requested_pins) {
      allocated_pins[pin] = name;
    }

    return true;
  }

  // Helper to forward vector to variadic template
  template <typename Vec, size_t... Is>
  bool alloc_pins_from_vector(const char* name, const Vec& pins, std::index_sequence<Is...>) {
    return alloc_pins(name, pins[Is]...);
  }

  template <typename... Pins>
  bool alloc_pins_ignore_unused(const char* name, Pins... pins) {
    std::vector<gpio_num_t> valid_pins;
    for (gpio_num_t pin : std::vector<gpio_num_t>{static_cast<gpio_num_t>(pins)...}) {
      if (pin != GPIO_NUM_NC) {
        valid_pins.push_back(pin);
      }
    }

    return alloc_pins_from_vector(name, valid_pins, std::make_index_sequence<sizeof...(pins)>{});
  }

  virtual bool always_enable_bms_power() { return false; }

  virtual gpio_num_t PIN_5V_EN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_EN_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_SE_PIN() { return GPIO_NUM_NC; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_NC; }

  // CAN_ADDON
  // SCK input of MCP2515
  virtual gpio_num_t MCP2515_SCK() { return GPIO_NUM_NC; }
  // SDI input of MCP2515
  virtual gpio_num_t MCP2515_MOSI() { return GPIO_NUM_NC; }
  // SDO output of MCP2515
  virtual gpio_num_t MCP2515_MISO() { return GPIO_NUM_NC; }
  // CS input of MCP2515
  virtual gpio_num_t MCP2515_CS() { return GPIO_NUM_NC; }
  // INT output of MCP2515
  virtual gpio_num_t MCP2515_INT() { return GPIO_NUM_NC; }

  // CANFD_ADDON defines for MCP2517
  virtual gpio_num_t MCP2517_SCK() { return GPIO_NUM_NC; }
  virtual gpio_num_t MCP2517_SDI() { return GPIO_NUM_NC; }
  virtual gpio_num_t MCP2517_SDO() { return GPIO_NUM_NC; }
  virtual gpio_num_t MCP2517_CS() { return GPIO_NUM_NC; }
  virtual gpio_num_t MCP2517_INT() { return GPIO_NUM_NC; }

  // CHAdeMO support pin dependencies
  virtual gpio_num_t CHADEMO_PIN_2() { return GPIO_NUM_NC; }
  virtual gpio_num_t CHADEMO_PIN_10() { return GPIO_NUM_NC; }
  virtual gpio_num_t CHADEMO_PIN_7() { return GPIO_NUM_NC; }
  virtual gpio_num_t CHADEMO_PIN_4() { return GPIO_NUM_NC; }
  virtual gpio_num_t CHADEMO_LOCK() { return GPIO_NUM_NC; }

  // Contactor handling
  virtual gpio_num_t POSITIVE_CONTACTOR_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t NEGATIVE_CONTACTOR_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t PRECHARGE_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t BMS_POWER() { return GPIO_NUM_NC; }
  virtual gpio_num_t BMS2_POWER() { return GPIO_NUM_NC; }
  virtual gpio_num_t SECOND_BATTERY_CONTACTORS_PIN() { return GPIO_NUM_NC; }

  // Automatic precharging
  virtual gpio_num_t HIA4V1_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t INVERTER_DISCONNECT_CONTACTOR_PIN() { return GPIO_NUM_NC; }

  // SMA CAN contactor pins
  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_PIN() { return GPIO_NUM_NC; }

  virtual gpio_num_t INVERTER_CONTACTOR_ENABLE_LED_PIN() { return GPIO_NUM_NC; }

  // SD card
  virtual gpio_num_t SD_MISO_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_MOSI_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_SCLK_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t SD_CS_PIN() { return GPIO_NUM_NC; }

  // LED
  virtual gpio_num_t LED_PIN() { return GPIO_NUM_NC; }
  virtual uint8_t LED_MAX_BRIGHTNESS() { return 40; }

  // Equipment stop pin
  virtual gpio_num_t EQUIPMENT_STOP_PIN() { return GPIO_NUM_NC; }

  // Battery wake up pins
  virtual gpio_num_t WUP_PIN1() { return GPIO_NUM_NC; }
  virtual gpio_num_t WUP_PIN2() { return GPIO_NUM_NC; }

  // Returns the available comm interfaces on this HW
  virtual std::vector<comm_interface> available_interfaces() = 0;

 private:
  std::unordered_map<gpio_num_t, std::string> allocated_pins;
};

extern Esp32Hal* esp32hal;

// Needed for AsyncTCPSock library.
#define WIFI_CORE (esp32hal->WIFICORE())

void init_hal();

#endif
