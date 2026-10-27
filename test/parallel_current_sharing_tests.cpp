#include <gtest/gtest.h>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/battery/TEST-FAKE-BATTERY.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/parallel_safety.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

/* A pack on the DC link whose breaker trips or whose fuse blows keeps talking to BE: only its
   current, now zero while the other packs carry its share, gives it away. These pin when the
   warning comes and for which pack, what clears it, and the patterns of normal operation it must
   not take for a trip. Currents in dA, positive while charging, one pass per second as the core
   loop runs it. */

namespace {

class ParallelCurrentSharingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The global DataLayerResetListener has already reset datalayer, the events and this check
    init_events();
    battery = new TestFakeBattery();
    battery2 = new TestFakeBattery(&datalayer.battery2, CAN_Interface::CAN_NATIVE);
    battery2->battery_index = 2;
    datalayer.system.info.configured_batteries = 2;
    datalayer.system.status.battery2_allowed_contactor_closing = true;
  }

  static void add_third_pack() {
    battery3 = new TestFakeBattery(&datalayer.battery3, CAN_Interface::CAN_NATIVE);
    battery3->battery_index = 3;
    datalayer.system.info.configured_batteries = 3;
    datalayer.system.status.battery3_allowed_contactor_closing = true;
  }

  // One second: the packs publish their currents, then the check runs
  static void pass(int16_t pack1_dA, int16_t pack2_dA, int16_t pack3_dA = 0) {
    datalayer.battery.status.current_dA = pack1_dA;
    datalayer.battery2.status.current_dA = pack2_dA;
    datalayer.battery3.status.current_dA = pack3_dA;
    check_parallel_current_sharing();
  }

  static void passes(int seconds, int16_t pack1_dA, int16_t pack2_dA, int16_t pack3_dA = 0) {
    for (int s = 0; s < seconds; s++) {
      pass(pack1_dA, pack2_dA, pack3_dA);
    }
  }

  static bool warned(EVENTS_ENUM_TYPE event) { return get_event_pointer(event)->state == EVENT_STATE_ACTIVE; }

  static bool any_warning() {
    return warned(EVENT_BATTERY_NO_CURRENT) || warned(EVENT_BATTERY2_NO_CURRENT) || warned(EVENT_BATTERY3_NO_CURRENT);
  }

  // Raised at any point of the test, even if a pack carrying current again cleared it since
  static bool ever_warned() {
    return get_event_pointer(EVENT_BATTERY_NO_CURRENT)->occurences > 0 ||
           get_event_pointer(EVENT_BATTERY2_NO_CURRENT)->occurences > 0 ||
           get_event_pointer(EVENT_BATTERY3_NO_CURRENT)->occurences > 0;
  }
};

}  // namespace

TEST_F(ParallelCurrentSharingTest, ATrippedPackIsReportedOnTheThirdSecond) {
  passes(10, 80, 80);  // 8 A each

  pass(160, 0);  // Pack 2's breaker trips, pack 1 takes it all
  pass(160, 0);
  EXPECT_FALSE(any_warning());
  pass(160, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
  EXPECT_FALSE(warned(EVENT_BATTERY_NO_CURRENT));
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY2_NO_CURRENT)->data, 8);  // The amps it carried
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY2_NO_CURRENT)->level, EVENT_LEVEL_WARNING);
}

TEST_F(ParallelCurrentSharingTest, ATripWhileDischargingIsReported) {
  passes(10, -100, -100);

  passes(3, -200, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
  EXPECT_EQ(get_event_pointer(EVENT_BATTERY2_NO_CURRENT)->data, 10);
}

TEST_F(ParallelCurrentSharingTest, Pack1CanTripToo) {
  passes(10, 80, 80);

  passes(3, 0, 160);

  EXPECT_TRUE(warned(EVENT_BATTERY_NO_CURRENT));
  EXPECT_FALSE(warned(EVENT_BATTERY2_NO_CURRENT));
}

TEST_F(ParallelCurrentSharingTest, OneOfThreePacksTrips) {
  add_third_pack();
  passes(10, 80, 80, 80);

  passes(3, 120, 120, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY3_NO_CURRENT));
  EXPECT_FALSE(warned(EVENT_BATTERY_NO_CURRENT));
  EXPECT_FALSE(warned(EVENT_BATTERY2_NO_CURRENT));
}

