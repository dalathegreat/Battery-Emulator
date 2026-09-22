#ifndef TEST_FAKE_BATTERY_H
#define TEST_FAKE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"
#include "CanBattery.h"

// Also implements BatteryHtmlRenderer directly, as AKASOL does, so the More Battery Info page can
// show the private constants below and each instance renders its own datalayer (battery 2 and 3 too)
class TestFakeBattery : public CanBattery, public BatteryHtmlRenderer {
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

  BatteryHtmlRenderer& get_status_renderer() { return *this; }
  String get_status_html();
  bool renders_own_battery_data() { return true; }

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  static const int MAX_CELL_DEVIATION_MV = 9999;

  static const int NUMBER_OF_CELLS = 96;
  // Random spread applied on top of the evenly divided pack voltage, per cell
  static const int CELL_SPREAD_MV = 20;
  // Simulated balancing starts once the calculated SOC is above this level
  static const uint16_t BALANCING_START_SOC_PPTT = 8500;  // 85.00%
};

#endif
