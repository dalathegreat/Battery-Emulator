#include "ModbusInverterProtocol.h"

static const int MB_RTU_NUM_VALUES = 13100;
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
