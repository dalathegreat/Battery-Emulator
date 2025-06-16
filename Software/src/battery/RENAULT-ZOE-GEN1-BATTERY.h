#ifndef RENAULT_ZOE_GEN1_BATTERY_H
#define RENAULT_ZOE_GEN1_BATTERY_H

#include "CanBattery.h"
#include "RENAULT-ZOE-GEN1-HTML.h"

#ifdef RENAULT_ZOE_GEN1_BATTERY
#define SELECTED_BATTERY_CLASS RenaultZoeGen1Battery
#endif

class RenaultZoeGen1Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4200;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  RenaultZoeGen1HtmlRenderer renderer;
};

#endif
