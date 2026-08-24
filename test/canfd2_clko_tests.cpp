#include <gtest/gtest.h>

#include <SPI.h>

#include "../Software/src/communication/can/mcp2517fd_clko.h"

// BECom's second MCP2518 has no crystal of its own: it is clocked from the
// first chip's CLKO pin (hw_becom.h declares CLKODIV divide-by-1). The first
// chip powers up with CLKO at divide-by-10, and only the first FD interface's
// driver programs it - so running the second interface standalone used to
// leave the second chip at 4 MHz while the driver configured it for 40 MHz,
// and it never came up. These tests pin the fix: the kick fires exactly when
// the first FD interface is absent AND the board declares a non-default
// divider, and it shifts out the reset + OSC write the datasheet specifies.

TEST(CanFd2Clko, KickOnlyWhenFd1AbsentAndDividerNonDefault) {
  // BECom with only the 2nd interface configured: nothing else programs CLKO.
  EXPECT_TRUE(mcp2517fd_clko_kick_needed(false, 0b00));
  // The 1st interface's own begin() programs the divider - no kick.
  EXPECT_FALSE(mcp2517fd_clko_kick_needed(true, 0b00));
  // Default divider means nothing consumes CLKO (stark, lilygo2can) - the
  // 2nd chip has its own clock and the 1st chip may not even be fitted.
  EXPECT_FALSE(mcp2517fd_clko_kick_needed(false, MCP2517_CLKODIV_DEFAULT));
}

TEST(CanFd2Clko, ProgramsOscViaResetThenWrite) {
  SPIClass spi;
  mcp2517fd_program_clko(spi, 14, 0b00);
  // RESET (0x0000), then WRITE (0b0010 << 12 | 0xE00) of one byte: PLLEN,
  // OSCDIS and SCLKDIV zero, CLKODIV divide-by-1 in bits 6:5.
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

TEST(CanFd2Clko, KeepsTheDeliberatelyConservativeSpiSpeed) {
  // The transaction runs at a deliberately conservative 800 kHz - it has no
  // throughput needs and that speed is legal in every state the bus can be
  // in. (CLKODIV divides only the CLKO OUTPUT: chip 1 runs from its own
  // crystal, so no divided-clock SPI limit applies to this transaction -
  // the speed is a choice, and this pins the choice.)
  SPIClass spi;
  mcp2517fd_program_clko(spi, 14, 0b00);
  EXPECT_LE(spi.transaction_clock, 800000u);
}
