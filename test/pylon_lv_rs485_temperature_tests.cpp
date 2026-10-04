#include <HardwareSerial.h>
#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/PYLON-LV-RS485.h"

// Exercise the real serial request/response path, including all nine temperature
// fields and the response checksum, rather than just testing a conversion helper.
class PylonLvRs485Temperature : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    Serial2.rx_buffer.clear();
    Serial2.tx_buffer.clear();
  }

  void TearDown() override {
    Serial2.rx_buffer.clear();
    Serial2.tx_buffer.clear();
  }

  // The default request is the V3.5 "get system analog data" command (CID2 61H).
  void check_temperatures(int16_t min_dC, int16_t max_dC, unsigned min_dK, unsigned max_dK,
                          const char* request = "~200246610000FDAB\r") {
    datalayer.aggregate.temperature_min_dC = min_dC;
    datalayer.aggregate.temperature_max_dC = max_dC;
    PylonLV485InverterProtocol inverter;
    inverter.update_values();
    Serial2.rx_buffer = request;
    inverter.receive();

    const auto& frame = Serial2.tx_buffer;
    ASSERT_EQ(frame.size(), 116u);
    // CID1 46H, then RTN 00H (Normal): the reply carries a return code in the byte
    // the request used for its command, not an echo of that command.
    EXPECT_EQ(frame.substr(0, 13), "~200246008062");
    EXPECT_EQ(frame.back(), '\r');
    // Payload offsets for average/max/min cell, MOSFET and BMS temperatures.
    for (size_t offset : {38u, 42u, 58u, 62u, 70u, 78u, 82u, 90u}) {
      EXPECT_EQ(std::stoul(frame.substr(13 + offset, 4), nullptr, 16), max_dK) << offset;
    }
    EXPECT_EQ(std::stoul(frame.substr(13 + 50, 4), nullptr, 16), min_dK);
    unsigned sum = 0;
    for (size_t i = 1; i < frame.size() - 5; ++i)
      sum += static_cast<unsigned char>(frame[i]);
    sum += std::stoul(frame.substr(frame.size() - 5, 4), nullptr, 16);
    EXPECT_EQ(sum & 0xFFFFu, 0u);
  }
};

TEST_F(PylonLvRs485Temperature, TwentyFiveDegreesIsNotTwoHundredAndFifty) {
  check_temperatures(250, 250, 2981, 2981);
}

TEST_F(PylonLvRs485Temperature, PreservesTenthsAndDistinctMinMax) {
  check_temperatures(123, 256, 2854, 2987);
}

TEST_F(PylonLvRs485Temperature, ZeroDegrees) {
  check_temperatures(0, 0, 2731, 2731);
}

TEST_F(PylonLvRs485Temperature, NegativeTemperaturesIncludingBelowMinus27) {
  check_temperatures(-400, -100, 2331, 2631);
}

// The V2.x per-battery "get analog value" command (CID2 42H) gets the same answer.
TEST_F(PylonLvRs485Temperature, V2AnalogValueRequestGetsTheSameAnswer) {
  check_temperatures(250, 250, 2981, 2981, "~200246420000FDAC\r");
}
