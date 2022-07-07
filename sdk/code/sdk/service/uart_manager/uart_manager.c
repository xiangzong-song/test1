#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "os_mem.h"
#include "driver_uart.h"
#include "driver_gpio.h"
#include "driver_system.h"
#include "hal_os.h"
#include "sys_queue.h"
#include "ringbuffer.h"
#include "log.h"
#include "runtime.h"
#include "uart_manager.h"


#define GPIO_PORT_FUNC_GPIO                 0x00
#define GPIO_PORT_FUNC_UART0                0x05
#define GPIO_PORT_FUNC_UART1                0x06
#define GPIO_FUNC_UART(x)                   ((x == UART_ID_0) ? GPIO_PORT_FUNC_UART0 : GPIO_PORT_FUNC_UART1)
#define UART_PORT(x)                        ((x == UART_ID_0) ? UART0 : UART1)
#define UART_IRQ(x)                         ((x == UART_ID_0) ? UART0_IRQn : UART1_IRQn)

#define UART_RING_BUFFER_MIN                5
#define UART_MSG_MAX_SIZE                   820
#define UART_MSG_START_CODE                 0x55
#define UART_MSG_VERSION                    0x01

#define UART_DEBUG_TYPE_MAX                 40

#define UART_TASK_INIT_CHECK                do { \
                                            	if (!g_task_init_flag) \
                                                { \
                                                    TAILQ_INIT(&g_uart_queue); \
                                                    LightRuntime_loop_task_register(uart_msg_process, 0, NULL); \
                                                    g_task_init_flag = 1; \
                                                } \
                                            } while (0)

typedef enum
{
    READ_HEADER = 0,
    READ_PAYLOAD,
    SEARCH_HEADER
} uart_state_e;

typedef enum
{
    UART_RECV = 0,
    UART_SEND,
} uart_type_e;

typedef struct
{
    uint8_t start;
    uint8_t version;
    uint32_t size;
    uint8_t command;
} __attribute__ ((packed)) uart_header_t;

struct uart_entry
{
    uart_msg callback;
    void* args;
    TAILQ_ENTRY(uart_entry) entry;
};

TAILQ_HEAD(uart_queue, uart_entry);


static struct uart_queue g_uart_queue;
static uint8_t g_task_init_flag = 0;
static LR_handler gp_uart_lr[UART_ID_COUNTS] = {NULL, NULL};
static uint8_t g_debug_print_type[UART_DEBUG_TYPE_MAX] = {0};
static uint8_t g_debug_print_flag = 0;
static uart_transparent_e g_transparent_type[UART_ID_COUNTS] = {TRANSPARENT_DISABLE, TRANSPARENT_DISABLE};
static uart_trans_cb g_transparent_cb[UART_ID_COUNTS] = {NULL, NULL};
static uart_hci_cb g_hci_callback = NULL;
static uart_config_t gt_uart_config[UART_ID_COUNTS] = {{.b_share = 0}, {.b_share = 0}};


__attribute__((section("ram_code"))) void uart0_isr_ram(void)
{
    uint8_t int_id;
	uint8_t buffer[128] = {0};
    uint8_t offset = 0;
    uint8_t c;
    volatile struct uart_reg_t * const uart_reg_ram = (volatile struct uart_reg_t *)UART0_BASE;
    int_id = uart_reg_ram->u3.iir.int_id;

    if (int_id == 0x04 || int_id == 0x0c)   /* Receiver data available or Character time-out indication */
    {
        if (g_hci_callback) //HCI test mode
        {
            (*g_hci_callback)();
        }
        else
        {
            if (g_transparent_type[UART_ID_0] && g_transparent_cb[UART_ID_0]) //transparent translation mode
            {
                if (g_transparent_type[UART_ID_0] == TRANSPARENT_READ_SINGLE)
                {
                    c = uart_reg_ram->u1.data;
                    (*g_transparent_cb[UART_ID_0])(c);
                }
                else
                {
                    while (uart_reg_ram->lsr & 0x01)
                    {
                        c = uart_reg_ram->u1.data;
                        (*g_transparent_cb[UART_ID_0])(c);
                    }
                }
            }
            else
            {
                while (offset < 128 && (uart_reg_ram->lsr & 0x01))
                {
                    c = uart_reg_ram->u1.data;
                    buffer[offset++] = c;
                }

                if (gp_uart_lr[UART_ID_0])
                {
                    Lite_ring_buffer_write_data(gp_uart_lr[UART_ID_0], buffer, offset);
                }
            }
        }
    }
    else if(int_id == 0x06)
    {
        volatile uint32_t line_status = uart_reg_ram->lsr;
    }
}

