#ifndef __HW_WAVESHARE_H__
#define __HW_WAVESHARE_H__

#include "hal.h"

class WaveshareS3Rs485CanHal : public Esp32Hal {
 public:
  const char* name() { return "Waveshare ESP32-S3-RS485-CAN"; }

  virtual gpio_num_t CAN_TX_PIN() { return GPIO_NUM_15; }
  virtual gpio_num_t CAN_RX_PIN() { return GPIO_NUM_16; }
  virtual gpio_num_t CAN_SE_PIN() { return GPIO_NUM_NC; }

  virtual gpio_num_t RS485_TX_PIN() { return GPIO_NUM_17; }
  virtual gpio_num_t RS485_RX_PIN() { return GPIO_NUM_18; }

  // SP3485 DE and /RE are tied together on this board.
  // HIGH = transmit, LOW = receive. Do not use RS485_EN_PIN for this signal.
  virtual gpio_num_t RS485_DE_PIN() { return GPIO_NUM_21; }

  virtual gpio_num_t RS485_EN_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t RS485_SE_PIN() { return GPIO_NUM_NC; }
  virtual gpio_num_t PIN_5V_EN() { return GPIO_NUM_NC; }

  virtual gpio_num_t LED_PIN() { return GPIO_NUM_NC; }

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
        return "";
      case comm_interface::CanFdAddonMcp2518:
        return "";
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

#define HalClass WaveshareS3Rs485CanHal

/* ----- Error checks below, don't change (can't be moved to separate file) ----- */
#ifndef HW_CONFIGURED
#define HW_CONFIGURED
#else
#error Multiple HW defined! Please select a single HW
#endif

#endif
