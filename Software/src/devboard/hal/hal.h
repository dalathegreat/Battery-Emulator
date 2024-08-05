#ifndef _HAL_H_
#define _HAL_H_

#include "../../../USER_SETTINGS.h"

/* Enumeration for CAN interfaces 
typedef enum {
    CAN_NATIVE = 0,
    CANFD_NATIVE = 1,
    CAN_ADDON_MCP2515 = 2,
    CAN_ADDON_FD_MCP2518 = 3
} CAN_Interface;

/* Struct to hold CAN assignments for components
typedef struct {
    CAN_Interface battery;
    CAN_Interface battery_double;
    CAN_Interface inverter;
} CAN_Configuration;
*/

#if defined(HW_LILYGO)
#include "hw_lilygo.h"
#elif defined(HW_STARK)
#include "hw_stark.h"
#elif defined(HW_SJB_V1)
#include "hw_sjb_v1.h"
#endif

#endif
