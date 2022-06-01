#ifndef _MCU_I2C_H_
#define _MCU_I2C_H_
#include <stdint.h>
#include "hal_gpio.h"

int LightSdk_mcu_i2c_init(hal_port_t scl, hal_port_t sda, uint16_t rate);
int LightSdk_mcu_i2c_write(uint8_t addr, uint8_t reg, uint8_t* buffer, uint32_t len);

#endif
