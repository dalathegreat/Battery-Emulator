#pragma once

#include <stdint.h>

// Calculate the SPI clock register value for clock_hz from the apb_hz APB
// clock. We can avoid searching since all clocks we use divide the APB clock
// exactly. Others round the divisor up, so the clock is never faster than
// asked. (No ESP-IDF dependencies, so the host unit tests can use this.)
static inline uint32_t asyncSpiClockReg(uint32_t apb_hz, uint32_t clock_hz) {
  const uint32_t div = (apb_hz + clock_hz - 1) / clock_hz;
  if (div <= 1) {
    return 1u << 31;  // CLK_EQU_SYSCLK: the APB clock itself
  }
  const uint32_t pre = (div + 63) / 64;
  // We want n to be as large as possible for the best duty cycle
  const uint32_t n = (div + pre - 1) / pre;
  const uint32_t h = n / 2;  // High for half the cycle (rounded down, as per IDF)
  // CLKDIV_PRE from bit 18, CLKCNT_N from 12, CLKCNT_H from 6, CLKCNT_L from 0 (= N)
  return (pre - 1) << 18 | (n - 1) << 12 | (h - 1) << 6 | (n - 1);
}
