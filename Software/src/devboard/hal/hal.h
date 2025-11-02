#ifndef _HAL_H_
#define _HAL_H_

#include <soc/gpio_num.h>
#include <chrono>
#include <unordered_map>
#include "../../../src/communication/nvm/comm_nvm.h"
#include "../../../src/devboard/utils/events.h"
#include "../../../src/devboard/utils/logging.h"
#include "../../../src/devboard/utils/types.h"

// Hardware Abstraction Layer base class.
// Derive a class to define board-specific parameters such as GPIO pin numbers
// This base class implements a mechanism for allocating GPIOs.
class Esp32Hal {
 public:
  virtual const char* name() = 0;

  // Time it takes before system is considered fully started up.
  virtual duration BOOTUP_TIME() { return milliseconds(1000); }
  virtual bool system_booted_up();

  // Core assignment
  virtual int CORE_FUNCTION_CORE() { return 1; }
  virtual int MODBUS_CORE() { return 0; }
  virtual int WIFICORE() { return 0; }

  virtual void set_default_configuration_values() {}

  template <typename... Pins>
  bool alloc_pins(const char* name, Pins... pins) {
    std::vector<gpio_num_t> requested_pins = {static_cast<gpio_num_t>(pins)...};

    for (gpio_num_t pin : requested_pins) {
      if (pin < 0) {
        set_event(EVENT_GPIO_NOT_DEFINED, (int)pin);
        allocator_name = name;
        DEBUG_PRINTF("%s attempted to allocate pin %d that wasn't defined for the selected HW.\n", name, (int)pin);
        return false;
      }

      auto it = allocated_pins.find(pin);
      if (it != allocated_pins.end()) {
        allocator_name = name;
        allocated_name = it->second.c_str();
        DEBUG_PRINTF("GPIO conflict for pin %d between %s and %s.\n", (int)pin, name, it->second.c_str());
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

  // Base case: no more pins
  inline bool alloc_pins_ignore_unused_impl(const char* name) {
    return alloc_pins(name);  // Call with 0 pins
  }

  // Recursive case: process one pin at a time
  template <typename... Rest>
  bool alloc_pins_ignore_unused_impl(const char* name, gpio_num_t first, Rest... rest) {
    if (first == GPIO_NUM_NC) {
      return alloc_pins_ignore_unused_impl(name, rest...);
    } else {
      return call_alloc_pins_filtered(name, first, rest...);
    }
  }

  // This helper just forwards pins after filtering is done
  template <typename... Pins>
  bool call_alloc_pins_filtered(const char* name, Pins... pins) {
    return alloc_pins(name, pins...);
  }

  // Entry point
  template <typename... Pins>
  bool alloc_pins_ignore_unused(const char* name, Pins... pins) {
    return alloc_pins_ignore_unused_impl(name, static_cast<gpio_num_t>(pins)...);
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
  // Reset pin for MCP2515
  virtual gpio_num_t MCP2515_RST() { return GPIO_NUM_NC; }

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

  String failed_allocator() { return allocator_name; }
  String conflicting_allocator() { return allocated_name; }

 private:
  std::unordered_map<gpio_num_t, std::string> allocated_pins;

  // For event logging, store the name of the allocator/allocated
  // for failed gpio allocations.
  String allocator_name;
  String allocated_name;
};

extern Esp32Hal* esp32hal;

// Needed for AsyncTCPSock library.
#define WIFI_CORE (esp32hal->WIFICORE())

void init_hal();

#endif