__attribute__((section("ram_code"))) void uart1_isr_ram(void)
{
    uint8_t int_id;
    uint8_t buffer[128] = {0};
    uint8_t offset = 0;
    uint8_t c;
    volatile struct uart_reg_t* const uart_reg_ram = (volatile struct uart_reg_t *)UART1_BASE;
    int_id = uart_reg_ram->u3.iir.int_id;

    if (int_id == 0x04 || int_id == 0x0c )   /* Receiver data available or Character time-out indication */
    {
        if (g_transparent_type[UART_ID_1] && g_transparent_cb[UART_ID_1])
        {
            if (g_transparent_type[UART_ID_1] == TRANSPARENT_READ_SINGLE)
            {
                c = uart_reg_ram->u1.data;
                (*g_transparent_cb[UART_ID_1])(c);
            }
            else
            {
                while (uart_reg_ram->lsr & 0x01)
                {
                    c = uart_reg_ram->u1.data;
                    (*g_transparent_cb[UART_ID_1])(c);
                }
            }
        }
        else
        {
            while (offset < 128 && (uart_reg_ram->lsr & 0x01))
            {
                c = uart_reg_ram->u1.data;
                buffer[offset++] = c;
            }

            if (gp_uart_lr[UART_ID_1])
            {
                Lite_ring_buffer_write_data(gp_uart_lr[UART_ID_1], buffer, offset);
            }
        }
    }
    else if(int_id == 0x06)
    {
        volatile uint32_t line_status = uart_reg_ram->lsr;
    }
}

static void uart_data_print(uart_header_t* header, uint8_t* buffer, uint32_t len, uart_type_e type)
{
    int i = 0;
    uint8_t b_print = 0;
    uint8_t* p = (uint8_t*)header;
    uart_header_t* ph = NULL;
    uint8_t cmd = 0x0;

    if (header != NULL)
    {
        cmd = header->command;
    }
    else
    {
        ph = (uart_header_t*)buffer;
        cmd = ph->command;
    }

    for (i = 0; i < UART_DEBUG_TYPE_MAX; i++)
    {
        if (cmd == g_debug_print_type[i])
        {
            b_print = 1;
            break;
        }
    }

    if (b_print)
    {
        HAL_printf("[uart %s]: ", type == UART_RECV ? "recv" : "send");

        if (header != NULL)
        {
            for (i = 0; i < sizeof(uart_header_t); i++)
            {
                HAL_printf("%02x ", p[i]);
            }
        }

        if (buffer != NULL)
        {
            for (i = 0; i < len; i++)
            {
                HAL_printf("%02x ", buffer[i]);
            }
        }

        HAL_printf("\r\n");
    }
}

