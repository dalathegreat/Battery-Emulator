#ifndef SAMSUNG_SDI_LV_BATTERY_H
#define SAMSUNG_SDI_LV_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class SamsungSdiLVBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Samsung SDI LV Battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 600;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 300;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4200;
  static const int MIN_CELL_VOLTAGE_MV = 3000;

  uint16_t system_voltage = 50000;
  int16_t system_current = 0;
  uint8_t system_SOC = 50;
  uint8_t system_SOH = 99;
  uint16_t battery_charge_voltage = 9999;
  uint16_t charge_current_limit = 0;
  uint16_t discharge_current_limit = 0;
  uint16_t battery_discharge_voltage = 0;
  uint8_t alarms_frame0 = 0;
  uint8_t alarms_frame1 = 0;
  uint8_t protection_frame2 = 0;
  uint8_t protection_frame3 = 0;
  uint16_t maximum_cell_voltage = 3700;
  uint16_t minimum_cell_voltage = 3700;
  int8_t maximum_cell_temperature = 0;
  int8_t minimum_cell_temperature = 0;
  uint8_t system_permanent_failure_status_dry_contact = 0;
  uint8_t system_permanent_failure_status_fuse_open = 0;
};

#endif
