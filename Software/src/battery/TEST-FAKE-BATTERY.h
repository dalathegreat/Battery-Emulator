#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../include.h"
#include "CanBattery.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS TestFakeBattery
#define MAX_CELL_DEVIATION_MV 9999

class TestFakeBattery : public CanBattery {
 public:
  TestFakeBattery(int batteryNumber, int targetCan) : CanBattery(batteryNumber, targetCan) {}

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
};

#endif
