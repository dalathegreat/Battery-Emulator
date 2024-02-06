// The test library must be included first!
#include "test_lib.h"

#include "events.cpp"

/* Helper functions */
static void reset_event_msg(void) {
  snprintf(event_message, sizeof(event_message), "");
}

TEST(init_events_test) {
  init_events();

  for (uint8_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    ASSERT_EQ(entries[i].occurences, 0);
  }
}

TEST(update_event_timestamps_test) {
  // Reset
  init_events();
  time_seconds = 0;

  // No delta, so time shouldn't increase
  testlib_millis = 0;
  update_event_timestamps();
  ASSERT_EQ(time_seconds, 0);

  // Almost time to bump the seconds
  testlib_millis = 999;
  update_event_timestamps();
  ASSERT_EQ(time_seconds, 0);
  ASSERT_EQ(previous_millis, 0);

  // millis == 1000, so we should add a second
  testlib_millis = 1000;
  update_event_timestamps();
  ASSERT_EQ(time_seconds, 1);
  ASSERT_EQ(previous_millis, 1000);

  // We shouldn't add more seconds until 2000 now
  testlib_millis = 1999;
  update_event_timestamps();
  ASSERT_EQ(time_seconds, 1);
  ASSERT_EQ(previous_millis, 1000);
  testlib_millis = 2000;
  update_event_timestamps();
  ASSERT_EQ(time_seconds, 2);
  ASSERT_EQ(previous_millis, 2000);
}

TEST(set_event_test) {
  // Reset
  init_events();
  time_seconds = 0;

  // Initially, the event should not have any data or occurences
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].data, 0);
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].occurences, 0);
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].timestamp, 0);
  // Set current time and overvoltage event for cell 23
  time_seconds = 345;
  set_event(EVENT_CELL_OVER_VOLTAGE, 123);
  // Ensure proper event data
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].data, 123);
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].occurences, 1);
  ASSERT_EQ(entries[EVENT_CELL_OVER_VOLTAGE].timestamp, 345);
}

TEST(event_message_test) {
  reset_event_msg();

  set_event(EVENT_DUMMY, 0);  // Set dummy event with no data

  ASSERT_STREQ("The dummy event was set!", event_message);
}

TEST_MAIN();
