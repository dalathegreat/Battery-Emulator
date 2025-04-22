#ifndef KIA_HYUNDAI_HYBRID_BATTERY_H
#define KIA_HYUNDAI_HYBRID_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class KiaHyundaiHybridBattery : public CanBattery {
 public:
  KiaHyundaiHybridBattery() : CanBattery(KiaHyundaiHybrid) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Kia/Hyundai Hybrid";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 2550; }
  virtual uint16_t min_pack_voltage_dv() { return 1700; }
  virtual uint16_t max_cell_deviation_mv() { return 100; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }
};

#endif
