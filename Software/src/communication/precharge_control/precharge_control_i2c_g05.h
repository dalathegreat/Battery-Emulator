#ifndef _PRECHARGE_CONTROL_I2C_G05_H_
#define _PRECHARGE_CONTROL_I2C_G05_H_

bool g05_i2c_init_if_needed();
bool g05_is_initialized();

void g05_enable_output();
void g05_disable_output();
void g05_set_voltage(float voltage);

#endif  // _PRECHARGE_CONTROL_I2C_G05_H_
