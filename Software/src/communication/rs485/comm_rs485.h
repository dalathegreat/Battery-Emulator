#ifndef _COMM_RS485_H_
#define _COMM_RS485_H_

#include "../../include.h"

#include "../../lib/eModbus-eModbus/Logging.h"
#include "../../lib/eModbus-eModbus/ModbusServerRTU.h"
#include "../../lib/eModbus-eModbus/scripts/mbServerFCs.h"

/**
 * @brief Initialization of RS485
 *
 * @param[in] void
 *
 * @return void
 */
void init_rs485();

#endif
