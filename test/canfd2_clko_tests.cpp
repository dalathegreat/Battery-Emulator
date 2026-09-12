#include <gtest/gtest.h>

#include <SPI.h>

#include "../Software/src/communication/can/mcp2517fd_clko.h"

// BECom's 2nd MCP2518 is clocked from the 1st chip's CLKO pin, which powers up at
// divide-by-10 and is otherwise only programmed by the 1st interface's begin().

TEST(CanFd2Clko, KickOnlyWhenFd1AbsentAndDividerNonDefault) {
  EXPECT_TRUE(mcp2517fd_clko_kick_needed(false, 0b00));
  EXPECT_FALSE(mcp2517fd_clko_kick_needed(true, 0b00));
  EXPECT_FALSE(mcp2517fd_clko_kick_needed(false, MCP2517_CLKODIV_DEFAULT));
}

TEST(CanFd2Clko, ProgramsOscViaResetThenWrite) {
  SPIClass spi;
  mcp2517fd_program_clko(spi, 14, 0b00);
  // RESET (0x0000), then WRITE (0b0010 << 12 | 0xE00) of one byte.
  const std::vector<uint8_t> expected = {0x00, 0x00, 0x2E, 0x00, 0x00};
  EXPECT_EQ(spi.transferred, expected);
  EXPECT_FALSE(spi.in_transaction) << "transaction must be closed";
}

TEST(CanFd2Clko, DividerLandsInBits5and6) {
  SPIClass spi;
  mcp2517fd_program_clko(spi, 14, 0b10);
  ASSERT_EQ(spi.transferred.size(), 5u);
  EXPECT_EQ(spi.transferred[4], 0b10 << 5);
}
