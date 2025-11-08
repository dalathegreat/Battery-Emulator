#include "comm_IOexpander.h"

#ifdef ENABLE_IOEXPANDER
#include "MCP23017.h"

MCP23017 MCP(0x38);

// Initialization functions
bool init_IO_expander() {

  Wire.begin();

  bool b = MCP.begin();

  MCP.pinMode8(0, 0x00);
  MCP.pinMode8(1, 0x00);

  return true;
}

#endif