static void uart_port_switch(uart_id_e id, uart_channel_e channel)
{
    if (channel == UART_CHANNEL_BASE)
    {
        system_set_port_mux(gt_uart_config[id].tx_base.GPIO_Port, gt_uart_config[id].tx_base.GPIO_Pin, GPIO_FUNC_UART(id));
        system_set_port_mux(gt_uart_config[id].tx_other.GPIO_Port, gt_uart_config[id].tx_other.GPIO_Pin, GPIO_PORT_FUNC_GPIO);
    }
    else
    {
        system_set_port_mux(gt_uart_config[id].tx_base.GPIO_Port, gt_uart_config[id].tx_base.GPIO_Pin, GPIO_PORT_FUNC_GPIO);
        system_set_port_mux(gt_uart_config[id].tx_other.GPIO_Port, gt_uart_config[id].tx_other.GPIO_Pin, GPIO_FUNC_UART(id));
    }
}

static int uart_data_write(uart_id_e id, uint8_t* data, size_t length)
{
    if (id >= UART_ID_COUNTS)
    {
        SDK_PRINT(LOG_ERROR, "Invalid uart id\r\n");
        return -1;
    }

    uart_write(UART_PORT(id), data, length);

    return 0;
}

static int uart_data_read(uart_id_e id, uint8_t* buffer, size_t length)
{
    int data_size = 0;

    if (id >= UART_ID_COUNTS || gp_uart_lr[id] == NULL)
    {
        SDK_PRINT(LOG_ERROR, "Invalid uart id\r\n");
        return -1;
    }

    data_size = Lite_ring_buffer_size_get(gp_uart_lr[id]);
    if (data_size < length)
    {
        SDK_PRINT(LOG_ERROR, "Not enough data.\r\n");
        return -1;
    }

    Lite_ring_buffer_read_data(gp_uart_lr[id], buffer, length);

    return 0;
}

static int uart_data_size(uart_id_e id)
{
    if (id >= UART_ID_COUNTS || gp_uart_lr[id] == NULL)
    {
        SDK_PRINT(LOG_ERROR, "Invalid uart id\r\n");
        return -1;
    }

    return Lite_ring_buffer_size_get(gp_uart_lr[id]);
}

static uint8_t uart_check_sum(uart_header_t* header, uint8_t* payload, int length)
{
    uint8_t result = 0;
    uint8_t* pheader = (uint8_t*)header;
    int i = 0;

    if (header != NULL)
    {
        for (i = 0; i < sizeof(uart_header_t); i++)
        {
            result += pheader[i];
        }
    }

    if (payload != NULL)
    {
        for (i = 0; i < length; i++)
        {
            result += payload[i];
        }
    }

    return result;
}

static int uart_check_header(uart_header_t* pt_header)
{
    if (pt_header->start == UART_MSG_START_CODE && pt_header->version && pt_header->size <= UART_MSG_MAX_SIZE)
    {
        return 0;
    }

    return -1;
}

static int uart_search_header(uart_header_t* pt_header, uint8_t* search, uint32_t length, uint8_t u_id)
{
    uint8_t buffer[UART_MSG_MAX_SIZE] = {0};
    int i = 0;

    memcpy(buffer, search, sizeof(uart_header_t));
    for (i = 0; i < length; i++)
    {
        uart_data_read(u_id, buffer + sizeof(uart_header_t) + i, 1);
        if (uart_check_header((uart_header_t*)(buffer + 1 + i)) == 0)
        {
            memcpy(pt_header, buffer + 1 + i, sizeof(uart_header_t));
            return 0;
        }
    }

    memcpy(search, buffer + i, sizeof(uart_header_t));

    return -1;
}

