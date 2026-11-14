#include <gtest/gtest.h>

#include "src/datalayer/datalayer.h"

TEST(CellBalancingStatus, RoundTripAndIsolatedBits) {
  CellBalancingStatus status;

  for (size_t cell = 0; cell < MAX_AMOUNT_CELLS; cell++) {
    EXPECT_FALSE(status.get(cell)) << cell;
  }

  const size_t edges[] = {0, 1, 7, 8, 9, 95, 96, 199};
  for (size_t cell : edges) {
    status.set(cell, true);
  }
  for (size_t cell = 0; cell < MAX_AMOUNT_CELLS; cell++) {
    bool expected = false;
    for (size_t edge : edges) {
      if (edge == cell) {
        expected = true;
      }
    }
    EXPECT_EQ(status.get(cell), expected) << cell;
  }

  status.set(8, false);
  EXPECT_FALSE(status.get(8));
  EXPECT_TRUE(status.get(7));
  EXPECT_TRUE(status.get(9));

  // The Nissan publisher used to memcpy 96 bools. Setting the same 96 flags must not touch the rest.
  CellBalancingStatus copied;
  bool shunts[96] = {};
  shunts[0] = true;
  shunts[8] = true;
  shunts[95] = true;
  for (int cell = 0; cell < 96; cell++) {
    copied.set(cell, shunts[cell]);
  }
  EXPECT_TRUE(copied.get(0));
  EXPECT_TRUE(copied.get(8));
  EXPECT_TRUE(copied.get(95));
  EXPECT_FALSE(copied.get(1));
  EXPECT_FALSE(copied.get(94));
  EXPECT_FALSE(copied.get(96));
  EXPECT_FALSE(copied.get(199));
}
