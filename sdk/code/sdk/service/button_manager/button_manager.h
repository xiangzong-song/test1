#ifndef _BUTTON_MANAGER_H_
#define _BUTTON_MANAGER_H_

#include "hal_gpio.h"

typedef enum _button_type_e
{
    BUTTON_TYPE_PRESSED = 0,
    BUTTON_TYPE_RELEASED,
    BUTTON_TYPE_SHORT_PRESSED,
    BUTTON_TYPE_MULTI_PRESSED,
    BUTTON_TYPE_LONG_PRESSED,
    BUTTON_TYPE_LONG_PRESSING,
    BUTTON_TYPE_LONG_RELEASED,
    BUTTON_TYPE_LONG_LONG_PRESSED,
    BUTTON_TYPE_LONG_LONG_RELEASED,
    BUTTON_TYPE_COMB_PRESSED,
    BUTTON_TYPE_COMB_RELEASED,
    BUTTON_TYPE_COMB_SHORT_PRESSED,
    BUTTON_TYPE_COMB_LONG_PRESSED,
    BUTTON_TYPE_COMB_LONG_PRESSING,
    BUTTON_TYPE_COMB_LONG_RELEASED,
    BUTTON_TYPE_COMB_LONG_LONG_PRESSED,
    BUTTON_TYPE_COMB_LONG_LONG_RELEASED,
} button_event_e;

typedef struct
{
    uint8_t button_id;
    hal_port_e port;
    hal_port_bit_e bits;
} button_config_t;

typedef struct
{
    uint32_t button_id;
    uint32_t button_value;
    button_event_e type;
    uint8_t count;
} button_data_t;

typedef void (*button_event)(button_data_t event, void* args);


int LightService_button_manager_init(button_config_t* buttons, int count);
int LightService_button_manager_register(button_event event, void* args);
void LightService_button_manager_unregister(button_event event);
void LightService_button_manager_deinit(void);


#endif
