#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../include.h"
#include "CanBattery.h"

#ifdef TEST_FAKE_BATTERY
#define SELECTED_BATTERY_CLASS TestFakeBattery
#endif

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

  static constexpr char* Name = "Fake battery for testing purposes";

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

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send

  CAN_frame TEST = {.FD = false,
                    .ext_ID = false,
                    .DLC = 8,
                    .ID = 0x123,
                    .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
};

#endif
