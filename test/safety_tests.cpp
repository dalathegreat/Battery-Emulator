#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"

TEST(SafetyTests, ShouldSetEventWhenTemperatureTooHigh) {
  init_events();
  datalayer.system.info.CPU_temperature = 88;
  update_machineryprotection();

  auto event_pointer = get_event_pointer(EVENT_CPU_OVERHEATING);
  EXPECT_EQ(event_pointer->occurences, 1);
}

TEST(SafetyTests, ShouldSetEventWhenTemperatureWayTooHigh) {
  init_events();
  datalayer.system.info.CPU_temperature = 200;
  update_machineryprotection();

  auto event_pointer = get_event_pointer(EVENT_CPU_OVERHEATED);
  EXPECT_EQ(event_pointer->occurences, 1);
}
