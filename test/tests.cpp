#include <gtest/gtest.h>
#include <stdio.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/charger/CHARGERS.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/inverter/INVERTERS.h"

void RegisterCanLogTests(void);
void RegisterStillAliveTests(void);

class DataLayerResetListener : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo& /*test_info*/) override {
    datalayer = DataLayer();
    reset_all_events();

    // Every instance holds pointers into the datalayer we just replaced, so
    // destroy them all.
    delete battery;
    battery = nullptr;
    delete battery2;
    battery2 = nullptr;
    delete battery3;
    battery3 = nullptr;
    delete charger;
    charger = nullptr;
    /* The inverter is the same kind of instance and was the one omission here.
       It also decides behaviour by TYPE - needs_can_startup_grace() is true for
       the SMA family and false for the rest - so a test inheriting the previous
       test's inverter silently runs against the wrong protocol. */
    delete inverter;
    inverter = nullptr;

    // Selection globals must be owned by each test's own fixture.
    user_selected_second_battery = false;
    user_selected_triple_battery = false;

    init_hal();
  }
};

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  // Add a listener to reset the datalayer and events before each test
  ::testing::UnitTest::GetInstance()->listeners().Append(new DataLayerResetListener);

  RegisterCanLogTests();
  RegisterStillAliveTests();

  return RUN_ALL_TESTS();
}

void store_settings_equipment_stop(void) {}
