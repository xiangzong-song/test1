#ifndef _UART_MANAGER_H_
#define _UART_MANAGER_H_
#include <stdint.h>
#include <unistd.h>
#include "hal_gpio.h"


#define UART_DEBUG_PRINT_SEND_FLAG         0x01

typedef enum _uart_baud_e
{
    BAUD_1200 = 0,
    BAUD_2400,
    BAUD_4800,
    BAUD_9600,
    BAUD_14400,
    BAUD_19200,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200,
    BAUD_230400,
    BAUD_460800,
    BAUD_921600
} uart_baud_e;

typedef enum _uart_id_e
{
    UART_ID_0 = 0,
    UART_ID_1,
    UART_ID_COUNTS
} uart_id_e;

typedef enum _uart_channel_e
{
    UART_CHANNEL_BASE = 0,
    UART_CHANNEL_OTHER
} uart_channel_e;

typedef enum _uart_transparent_e
{
    TRANSPARENT_DISABLE = 0,
    TRANSPARENT_READ_SINGLE,
    TRANSPARENT_READ_CYCLE
} uart_transparent_e;

typedef struct _uart_config_t
{
    uint8_t b_share;
    hal_port_t rx;
    hal_port_t tx_base;
    hal_port_t tx_other;
    uart_baud_e speed;
    uint32_t buffer_size;
} uart_config_t;

typedef struct _uart_data_t
{
    uint8_t command;
    uint8_t* buffer;
    uint32_t size;
} uart_data_t;

typedef void (*uart_msg)(uart_id_e, uint8_t, uint8_t*, size_t, void*);
typedef void (*uart_trans_cb)(uint8_t);
typedef uint8_t (*uart_hci_cb)(void);


int LightService_uart_manager_init(uart_id_e id, uart_config_t config);
int LightService_uart_manager_register(uart_msg callback, void* args);
void LightService_uart_manager_unregister(uart_msg callback);
void LightService_uart_manager_deinit(uart_id_e id);
int LightService_uart_manager_send(uart_id_e id, uart_data_t data);
int LightService_uart_manager_send_channel(uart_id_e id, uart_data_t data, uart_channel_e channel);
void LightService_uart_manager_print_set(uint8_t* type, uint8_t count, uint8_t flag);
void LightService_uart_manager_transparent(uart_transparent_e type, uart_id_e id, uart_trans_cb cb);
int LightService_uart_manager_hci_mode(uart_id_e id, uart_hci_cb cb);

#endif
