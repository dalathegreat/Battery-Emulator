#ifndef MCP2517FD_CLKO_H
#define MCP2517FD_CLKO_H

#include <stdint.h>

class SPIClass;

// Power-up value of the MCP2517FD CLKODIV field: clock output divided by 10
// (DS20005688B, OSC register). Boards with no consumer on the CLKO pin leave
// it alone, so a non-default MCP2517_CLKODIV() marks a board where something
// (the 2nd CAN FD chip) is clocked from the 1st chip's CLKO output.
static const int MCP2517_CLKODIV_DEFAULT = 0b11;

// True when starting the 2nd CAN FD interface must program the 1st chip's
// clock output by hand: the board declares a non-default divider, but no 1st
// FD interface is configured, so the driver that normally programs it never
// runs and the 2nd chip would be clocked 10x slow.
bool mcp2517fd_clko_kick_needed(bool fd1_in_use, int clkodiv);

// Program the 1st MCP2517FD's OSC register so its CLKO pin outputs the clock
// the board declares. Resets the chip first: it is unused in the only
// configuration that gets here, and reset guarantees Configuration mode,
// where the oscillator bits are writable.
void mcp2517fd_program_clko(SPIClass& spi, uint8_t cs_pin, int clkodiv);

#endif
