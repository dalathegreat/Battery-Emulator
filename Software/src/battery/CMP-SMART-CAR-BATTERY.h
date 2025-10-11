#ifndef CMP_SMART_CAR_BATTERY_H
#define CMP_SMART_CAR_BATTERY_H
#include "CMP-SMART-CAR-BATTERY-HTML.h"
#include "CanBattery.h"

class CmpSmartCarBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis CMP Smart Car Battery";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  CmpSmartCarHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_100S_DV = 3700;
  static const int MIN_PACK_VOLTAGE_100S_DV = 2900;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 3650;
  static const int MIN_CELL_VOLTAGE_MV = 2800;

  unsigned long previousMillis10 = 0;  // will store last time a 10ms CAN Message was sent
  uint8_t mux = 0;
  int16_t temperature_sensors[16];
  int16_t temp_min = 0;
  int16_t temp_max = 0;
  uint16_t cell_voltages_mV[100];
  uint16_t battery_soc = 5000;
  uint16_t battery_voltage = 3300;
  uint8_t battery_interlock = 0x50;  //0x5 closed, 0xF if either interlock broken
};
#endif
