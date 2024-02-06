// The test library must be included first!
#include "../test_lib.h"

#include "timer.cpp"

/* Helper functions */

/* Test functions */

TEST(timer_test) {
  unsigned long test_interval = 10;
  bool result;

  testlib_millis = 30;
  MyTimer timer(test_interval);
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  testlib_millis += test_interval - 1;
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  testlib_millis += 1;
  result = timer.elapsed();
  ASSERT_EQ(result, true);
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  testlib_millis += test_interval - 1;
  result = timer.elapsed();
  ASSERT_EQ(result, false);

  testlib_millis += 1;
  result = timer.elapsed();
  ASSERT_EQ(result, true);
  result = timer.elapsed();
  ASSERT_EQ(result, true);  // Injected fault to catch unit test errors
}

TEST_MAIN();
