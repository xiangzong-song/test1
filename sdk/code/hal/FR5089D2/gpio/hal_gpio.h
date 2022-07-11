#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_
#include <stdint.h>

#define HAL_PA0              (1<<0)
#define HAL_PA1              (1<<1)
#define HAL_PA2              (1<<2)
#define HAL_PA3              (1<<3)
#define HAL_PA4              (1<<4)
#define HAL_PA5              (1<<5)
#define HAL_PA6              (1<<6)
#define HAL_PA7              (1<<7)

#define HAL_PB0              (1<<8)
#define HAL_PB1              (1<<9)
#define HAL_PB2              (1<<10)
#define HAL_PB3              (1<<11)
#define HAL_PB4              (1<<12)
#define HAL_PB5              (1<<13)
#define HAL_PB6              (1<<14)
#define HAL_PB7              (1<<15)

#define HAL_PC0              (1<<16)
#define HAL_PC1              (1<<17)
#define HAL_PC2              (1<<18)
#define HAL_PC3              (1<<19)
#define HAL_PC4              (1<<20)
#define HAL_PC5              (1<<21)
#define HAL_PC6              (1<<22)
#define HAL_PC7              (1<<23)

#define HAL_PD0              (1<<24)
#define HAL_PD1              (1<<25)
#define HAL_PD2              (1<<26)
#define HAL_PD3              (1<<27)
#define HAL_PD4              (1<<28)
#define HAL_PD5              (1<<29)
#define HAL_PD6              (1<<30)
#define HAL_PD7              ((uint32_t)1<<31)

#define GPIO2PORT(GPIO, BIT)  ((uint32_t)1 << ((GPIO*8) + BIT))

typedef enum _hal_port_e
{
    HAL_PORT_A,
    HAL_PORT_B,
    HAL_PORT_C,
    HAL_PORT_D,
} hal_port_e;

typedef enum _hal_port_bit_e
{
    HAL_BIT_0,
    HAL_BIT_1,
    HAL_BIT_2,
    HAL_BIT_3,
    HAL_BIT_4,
    HAL_BIT_5,
    HAL_BIT_6,
    HAL_BIT_7,
} hal_port_bit_e;

typedef struct _hal_port_t
{
    hal_port_e GPIO_Port;
    hal_port_bit_e GPIO_Pin;
} hal_port_t;
#endif