static int uart_msg_read(uart_header_t* pt_header, uint8_t* buffer, uint8_t u_id)
{
    static uart_header_t header[UART_ID_COUNTS];
    static uint8_t state[UART_ID_COUNTS] = {READ_HEADER, READ_HEADER};
    static uint8_t search_buffer[UART_ID_COUNTS][sizeof(uart_header_t)] = {{0}, {0}};
    int data_size = 0;
    uint8_t check_sum = 0;

    while (1)
    {
        data_size = uart_data_size(u_id);

        if (state[u_id] == READ_HEADER)
        {
            if (data_size < sizeof(uart_header_t))
            {
                return -1;
            }

            memset(&header[u_id], 0, sizeof(uart_header_t));
            uart_data_read(u_id, (uint8_t*)&header[u_id], sizeof(uart_header_t));

            if (uart_check_header(&header[u_id]) != 0)
            {
                memcpy(search_buffer[u_id], &header[u_id], sizeof(uart_header_t));
                state[u_id] = SEARCH_HEADER;
            }
            else
            {
                state[u_id] = READ_PAYLOAD;
            }
        }
        else if (state[u_id] == READ_PAYLOAD)
        {
            if (data_size < header[u_id].size + 1)
            {
                return -1;
            }

            uart_data_read(u_id, buffer, header[u_id].size + 1);
            check_sum = uart_check_sum(&header[u_id], buffer, header[u_id].size);
            state[u_id] = READ_HEADER;
            break;
        }
        else if (state[u_id] == SEARCH_HEADER)
        {
            if (data_size < 1)
            {
                return -1;
            }

            memset(&header[u_id], 0, sizeof(uart_header_t));
            if (uart_search_header(&header[u_id], search_buffer[u_id], data_size, u_id) != 0)
            {
                return -1;
            }
            memset(search_buffer[u_id], 0, sizeof(uart_header_t));
            state[u_id] = READ_PAYLOAD;
        }
    }

    if (check_sum != buffer[header[u_id].size])
    {
        SDK_PRINT(LOG_ERROR, "uart %d data check sum failed. native sum :%02x, data sum : %02x\r\n", u_id, check_sum, buffer[header[u_id].size]);
        return -1;
    }
    memcpy(pt_header, &header[u_id], sizeof(uart_header_t));

    return 0;
}

static void uart_msg_process(void* args)
{
    uint8_t buffer[UART_MSG_MAX_SIZE] = {0};
    uart_header_t header;
    uint8_t i = 0;
    struct uart_entry* var = NULL;

    for (i = 0; i < UART_ID_COUNTS; i++)
    {
        if (gp_uart_lr[i] && uart_msg_read(&header, buffer, i) == 0)
        {
            if (g_sdk_log_level >= SDK_LOG_DEBUG)
            {
                uart_data_print(&header, buffer, header.size + 1, UART_RECV);
            }

            TAILQ_FOREACH(var, &g_uart_queue, entry)
            {
                if (var->callback)
                {
                    (*var->callback)(i, header.command, buffer, header.size, var->args);
                }
            }
        }
    }
}

int LightService_uart_manager_init(uart_id_e id, uart_config_t config)
{
    uint32_t buffer_len = (config.buffer_size > 0) ? config.buffer_size : UART_RING_BUFFER_MIN;

    if (id >= UART_ID_COUNTS)
    {
        SDK_PRINT(LOG_ERROR, "Wrong uart id.\r\n");
        return -1;
    }

    gp_uart_lr[id] = Lite_ring_buffer_init(buffer_len);
    if (gp_uart_lr[id] == NULL)
    {
        SDK_PRINT(LOG_ERROR, "Uart ring buffer init failed.\r\n");
        return -1;
    }

    gt_uart_config[id] = config;
    system_set_port_pull(GPIO2PORT(config.rx.GPIO_Port, config.rx.GPIO_Pin), true);
    system_set_port_mux(config.rx.GPIO_Port, config.rx.GPIO_Pin, GPIO_FUNC_UART(id));

    if (config.b_share == 0)
    {
        system_set_port_mux(config.tx_base.GPIO_Port, config.tx_base.GPIO_Pin, GPIO_FUNC_UART(id));
    }
    else
    {
        uart_port_switch(id, UART_CHANNEL_BASE);
    }

    uart_init(UART_PORT(id), config.speed);
    NVIC_EnableIRQ(UART_IRQ(id));

    UART_TASK_INIT_CHECK;

    return 0;
}

