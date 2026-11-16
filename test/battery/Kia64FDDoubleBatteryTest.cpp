#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "../../Software/src/battery/KIA-64FD-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"

namespace {

// Independent CRC-16/XMODEM implementation used to build valid test frames.
uint16_t test_crc16_step(uint16_t crc, uint8_t b) {
  crc ^= static_cast<uint16_t>(b) << 8;
  for (int i = 0; i < 8; i++) {
    crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

// Writes a valid little-endian CRC into data[0..1] of a 32 byte 0x215 frame.
void apply_valid_crc(CAN_frame& frame) {
  uint16_t crc = 0;
  for (int i = 2; i < 32; i++) {
    crc = test_crc16_step(crc, frame.data.u8[i]);
  }
  crc = test_crc16_step(crc, 0x15);  // CAN ID 0x215, low byte
  crc = test_crc16_step(crc, 0x02);  // CAN ID 0x215, high byte
  crc ^= 0x9F5B;
  frame.data.u8[0] = crc & 0xFF;
  frame.data.u8[1] = crc >> 8;
}

// Builds a CAN_frame from raw bytes (32 bytes expected).
CAN_frame frame_from_bytes(const uint8_t (&bytes)[32]) {
  CAN_frame frame = {};
  frame.ID = 0x215;
  frame.DLC = 32;
  std::memcpy(frame.data.u8, bytes, 32);
  return frame;
}

// One 0x215 cell-voltage frame: mux 0x01, batch starting at cell 1, one little-endian mV value.
// 32 bytes with a valid CRC, as the BMS sends it.
CAN_frame cell_frame(uint16_t mv) {
  CAN_frame frame = {};
  frame.ID = 0x215;
  frame.DLC = 32;
  frame.data.u8[3] = 0x01;
  frame.data.u8[4] = 0x01;
  frame.data.u8[5] = mv & 0xFF;
  frame.data.u8[6] = mv >> 8;
  apply_valid_crc(frame);
  return frame;
}

void reset_cells() {
  datalayer.battery.status = DATALAYER_BATTERY_STATUS_TYPE{};
  datalayer.battery2.status = DATALAYER_BATTERY_STATUS_TYPE{};
}

// Captured from a real pack (counter 0x13, mux 0x01, cells 14-26).
const uint8_t kValidFrame[32] = {0x2b, 0x56, 0x13, 0x01, 0x1b, 0xed, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d,
                                 0xf0, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0xef, 0x0d, 0xf0, 0x0d, 0xf0,
                                 0x0d, 0xef, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0x00};

// Same frame with the first voltage byte corrupted (0xed -> 0xe1), CRC bytes untouched.
const uint8_t kCorruptFrame[32] = {0x2b, 0x56, 0x13, 0x01, 0x1b, 0xe1, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d,
                                   0xf0, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0xef, 0x0d, 0xf0, 0x0d, 0xf0,
                                   0x0d, 0xef, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0xf0, 0x0d, 0x00};

}  // namespace

TEST(Kia64FDDoubleBatteryTest, SecondBatteryCellVoltagesLandInItsOwnDatalayer) {
  reset_cells();
  Kia64FDBattery second(&datalayer.battery2, CAN_Interface::CANFD_NATIVE);

  second.handle_incoming_can_frame(cell_frame(3692));
  second.update_values();

  EXPECT_EQ(datalayer.battery2.status.cell_voltages_mV[0], 3692);
  EXPECT_EQ(datalayer.battery2.status.cell_max_voltage_mV, 3692);
  EXPECT_EQ(datalayer.battery2.status.cell_min_voltage_mV, 3692);

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 0);
}

TEST(Kia64FDDoubleBatteryTest, FirstBatteryStillWritesThePrimaryDatalayer) {
  reset_cells();
  Kia64FDBattery first(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  first.handle_incoming_can_frame(cell_frame(3701));
  first.update_values();

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[0], 3701);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 3701);
  EXPECT_EQ(datalayer.battery2.status.cell_voltages_mV[0], 0);
}

TEST(Kia64FDCrcTest, ValidFrameIsAcceptedAndParsed) {
  reset_cells();
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  battery.handle_incoming_can_frame(frame_from_bytes(kValidFrame));

  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0u);
  // Start cell is 0x1b = 27 (1-based) -> index 26; first value is 0x0ded = 3565 mV
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[26], 0x0ded);
  // Second value 0x0df0 = 3568 mV
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[27], 0x0df0);
}

TEST(Kia64FDCrcTest, CorruptFrameIsRejectedAndIncrementsErrorCounter) {
  reset_cells();
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  battery.handle_incoming_can_frame(frame_from_bytes(kCorruptFrame));

  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1u);
  // Nothing from the corrupt frame may reach the datalayer
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[26], 0);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[27], 0);
}

TEST(Kia64FDCrcTest, EachCorruptFrameIncrementsErrorCounterOnce) {
  reset_cells();
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  battery.handle_incoming_can_frame(frame_from_bytes(kCorruptFrame));
  battery.handle_incoming_can_frame(frame_from_bytes(kCorruptFrame));
  battery.handle_incoming_can_frame(frame_from_bytes(kValidFrame));

  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 2u);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[26], 0x0ded);
}

TEST(Kia64FDCrcTest, CorruptCrcBytesAreRejected) {
  reset_cells();
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  CAN_frame frame = frame_from_bytes(kValidFrame);
  frame.data.u8[0] ^= 0x01;  // flip one bit in the CRC itself
  battery.handle_incoming_can_frame(frame);

  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1u);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[26], 0);
}

TEST(Kia64FDCrcTest, SingleBitFlipInEveryByteIsDetected) {
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  for (int byte = 0; byte < 32; byte++) {
    for (int bit = 0; bit < 8; bit++) {
      reset_cells();
      CAN_frame frame = frame_from_bytes(kValidFrame);
      frame.data.u8[byte] ^= (1 << bit);
      battery.handle_incoming_can_frame(frame);
      EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1u) << "byte " << byte << " bit " << bit;
    }
  }
}

TEST(Kia64FDCrcTest, WrongLengthFrameIsRejected) {
  reset_cells();
  Kia64FDBattery battery(&datalayer.battery, CAN_Interface::CAN_NATIVE);

  CAN_frame frame = frame_from_bytes(kValidFrame);
  frame.DLC = 8;
  battery.handle_incoming_can_frame(frame);

  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1u);
}