#include <gtest/gtest.h>

#include <string>

#include "../Software/src/devboard/utils/events.h"

/* EVENT_WEB_403 is raised by the web server at most once a minute while it refuses requests with a
 * 403 (report_refused_request() in webserver.cpp), and cleared by webserver_tick() after a minute
 * without one. Its counter therefore counts the minutes in which something was refused: it has to
 * grow with every set_event(), also while the event is still active, and keep its value across the
 * clear that ends a quiet minute. */

namespace {

class Web403EventTest : public ::testing::Test {
 protected:
  void SetUp() override {
    init_events();
    reset_all_events();
  }
  const EVENTS_STRUCT_TYPE* event() const { return get_event_pointer(EVENT_WEB_403); }
};

TEST_F(Web403EventTest, IsAWarningWithItsMessage) {
  EXPECT_STREQ(get_event_enum_string(EVENT_WEB_403), "WEB_403");
  EXPECT_STREQ(get_event_level_string(EVENT_WEB_403), "WARNING");
  EXPECT_EQ(std::string(get_event_message_string(EVENT_WEB_403).c_str()),
            "Forbidden webserver access blocked at least once in the last minute. "
            "Don't expose this device to the Internet!");
}

TEST_F(Web403EventTest, CounterGrowsWithEveryReportWhileActive) {
  set_event(EVENT_WEB_403, 0);
  set_event(EVENT_WEB_403, 0);
  set_event(EVENT_WEB_403, 0);
  EXPECT_EQ(event()->occurences, 3);
  EXPECT_EQ(event()->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(get_emulator_status(), EMULATOR_STATUS::STATUS_WARNING);
}

TEST_F(Web403EventTest, QuietMinuteClearsItButKeepsTheCount) {
  set_event(EVENT_WEB_403, 0);
  set_event(EVENT_WEB_403, 0);
  clear_event(EVENT_WEB_403);
  EXPECT_EQ(event()->state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(event()->occurences, 2);
  EXPECT_EQ(get_emulator_status(), EMULATOR_STATUS::STATUS_OK);

  set_event(EVENT_WEB_403, 0);
  EXPECT_EQ(event()->state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event()->occurences, 3);
}

TEST_F(Web403EventTest, CounterStopsAtItsMaximumInsteadOfWrapping) {
  // 300 minutes of scanning. Wrapped to 0, the event would vanish from the events page, MQTT and
  // ESP-NOW, which skip events whose count is 0.
  for (int minute = 0; minute < 300; minute++) {
    set_event(EVENT_WEB_403, 0);
  }
  EXPECT_EQ(event()->occurences, UINT8_MAX);
  EXPECT_EQ(event()->state, EVENT_STATE_ACTIVE);
}

TEST_F(Web403EventTest, DoesNotFaultTheSystem) {
  // A warning: it must not take the battery offline, only flag the exposure.
  set_event(EVENT_WEB_403, 0);
  EXPECT_NE(get_emulator_status(), EMULATOR_STATUS::STATUS_ERROR);
}

}  // namespace
