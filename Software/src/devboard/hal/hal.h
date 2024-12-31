#ifndef _HAL_H_
#define _HAL_H_

#include "../../../USER_SETTINGS.h"

#if defined(HW_LILYGO)
#include "hw_lilygo.h"
#elif defined(HW_STARK)
#include "hw_stark.h"
#elif defined(HW_3LB)
#include "hw_3LB.h"
#elif defined(HW_DEVKIT)
#include "hw_devkit.h"
#endif

#endif
