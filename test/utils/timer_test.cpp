// The test library must be included first!
#include "timer.cpp"
#include "../test_lib.h"

/* Helper functions */

/* Test functions */

TEST(timer_test) {
  unsigned long test_interval = 10;

  testlib_millis = 0;
  MyTimer timer(test_interval);
  ASSERT_EQ(timer.elapsed(), false);

  testlib_millis = test_interval - 1;
  ASSERT_EQ(timer.elapsed(), false);

  testlib_millis = test_interval;
  ASSERT_EQ(timer.elapsed(), true);
  ASSERT_EQ(timer.elapsed(), false);

  testlib_millis = 2 * test_interval - 1;
  ASSERT_EQ(timer.elapsed(), false);

  testlib_millis = 2 * test_interval;
  ASSERT_EQ(timer.elapsed(), true);
  ASSERT_EQ(timer.elapsed(), false);
}

TEST_MAIN();
