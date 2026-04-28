#pragma once

#include "../../test/emul/Logging.h"

// Make printf actually print to the console
#undef DEBUG_PRINTF
#define DEBUG_PRINTF printf
