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
  static const int MAX_PACK_VOLTAGE_DV = 5000;
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2700;

  unsigned long previousMillis10 = 0;  // will store last time a 10ms CAN Message was sent
  uint8_t mux = 0;
  int16_t temperature_sensors[16];
  int16_t temp_min = 0;
  int16_t temp_max = 0;
};
#endif
