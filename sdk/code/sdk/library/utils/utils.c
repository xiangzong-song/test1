#include <stdio.h>
#include <string.h>
#include "co_printf.h"
#include "utils.h"


void LightSdk_byte_print(uint8_t* p_data, uint32_t len, int b_newline)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (b_newline && (i % 16 == 0) && (i != 0))
        {
            co_printf("\r\n");
        }

        co_printf("%02x ", p_data[i]);
    }

    co_printf("\r\n");
}

void LightSdk_short_print(int16_t* p_data, uint32_t len, int b_newline)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (b_newline && (i % 16 == 0) && (i != 0))
        {
            co_printf("\r\n");
        }

        co_printf("%5d ", p_data[i]);
    }

    co_printf("\r\n");
}

void LightSdk_int_print(int32_t* p_data, uint32_t len, int b_newline)
{
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (b_newline && (i % 16 == 0) && (i != 0))
        {
            co_printf("\r\n");
        }

        co_printf("%5d ", p_data[i]);
    }

    co_printf("\r\n");
}
