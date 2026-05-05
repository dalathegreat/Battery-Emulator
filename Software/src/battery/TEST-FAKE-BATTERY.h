#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class TestFakeBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  TestFakeBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  TestFakeBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
  }

  static constexpr const char* Name = "Fake battery for testing purposes";

  virtual void setup();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_set_fake_voltage() { return true; }
  void set_fake_voltage(float val) { datalayer.battery.status.voltage_dV = val * 10; }

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  static const int MAX_CELL_DEVIATION_MV = 9999;
};

#endif
