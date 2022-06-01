#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>

void LightSdk_byte_print(uint8_t* p_data, uint32_t len, int b_newline);
void LightSdk_short_print(int16_t* p_data, uint32_t len, int b_newline);
void LightSdk_int_print(int32_t* p_data, uint32_t len, int b_newline);


#endif
