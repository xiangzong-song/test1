#include <stdint.h>
#include <string.h>
#include "ll.h"
#include "driver_uart.h"
#include "sys_utils.h"
#include "hal_os.h"
#include "sys_queue.h"
#include "ipc_manager.h"
#include "log.h"
#include "uart_manager.h"
#include "runtime.h"


#define IPC_UART                            UART_ID_0
#define IPC_SEND_TIMEOUT                    50
#define IPC_DATA_BUFFER_MAX                 1024
#define IPC_START_CODE                      0x66

#define IPC_TASK_INIT_CHECK                 do { \
                                            	if (!g_ipc_init_flag) \
                                                { \
                                                    TAILQ_INIT(&g_ipc_queue); \
                                                    g_ipc_init_flag = 1; \
                                                } \
                                            } while (0)

typedef enum
{
    OPCODE = 0x00,
    LENGTH_L,
    LENGTH_H,
    FIFOLEN,
    RECVDATA,
    LASTDATA,
    RSPDATA
} recv_process_t;

typedef enum
{
    START = 0xA5,
    CONT,
    END,
    SINGLE,
    RSP
} header_opcode_t;

typedef struct
{
    uint8_t* rx_data;
    uint16_t rx_index;
    uint8_t rx_state;
    uint8_t rx_opcode;
    uint8_t tx_wait;
} data_state_t;

typedef struct
{
    uint8_t* ipc_data;
    uint16_t size;
} ipc_data_t;

typedef struct
{
    uint8_t start_code;
    uint8_t version;
    uint32_t length;
    uint8_t command;
} __attribute__ ((packed)) ipc_header_t;

struct ipc_entry
{
    ipc_msg callback;
    void* args;
    TAILQ_ENTRY(ipc_entry) entry;
};

TAILQ_HEAD(ipc_queue, ipc_entry);


static struct ipc_queue g_ipc_queue;
static uint8_t g_ipc_init_flag = 0;
static data_state_t gt_data_state = {NULL, 0, 0, 0, 0};
static uint16_t g_ipc_task_even_id = 0;
static uint8_t g_ipc_data_buffer[IPC_DATA_BUFFER_MAX] = {0};


/*
1. 每次收数据区malloc，耗资源
2. 如果数据长度出错，malloc不到buffer，会卡死
3. 中断里直接进行数据处理，是否会有问题
4. 收rsp timeout 处理是否合理
*/

static uint8_t ipc_check_sum(uint8_t* p_data, uint32_t length)
{
    int i = 0;
    uint8_t check_sum = 0;

    if (p_data != NULL)
    {
        for (i = 0; i < length; i++)
        {
            check_sum += p_data[i];
        }
    }

    return check_sum;
}

static void ipc_fifo_response_fill(void)
{
    volatile struct uart_reg_t *uart_reg = (volatile struct uart_reg_t *)UART0;

    GLOBAL_INT_DISABLE();
    uart_reg->u1.data = RSP;
    uart_reg->u1.data = 0x00;
    GLOBAL_INT_RESTORE();
}

static int ipc_fifo_data_fill(uint8_t opcode, uint8_t* data, uint8_t length, uint16_t param_len)
{
    volatile struct uart_reg_t *uart_reg = (volatile struct uart_reg_t *)UART0;
    uint8_t send_size = 0;
    uint32_t out_time = IPC_SEND_TIMEOUT*100;

    gt_data_state.tx_wait = 1;

    GLOBAL_INT_DISABLE();
    uart_reg->u1.data = opcode;
    if (opcode == START)
    {
        uart_reg->u1.data = param_len & 0xff;
        uart_reg->u1.data = (param_len >> 8) & 0xff;
    }

    uart_reg->u1.data = length;
    while (send_size < length)
    {
        uart_reg->u1.data = data[send_size++];
    }
    GLOBAL_INT_RESTORE();

    while (gt_data_state.tx_wait)
    {
        co_delay_10us(1);
        if (out_time-- == 0)
        {
            return -1;
        }
    }

    return 0;
}

