#include <gtest/gtest.h>

#include "../utils/utils.h"

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/devboard/utils/events.h"

class BatteryTestFixture : public testing::Test {
 public:
  BatteryTestFixture(BatteryType type) : type(type) {}
  // Optional:
  //   static void SetUpTestSuite() { ... }
  //   static void TearDownTestSuite() { ... }

  void SetUp() override {
    // Reset the datalayer and events before each test
    datalayer = DataLayer();
    reset_all_events();
    if (battery) {
      delete battery;
      battery = nullptr;
    }
    init_hal();

    user_selected_battery_type = type;
    setup_battery();
  }

  void TearDown() override {
    if (battery) {
      delete battery;
      battery = nullptr;
    }
  }

 private:
  BatteryType type;
};

// Check that the CAN aliveness timeout isn't being renewed by bogus CAN frames
class TestNotStillAlive : public BatteryTestFixture {
 public:
  explicit TestNotStillAlive(BatteryType type) : BatteryTestFixture(type) {}
  void TestBody() override {
    // check if battery is a CanBattery subclass
    auto* battery = dynamic_cast<CanBattery*>(::battery);
    if (battery == nullptr) {
      GTEST_SKIP() << "Battery is not a CanBattery subclass";
    }

    // Set the still-alive counter to 0 (ie, not alive)
    datalayer.battery.status.CAN_battery_still_alive = 0;

    // A random fake CAN frame
    CAN_frame frame = {
        .ID = 0x7ff,
        .data = {.u8 = {0x00, 0x64, 0x00, 0x64, 0x0F, 0xA0, 0x27, 0x10}},
    };
    for (int i = 0; i < 50; i++) {
      battery->handle_incoming_can_frame(frame);
    }

    // Check it's still not alive (ie, the CAN frame handling didn't renew the counter)
    EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 0);
  }
};

bool IsValidCanBattery(BatteryType type) {
  if (type == BatteryType::TestFake) {
    return false;
  }

  // We need some minimal setup or the battery constructors may segfault
  datalayer = DataLayer();
  // This leaks memory (but not much...)
  init_hal();

  auto* tmp_battery = create_battery(type);
  if (tmp_battery == nullptr) {
    // Not a valid battery type
    return false;
  }

  auto* as_can_battery = dynamic_cast<CanBattery*>(tmp_battery);
  if (as_can_battery == nullptr) {
    // Failed to cast to CanBattery, so it's not a CAN battery
    delete tmp_battery;
    return false;
  }
  delete tmp_battery;

  return true;
}

void RegisterStillAliveTests() {
  for (int i = 0; i < (int)BatteryType::Highest; i++) {
    if (!IsValidCanBattery((BatteryType)i)) {
      continue;
    }

    std::string test_name = ("TestNotStillAlive" + snake_case_to_camel_case(name_for_battery_type((BatteryType)i)));
    testing::RegisterTest("StillAliveTests", test_name.c_str(), nullptr, nullptr, __FILE__, __LINE__,
                          [=]() -> BatteryTestFixture* { return new TestNotStillAlive((BatteryType)i); });
  }
}
