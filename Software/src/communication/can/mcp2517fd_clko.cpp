#include "mcp2517fd_clko.h"

#include <Arduino.h>
#include <SPI.h>

// SPI command format (DS20005688B table 4-1): 4-bit opcode + 12-bit address.
static const uint16_t RESET_COMMAND = 0x0000;
static const uint16_t WRITE_OPCODE = 0x2;
static const uint16_t OSC_REGISTER = 0xE00;

bool mcp2517fd_clko_kick_needed(bool fd1_in_use, int clkodiv) {
  return !fd1_in_use && clkodiv != MCP2517_CLKODIV_DEFAULT;
}

void mcp2517fd_program_clko(SPIClass& spi, uint8_t cs_pin, int clkodiv) {
  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);

  // 800 kHz: no throughput needs, and legal in every state this bus can be in.
  spi.beginTransaction(SPISettings(800000, MSBFIRST, SPI_MODE0));

  // Reset first: it guarantees Configuration mode, where OSC is writable.
  digitalWrite(cs_pin, LOW);
  spi.transfer16(RESET_COMMAND);
  digitalWrite(cs_pin, HIGH);

  // CLKODIV lives in bits 6:5; PLLEN, OSCDIS and SCLKDIV stay 0.
  digitalWrite(cs_pin, LOW);
  spi.transfer16((WRITE_OPCODE << 12) | OSC_REGISTER);
  spi.transfer((uint8_t)((clkodiv & 0b11) << 5));
  digitalWrite(cs_pin, HIGH);

  spi.endTransaction();
}