static void ipc_rx_state_clear(void)
{
    gt_data_state.rx_state = OPCODE;
    gt_data_state.rx_index = 0;
}

static void ipc_rx_buffer_malloc(uint32_t length)
{
    if (gt_data_state.rx_data != NULL)
    {
        HAL_free(gt_data_state.rx_data);
        gt_data_state.rx_data = NULL;
    }

    gt_data_state.rx_data = (uint8_t*)HAL_malloc(length);
    if (gt_data_state.rx_data == NULL)
    {
        while(1);
    }
}

__attribute__((section("ram_code"))) static void ipc_opcode_receive(uint8_t data, uint16_t* p_length)
{
    switch (data)
    {
        case START:
            *p_length = 0;
            gt_data_state.rx_state = LENGTH_L;
            break;
        case CONT:
        case END:
            gt_data_state.rx_state = (*p_length != 0) ? FIFOLEN : OPCODE;
            break;
        case SINGLE:
            gt_data_state.rx_state = FIFOLEN;
            *p_length = 32;
            ipc_rx_buffer_malloc(*p_length);
            break;
        case RSP:
            gt_data_state.rx_state = RSPDATA;
            break;

        default:
            gt_data_state.rx_data = OPCODE;
            break;
    }

    gt_data_state.rx_opcode = data;
}

__attribute__((section("ram_code"))) static void ipc_data_receive(uint8_t data)
{
    static uint8_t fifo_index = 0;
    static uint8_t fifo_length = 0;
    static uint16_t param_length = 0;
    ipc_data_t data_recv;

    switch (gt_data_state.rx_state)
    {
        case OPCODE:
            ipc_opcode_receive(data, &param_length);
            break;
        case LENGTH_L:
            param_length = data & 0xff;
            gt_data_state.rx_state = LENGTH_H;
            break;
        case LENGTH_H:
            param_length = (data << 8) | param_length;
            gt_data_state.rx_state = FIFOLEN;
            ipc_rx_buffer_malloc(param_length);
            break;
        case FIFOLEN:
            fifo_index = 0;
            fifo_length = data;
            if (gt_data_state.rx_opcode == START || gt_data_state.rx_opcode == CONT)
            {
                gt_data_state.rx_state = RECVDATA;
            }
            else if (gt_data_state.rx_opcode == END || gt_data_state.rx_opcode == SINGLE)
            {
                gt_data_state.rx_state = LASTDATA;
            }
            break;
        case RECVDATA:
            fifo_index++;
            gt_data_state.rx_data[gt_data_state.rx_index++] = data;
            if (fifo_index == fifo_length)
            {
                ipc_fifo_response_fill();
                fifo_index = 0;
                fifo_length = 0;
                gt_data_state.rx_state = 0;
            }
            break;
        case LASTDATA:
            fifo_index++;
            gt_data_state.rx_data[gt_data_state.rx_index++] = data;
            if (fifo_index == fifo_length)
            {
                memcpy(g_ipc_data_buffer, gt_data_state.rx_data, gt_data_state.rx_index);
                data_recv.ipc_data = g_ipc_data_buffer;
                data_recv.size = gt_data_state.rx_index;
                LightRuntime_event_task_post(g_ipc_task_even_id, &data_recv, sizeof(ipc_data_t));

                if (gt_data_state.rx_data)
                {
                    HAL_free(gt_data_state.rx_data);
                    gt_data_state.rx_data = NULL;
                }
                fifo_index = 0;
                fifo_length = 0;

                ipc_rx_state_clear();
                ipc_fifo_response_fill();
            }
            break;
        case RSPDATA:
            gt_data_state.tx_wait = 0;
            gt_data_state.rx_state = OPCODE;
            break;

        default:
            break;
    }
}

