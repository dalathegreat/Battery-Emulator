#ifndef _COMM_EQUIPMENTSTOPBUTTON_H_
#define _COMM_EQUIPMENTSTOPBUTTON_H_

#include "../../include.h"

#ifdef EQUIPMENT_STOP_BUTTON
#include "../../devboard/utils/debounce_button.h"
#endif

void init_equipment_stop_button();

void monitor_equipment_stop_button();

#endif
