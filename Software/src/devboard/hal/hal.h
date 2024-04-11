#ifndef _HAL_H_
#define _HAL_H_

#include "../../include.h"

#if defined(HW_LILYGO)
#include "hw_lilygo.h"
#elif defined(HW_SJB_V1)
#include "hw_sjb_v1.h"
#endif

#if !defined(HW_CONFIGURED)
#error You must select a HW to run on!
#endif

#endif
