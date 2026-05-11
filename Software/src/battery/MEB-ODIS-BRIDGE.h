#ifndef MEB_ODIS_BRIDGE_H
#define MEB_ODIS_BRIDGE_H

#include "../system_settings.h"
#include "CanBattery.h"

class MebOdisBridge : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "MEB ODIS Bridge";

 private:
};

#endif