int LightService_uart_manager_register(uart_msg callback, void* args)
{
    struct uart_entry* element = NULL;

    UART_TASK_INIT_CHECK;

    if (NULL == (element = HAL_malloc(sizeof(struct uart_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc uart entry failed.\r\n");
        return -1;
    }

    element->callback = callback;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_uart_queue, element, entry);

    return 0;
}

void LightService_uart_manager_unregister(uart_msg callback)
{
    struct uart_entry* var = NULL;

    TAILQ_FOREACH(var, &g_uart_queue, entry)
    {
        if (var->callback == callback)
        {
            TAILQ_REMOVE(&g_uart_queue, var, entry);
            HAL_free(var);
            break;
        }
    }
}

void LightService_uart_manager_deinit(uart_id_e id)
{

}

int LightService_uart_manager_send(uart_id_e id, uart_data_t data)
{
    uint8_t buffer[UART_MSG_MAX_SIZE] = {0};
    uint8_t check_sum = 0;
    uart_header_t* pt_header = (uart_header_t*)buffer;

    if (data.size > UART_MSG_MAX_SIZE - sizeof(uart_header_t))
    {
        SDK_PRINT(LOG_ERROR, "uart msg is too long.\r\n");
        return -1;
    }

    pt_header->start = UART_MSG_START_CODE;
    pt_header->version = UART_MSG_VERSION;
    pt_header->command = data.command;
    pt_header->size = data.size;

    if (data.buffer && data.size > 0)
    {
        memcpy(buffer + sizeof(uart_header_t), data.buffer, data.size);
    }
    check_sum = uart_check_sum(pt_header, buffer + sizeof(uart_header_t), data.size);
    buffer[sizeof(uart_header_t) + data.size] = check_sum;

    uart_data_write(id, buffer, sizeof(uart_header_t) + data.size + 1);

    if (g_sdk_log_level >= SDK_LOG_DEBUG)
    {
        if (g_debug_print_flag & UART_DEBUG_PRINT_SEND_FLAG)
        {
            uart_data_print(NULL, buffer, sizeof(uart_header_t) + data.size + 1, UART_SEND);
        }
    }

    return 0;
}

int LightService_uart_manager_send_channel(uart_id_e id, uart_data_t data, uart_channel_e channel)
{
    int ret = 0;

    if (gt_uart_config[id].b_share == 0)
    {
        ret = LightService_uart_manager_send(id, data);
    }
    else
    {
        if (channel == UART_CHANNEL_BASE)
        {
            ret = LightService_uart_manager_send(id, data);
        }
        else
        {
            uart_finish_transfers(UART_PORT(id));
            uart_port_switch(id, channel);
            ret = LightService_uart_manager_send(id, data);
            uart_finish_transfers(UART_PORT(id));
            uart_port_switch(id, UART_CHANNEL_BASE);
        }
    }

    return ret;
}

void LightService_uart_manager_print_set(uint8_t* type, uint8_t count, uint8_t flag)
{
    if (count <= UART_DEBUG_TYPE_MAX)
    {
        memcpy(g_debug_print_type, type, count*sizeof(uint8_t));
    }
    else
    {
        memcpy(g_debug_print_type, type, UART_DEBUG_TYPE_MAX*sizeof(uint8_t));
    }

    g_debug_print_flag = flag;
}

void LightService_uart_manager_transparent(uart_transparent_e type, uart_id_e id, uart_trans_cb cb)
{
    g_transparent_type[id] = type;
    g_transparent_cb[id] = (type == TRANSPARENT_DISABLE ? NULL : cb);
}

int LightService_uart_manager_hci_mode(uart_id_e id, uart_hci_cb cb)
{
    if (id != UART_ID_0)
    {
        SDK_PRINT(LOG_ERROR, "HCI test only suppor uart 0 now.\r\n");
        return -1;
    }

    g_hci_callback = cb;

    return 0;
}
