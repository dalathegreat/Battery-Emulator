#include "ModbusInverterProtocol.h"
#include "../lib/eModbus-eModbus/ModbusServerRTU.h"

static const int MB_RTU_NUM_VALUES = 13100;
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory

ModbusInverterProtocol::ModbusInverterProtocol() {
  mbPV = ::mbPV;
  MBserver = new ModbusServerRTU(2000);
}
