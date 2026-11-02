#include <gtest/gtest.h>

#include <set>

#include "../Software/src/devboard/espnow/espnow.h"

/* The ESP-NOW protocol is consumed by receivers built against this header, so a key id may
   never be reused for a different meaning and the aggregate frame must stay distinct from a
   per-pack one. */
namespace {

TEST(EspNowAggregateKeys, KeyIdsAreUniqueAndOutsideThePerPackRange) {
  const uint8_t aggregate_keys[] = {ESPNOW_KEY_AGG_SOC_PPTT,
                                    ESPNOW_KEY_AGG_SOC_REAL_PPTT,
                                    ESPNOW_KEY_AGG_SOH_PPTT,
                                    ESPNOW_KEY_AGG_VOLTAGE_DV,
                                    ESPNOW_KEY_AGG_CURRENT_DA,
                                    ESPNOW_KEY_AGG_ACTIVE_POWER_W,
                                    ESPNOW_KEY_AGG_TOTAL_CAPACITY_WH,
                                    ESPNOW_KEY_AGG_REPORTED_CAPACITY_WH,
                                    ESPNOW_KEY_AGG_REMAINING_CAPACITY_WH,
                                    ESPNOW_KEY_AGG_REPORTED_REMAIN_WH,
                                    ESPNOW_KEY_AGG_MAX_CHARGE_POWER_W,
                                    ESPNOW_KEY_AGG_MAX_DISCHARGE_POWER_W,
                                    ESPNOW_KEY_AGG_MAX_CHARGE_CURRENT_DA,
                                    ESPNOW_KEY_AGG_MAX_DISCHARGE_CURRENT_DA,
                                    ESPNOW_KEY_AGG_CELL_MAX_MV,
                                    ESPNOW_KEY_AGG_CELL_MIN_MV,
                                    ESPNOW_KEY_AGG_TEMPERATURE_MAX_DC,
                                    ESPNOW_KEY_AGG_TEMPERATURE_MIN_DC,
                                    ESPNOW_KEY_AGG_TOTAL_CHARGED_WH,
                                    ESPNOW_KEY_AGG_TOTAL_DISCHARGED_WH,
                                    ESPNOW_KEY_AGG_CHARGING_STATE,
                                    ESPNOW_KEY_AGG_LIMITING_FACTOR};

  std::set<uint8_t> seen;
  for (uint8_t key : aggregate_keys) {
    EXPECT_TRUE(seen.insert(key).second) << "duplicate aggregate key 0x" << std::hex << (int)key;
    // Clear of the per-pack live values (0x50..0x74), the cell arrays (0x90..0x93) and the
    // event keys (0xA0..0xAA)
    EXPECT_GE(key, 0xB0);
  }
  EXPECT_EQ(seen.size(), sizeof(aggregate_keys));
}

// The pack frame's limiting factor and the installation's must stay distinct keys: a receiver
// that saw the same id in both frames could not tell whose answer it was holding.
TEST(EspNowAggregateKeys, LimitingFactorKeysDoNotCollide) {
  EXPECT_NE(ESPNOW_KEY_AGG_LIMITING_FACTOR, ESPNOW_KEY_LIMITING_FACTOR);
  EXPECT_NE(ESPNOW_KEY_AGG_CHARGING_STATE, ESPNOW_KEY_CHARGING_STATE);
}

TEST(EspNowAggregateKeys, AggregateIsItsOwnFrameType) {
  EXPECT_EQ(ESPNOW_FRAME_AGGREGATE, 0x05);
  EXPECT_NE(ESPNOW_FRAME_AGGREGATE, ESPNOW_FRAME_BATTERY);
  EXPECT_NE(ESPNOW_FRAME_AGGREGATE, ESPNOW_FRAME_SYSTEM);
}

}  // namespace
