#ifndef CHADEMO_CT_H
#define CHADEMO_CT_H

#include <stdint.h>
#include "../devboard/utils/types.h"

uint16_t get_measured_voltage_ct();
uint16_t get_measured_current_ct();

void setup_ct(void);

#endif
