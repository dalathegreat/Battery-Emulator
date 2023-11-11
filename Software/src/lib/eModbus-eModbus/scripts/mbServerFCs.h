#include "../ModbusServerRTU.h"

#define MBTCP_ID 21  // modbus TCP server ID
#define MBPV_MAX 30000

ModbusMessage FC03(ModbusMessage request);
ModbusMessage FC06(ModbusMessage request);
ModbusMessage FC16(ModbusMessage request);
ModbusMessage FC23(ModbusMessage request);
