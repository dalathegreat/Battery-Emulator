#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_CELL_DEVIATION_MV 9999

class TestFakeBattery : public CanBattery {
 public:
  TestFakeBattery(DATALAYER_BATTERY_TYPE* target, CAN_Interface can_interface) : CanBattery(TestFake) {
    m_target = target;
    m_can_interface = can_interface;
  }
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Fake battery for testing purposes";

  virtual bool supportsDoubleBattery() { return true; };
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

 private:
  DATALAYER_BATTERY_TYPE* m_target;
  CAN_Interface m_can_interface;
};

#endif