// The pack already reported is left out, so the two left are compared with each other
TEST_F(ParallelCurrentSharingTest, ASecondPackTrippingLaterIsReportedToo) {
  add_third_pack();
  passes(10, 80, 80, 80);
  passes(10, 120, 120, 0);
  ASSERT_TRUE(warned(EVENT_BATTERY3_NO_CURRENT));

  passes(3, 240, 0, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
  EXPECT_TRUE(warned(EVENT_BATTERY3_NO_CURRENT));
  EXPECT_FALSE(warned(EVENT_BATTERY_NO_CURRENT));
}

// Each pack has to have carried 2 A: below that, noise and quantisation decide
TEST_F(ParallelCurrentSharingTest, TooLittleCurrentToTell) {
  passes(10, 15, 15);
  passes(10, 30, 0);
  EXPECT_FALSE(ever_warned());

  passes(10, 20, 20);
  passes(3, 40, 0);
  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
}

// Until the total holds still again some packs may still show what they carried before
TEST_F(ParallelCurrentSharingTest, ATripAsTheTotalHalvesIsReportedOnceItSettles) {
  passes(10, 80, 80);

  passes(6, 80, 0);
  EXPECT_FALSE(any_warning());
  pass(80, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
}

// A pack at a higher state of charge can carry nothing at all under a cloud, and even feed the
// other one, while it carried its share in full sun. It lost no more than its share of the drop.
TEST_F(ParallelCurrentSharingTest, APackAtAnotherSocUnderACloudIsNotATrip) {
  passes(20, 130, 170);
  passes(30, 0, 30);
  passes(30, -5, 35);
  passes(20, 130, 170);

  EXPECT_FALSE(ever_warned());
}

// A polled pack shows its last reading for up to 3 s. Under a cloud the fresh pack reads near zero
// next to it, which taken at face value is a trip.
TEST_F(ParallelCurrentSharingTest, AStaleReadingCatchingUpIsNotATrip) {
  passes(10, 80, 80);

  passes(3, 5, 80);  // The total fell to 1 A, pack 2 has not been polled since
  passes(10, 5, 5);

  EXPECT_FALSE(ever_warned());
}

// Nor is a pack compared with a reading taken while the total was moving: a cloud passing for a few
// seconds, pack 2 polled, pack 1 at a higher state of charge and carrying nothing under it. Next to
// pack 2's stale 3 A, pack 1's 13 A in the sun looks like most of the total.
TEST_F(ParallelCurrentSharingTest, AReadingTakenWhileTheTotalMovedIsNoReference) {
  passes(20, 0, 30);   // Cloud
  passes(3, 130, 30);  // Sun, pack 2 not polled yet
  pass(130, 170);      // Polled
  passes(3, 0, 170);   // Cloud again, pack 2 not polled yet
  passes(20, 0, 30);

  EXPECT_FALSE(ever_warned());
}

TEST_F(ParallelCurrentSharingTest, APackBeDropsItselfIsNotATrip) {
  passes(10, 80, 80);

  datalayer.system.status.battery2_allowed_contactor_closing = false;  // Out of sync, or a fault
  passes(10, 160, 0);

  EXPECT_FALSE(ever_warned());
}

// A tripped pack's voltage drifts away from the others', and the voltage check drops it
TEST_F(ParallelCurrentSharingTest, ATripIsStillReportedWhenTheVoltageCheckDropsThePack) {
  passes(10, 80, 80);
  pass(160, 0);

  datalayer.system.status.battery2_allowed_contactor_closing = false;
  passes(2, 160, 0);

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
}

TEST_F(ParallelCurrentSharingTest, APackThatNeverCarriedCurrentIsNotReported) {
  passes(30, 160, 0);

  EXPECT_FALSE(ever_warned());
}

TEST_F(ParallelCurrentSharingTest, TheWarningStaysUntilThePackCarriesCurrentAgain) {
  passes(10, 80, 80);
  passes(3, 160, 0);
  ASSERT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));

  passes(600, 0, 0);  // Evening: nobody carries anything
  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));

  reset_all_events();  // Cleared from the events page, the breaker still open
  pass(0, 0);
  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));

  passes(2, 80, 80);  // Breaker reset
  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
  pass(80, 80);
  EXPECT_FALSE(warned(EVENT_BATTERY2_NO_CURRENT));

  passes(10, 80, 80);  // Watched again
  passes(3, 160, 0);
  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
}

TEST_F(ParallelCurrentSharingTest, ASingleBatteryIsLeftAlone) {
  datalayer.system.info.configured_batteries = 1;

  passes(10, 80, 80);
  passes(10, 160, 0);

  EXPECT_FALSE(ever_warned());
}

// Through the safety pass the core loop runs: reported, and nothing stopped
TEST_F(ParallelCurrentSharingTest, TheSafetyPassReportsItAndStopsNothing) {
  datalayer.system.info.CPU_free_heap = 200000;  // Keep the low-heap check quiet
  emulator_pause_request_ON = false;             // Globals other suites leave behind
  emulator_pause_status = NORMAL;
  battery_detected = true;
  battery2_detected = true;
  battery3_detected = false;
  for (DATALAYER_BATTERY_TYPE* pack : {&datalayer.battery, &datalayer.battery2}) {
    pack->status.voltage_dV = 3700;
    pack->status.real_soc = 5000;
    pack->status.cell_max_voltage_mV = 3800;
    pack->status.cell_min_voltage_mV = 3750;
  }
  datalayer.aggregate.real_soc = 5000;
  datalayer.aggregate.reported_soc = 5000;
  auto run_cycle = [](int16_t pack1_dA, int16_t pack2_dA) {
    for (DATALAYER_BATTERY_TYPE* pack : {&datalayer.battery, &datalayer.battery2}) {
      pack->status.max_charge_power_W = 5000;
      pack->status.max_discharge_power_W = 5000;
      pack->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    }
    datalayer.battery.status.current_dA = pack1_dA;
    datalayer.battery2.status.current_dA = pack2_dA;
    update_machineryprotection(0);
  };
  for (int s = 0; s < 10; s++) {
    run_cycle(80, 80);
  }

  for (int s = 0; s < 3; s++) {
    run_cycle(160, 0);
  }

  EXPECT_TRUE(warned(EVENT_BATTERY2_NO_CURRENT));
  EXPECT_EQ(datalayer.system.status.system_status, ACTIVE);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery2.status.max_charge_power_W, 5000u);
  EXPECT_TRUE(datalayer.system.status.battery2_allowed_contactor_closing);
}
