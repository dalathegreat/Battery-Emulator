#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 9999

class TestFakeBattery : public CanBattery {
 public:
  TestFakeBattery() : CanBattery(TestFake) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Fake battery for testing purposes";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
