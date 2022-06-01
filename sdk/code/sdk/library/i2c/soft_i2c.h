#ifndef _SOFT_I2C_H_
#define _SOFT_I2C_H_

#include "hal_gpio.h"

int LightSdk_software_i2c_init(hal_port_t scl, hal_port_t sda, uint32_t speed);
int LightSdk_software_i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, size_t size);
int LightSdk_software_i2c_read(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t size);

#endif
