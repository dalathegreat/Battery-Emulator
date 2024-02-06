// The test library must be included first!
#include "../test_lib.h"

#include "timer.cpp"

/* Helper functions */

/* Test functions */

TEST(timer_test) {
  unsigned long test_interval = 10;
  bool result;

  // Create a timer, assert that it hasn't elapsed immediately
  testlib_millis = 30;
  MyTimer timer(test_interval);
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  // Test interval - 1, shouldn't have elapsed
  testlib_millis += test_interval - 1;
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  // Add 1, so now it should have elapsed
  testlib_millis += 1;
  result = timer.elapsed();
  ASSERT_EQ(result, true);

  // The timer should have reset when it elapsed
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  // Test close to the next interval
  testlib_millis += test_interval - 1;
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  // Add 1, ensure that the timer elapses but only once
  testlib_millis += 1;
  result = timer.elapsed();
  ASSERT_EQ(result, true);
  result = timer.elapsed();
  ASSERT_EQ(result, false);
}

TEST_MAIN();