static int ipc_data_valid_check(uint8_t* data, int length, uint8_t* cmd, uint32_t* payload_len)
{
    ipc_header_t* pt_header = (ipc_header_t*)data;
    uint8_t check_sum = 0;

    if (pt_header->start_code != IPC_START_CODE)
    {
        SDK_PRINT(LOG_ERROR, "Wrong ipc start code\r\n");
        return -1;
    }

    check_sum = ipc_check_sum(data, length - 1);
    if (check_sum != data[length - 2])
    {
        SDK_PRINT(LOG_ERROR, "Wrong ipc check sum, %02x, %02x\r\n", check_sum, data[length - 2]);
        return -1;
    }

    if (length - sizeof(ipc_header_t) - 1 != pt_header->length)
    {
        SDK_PRINT(LOG_ERROR, "Wrong ipc data length, total : %d, payload : %d\r\n", length, pt_header->length);
        return -1;
    }

    *cmd = pt_header->command;
    *payload_len = pt_header->length;

    return 0;
}

static void ipc_data_receive_task(void* param, void* args)
{
    ipc_data_t data_recv = *(ipc_data_t*)param;
    struct ipc_entry* var = NULL;
    uint8_t command = 0;
    uint32_t len = 0;

    if (ipc_data_valid_check(data_recv.ipc_data, data_recv.size, &command, &len) == 0)
    {
        TAILQ_FOREACH(var, &g_ipc_queue, entry)
        {
            if (var->callback)
            {
                (*var->callback)(command, data_recv.ipc_data + sizeof(ipc_header_t), len, var->args);
            }
        }
    }
}

int LightService_ipc_manager_send(uint8_t *data, uint16_t len)
{
    uint16_t count = 0;
    uint8_t last_len = 0;
    uint16_t len_temp = 0;
    int16_t i = 0;

    if (len <= 28)
    {
        if (ipc_fifo_data_fill(SINGLE, &data[0], len, 0) != 0)
        {
            return -1;
        }
    }
    else
    {
        len_temp = len - 26; //first fifo have param length two byte
        count = len_temp / 28;
        last_len = len_temp % 28;

        if (ipc_fifo_data_fill(START, &data[0], 26, len) != 0)
        {
            return -1;
        }

        for (i = 0; i < count; i++)
        {
            if (ipc_fifo_data_fill(CONT, &data[26 + (i*28)], 28, 0) != 0)
            {
                return -1;
            }
        }

        if (last_len)
        {
            if (ipc_fifo_data_fill(END, &data[len - last_len], last_len, 0) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int LightService_ipc_manager_register(ipc_msg callback, void* args)
{
    struct ipc_entry* element = NULL;

    IPC_TASK_INIT_CHECK;

    if (NULL == (element = HAL_malloc(sizeof(struct ipc_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc ipc entry failed.\r\n");
        return -1;
    }

    element->callback = callback;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_ipc_queue, element, entry);

    return 0;
}

void LightService_ipc_manager_unregister(ipc_msg callback)
{
    struct ipc_entry* var = NULL;

    TAILQ_FOREACH(var, &g_ipc_queue, entry)
    {
        if (var->callback == callback)
        {
            TAILQ_REMOVE(&g_ipc_queue, var, entry);
            HAL_free(var);
            break;
        }
    }
}

void LightService_ipc_manager_deinit(void)
{

}

int LightService_ipc_manager_init(void)
{
    uart_config_t config = {0, {HAL_PORT_A, HAL_BIT_2}, {HAL_PORT_A, HAL_BIT_3}, {}, BAUD_921600, 0};

    LightService_uart_manager_transparent(TRANSPARENT_READ_CYCLE, IPC_UART, ipc_data_receive);
    LightService_uart_manager_init(IPC_UART, config);
    LightRuntime_event_task_register(&g_ipc_task_even_id, ipc_data_receive_task, NULL);

    IPC_TASK_INIT_CHECK;

    SDK_PRINT(LOG_DEBUG, "IPC manager init.\r\n");

    return 0;
}

