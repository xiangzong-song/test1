#include <stdio.h>
#include <string.h>

#include "os_task.h"
#include "os_msg_q.h"
#include "os_mem.h"
#include "gap_api.h"
#include "sys_utils.h"
#include "jump_table.h"
#include "driver_plf.h"
#include "driver_pmu.h"
#include "driver_flash.h"
#include "driver_system.h"
#include "driver_uart.h"
#include "uart_manager.h"
#include "hci_test.h"
#include "config.h"
#include "platform.h"


#define SKU_MAX_LEN                     5
#define VER_MAX_LEN                     7


typedef enum
{
    EVT_AT,
    EVT_HCI
} evt_status_e;

typedef enum
{
    MAC_ADDR_WRITE,
    MAC_ADDR_READ,
    SKU_WRITE,
    SKU_READ,
    VER_READ,
    EARSE_TEST,
    HCI_TX_SINGLE_POWER,
    FREQ_ADJUST,
} hci_test_e;

typedef struct
{
    uint8_t sku[SKU_MAX_LEN + 1];
    uint8_t crc;
} sku_data_t;


typedef void (*rwip_eif_callback) (void*, uint8_t);

struct uart_txrxchannel
{
    /// call back function pointer
    rwip_eif_callback callback;
};

struct uart_env_tag
{
    /// rx channel
    struct uart_txrxchannel rx;
    uint32_t rxsize;
    uint8_t* rxbufptr;
    void* dummy;
    /// error detect
    uint8_t errordetect;
    /// external wakeup
    bool ext_wakeup;
};

struct user_uart_recv_t
{
    uint8_t length;
    uint8_t indx;
    uint8_t recv_data[255];
    uint8_t start_flag;
    uint8_t finish_flag;
};

struct dev_msg_init_t
{
    uint8_t dev_sw_version[VER_MAX_LEN];
    uint8_t dev_hw_version[VER_MAX_LEN];
};

struct lld_test_params
{
    /// Type (0: RX | 1: TX)
    uint8_t type;

    /// RF channel, N = (F - 2402) / 2
    uint8_t channel;

    /// Length of test data
    uint8_t data_len;

    /**
     * Packet payload
     * 0x00 PRBS9 sequence "11111111100000111101" (in transmission order) as described in [Vol 6] Part F, Section 4.1.5
     * 0x01 Repeated "11110000" (in transmission order) sequence as described in [Vol 6] Part F, Section 4.1.5
     * 0x02 Repeated "10101010" (in transmission order) sequence as described in [Vol 6] Part F, Section 4.1.5
     * 0x03 PRBS15 sequence as described in [Vol 6] Part F, Section 4.1.5
     * 0x04 Repeated "11111111" (in transmission order) sequence
     * 0x05 Repeated "00000000" (in transmission order) sequence
     * 0x06 Repeated "00001111" (in transmission order) sequence
     * 0x07 Repeated "01010101" (in transmission order) sequence
     * 0x08-0xFF Reserved for future use
     */
    uint8_t payload;

    /**
     * Tx/Rx PHY
     * For Tx PHY:
     * 0x00 Reserved for future use
     * 0x01 LE 1M PHY
     * 0x02 LE 2M PHY
     * 0x03 LE Coded PHY with S=8 data coding
     * 0x04 LE Coded PHY with S=2 data coding
     * 0x05-0xFF Reserved for future use
     * For Rx PHY:
     * 0x00 Reserved for future use
     * 0x01 LE 1M PHY
     * 0x02 LE 2M PHY
     * 0x03 LE Coded PHY
     * 0x04-0xFF Reserved for future use
     */
    uint8_t phy;
};


static evt_status_e evt_status = EVT_HCI;
static uint16_t user_at_id = 0x0;
static uint8_t tx_power_mode = 0;
static struct dev_msg_init_t dev_msg_init_p = {{0}, {0}};
static uint8_t freq_adjust_tab[0x0f] = {10, 9, 9, 8, 7, 7, 6, 6, 6, 6, 6, 5, 5, 5, 5};
static uint8_t cur_freq_adjust_val = 0;
static struct user_uart_recv_t uart_recv ={.length = 0, .indx = 0, .recv_data = {0}, .start_flag = 0, .finish_flag = 0};


extern uint8_t lld_test_start(struct lld_test_params* params);
extern uint8_t lld_test_stop(void);
extern void flash_OTP_erase(uint32_t offset);
extern void flash_OTP_write(uint32_t offset, uint32_t length, uint8_t* buffer);

#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
static uint8_t hci_sku_check_sum(uint8_t* sku, uint8_t len)
{
    uint8_t check_sum  = 0x00;
    uint8_t i = 0x00;

    for (i = 0; i < len; i++)
    {
        check_sum ^= sku[i];
    }

    return check_sum;
}

static void hci_adjust_data_store(uint8_t adjust_val)
{
    uint8_t store_data[10] = {'f', 'r', 'e', 'q', 0xff, 0xff, 'c', 'h', 'i', 'p'};

    store_data[4] = adjust_val;
    store_data[5] = adjust_val ^ 0xff;

    flash_OTP_erase(0x1000);
    flash_OTP_write(0x1000, sizeof(store_data), store_data);
    flash_OTP_erase(0x2000);
    flash_OTP_write(0x2000, sizeof(store_data), store_data);
}

static int hci_command_handler(os_event_t* param)
{
    uint8_t rsp_data[32] = {0x01, 0xE0, 0xFC};
    uint8_t* buff = param->param;
    mac_addr_t addr;
    uint8_t sku_data[7] = {0};
    struct lld_test_params test_params;
    uint8_t i = 0;
    uint8_t adjust_num = 0;

    switch (buff[0])
    {
        case MAC_ADDR_WRITE:
            {
                rsp_data[3] = 7;
                rsp_data[4] = 0x60;
                memcpy(rsp_data + 5, buff + 1, 6);
                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case MAC_ADDR_READ:
            {
                gap_address_get(&addr);
                rsp_data[3] = 7;
                rsp_data[4] = 0x61;

                for (i = 0; i < 6; i++)
                {
                    rsp_data[5 + i] = addr.addr[5 - i];
                }
                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case SKU_WRITE:
            {
                memcpy(sku_data, buff + 1, SKU_MAX_LEN + 1);
                sku_data[6] = hci_sku_check_sum(buff + 1, SKU_MAX_LEN);

                LightSdk_config_data_erase(DEVICE_SKU, DATA_ADDR_DEFAULT, 0x1000);
                LightSdk_config_data_write(DEVICE_SKU, DATA_ADDR_DEFAULT, sku_data, SKU_MAX_LEN + 2);

                rsp_data[3] = 0x01;
                rsp_data[4] = 0x62;
                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case SKU_READ:
            {
                rsp_data[3] = 0x06;
                rsp_data[4] = 0x63;

                LightSdk_config_data_read(DEVICE_SKU, DATA_ADDR_DEFAULT, sku_data, sizeof(sku_data));
                memcpy(rsp_data + 5, sku_data, SKU_MAX_LEN);
                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case VER_READ:
            {
                rsp_data[3] = 1 + 2 * VER_MAX_LEN;
                rsp_data[4] = 0x64;
                memcpy(&rsp_data[5], dev_msg_init_p.dev_sw_version, VER_MAX_LEN);
                memcpy(&rsp_data[5 + VER_MAX_LEN], dev_msg_init_p.dev_hw_version, VER_MAX_LEN);
                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case EARSE_TEST:
            break;

        case HCI_TX_SINGLE_POWER:
            {
                lld_test_stop();
                rsp_data[3] = 0x02;
                rsp_data[4] = 0x66;

                if ((buff[1] < RF_TX_POWER_MAX) && (buff[2] < 0x39))
                {
                    *(volatile uint32_t*)0x400000d0 = 0x80008000;
                    *(volatile uint8_t*)(MODEM_BASE + 0x19) = 0x0c;
                    system_set_tx_power((enum rf_tx_power_t)buff[1]); // enum rf_tx_power_t tx_power

                    test_params.type = 1;
                    test_params.channel = buff[2];
                    test_params.data_len = 0x25;
                    test_params.payload = 0;
                    test_params.phy = 1; //PHY_1MBPS_BIT;
                    lld_test_start(&test_params);
                    rsp_data[5] = 0x01;
                    tx_power_mode = 1;
                }
                else
                {
                    rsp_data[5] = 0x00;
                }

                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        case FREQ_ADJUST:
            {
                rsp_data[3] = 0x02;
                rsp_data[4] = 0x67;

                if ((buff[1] <= 1) && (buff[2] <= 100))
                {
                    i = cur_freq_adjust_val;

                    if (buff[1])
                    {
                        for (; i < 0x0f; i++)
                        {
                            adjust_num += freq_adjust_tab[i];

                            if (abs(adjust_num - buff[2]) < 10) // 10k
                            {
                                cur_freq_adjust_val = i + 1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        for (; i > 0; i--)
                        {
                            adjust_num += freq_adjust_tab[i - 1];

                            if (abs(adjust_num - buff[2]) < 10) // 10k
                            {
                                cur_freq_adjust_val = i - 1;
                                break;
                            }
                        }
                    }

                    if (cur_freq_adjust_val)
                    {
                        hci_adjust_data_store(cur_freq_adjust_val);

                        if (cur_freq_adjust_val < 8)
                        {
                            ool_write(0x10, cur_freq_adjust_val);
                        }
                        else
                        {
                            ool_write(0x10, 7);
                            ool_write(0x10, cur_freq_adjust_val);
                        }

                        rsp_data[5] = 0x01;
                    }
                    else
                    {
                        rsp_data[5] = 0x00;
                    }
                }
                else
                {
                    rsp_data[5] = 0x00;
                }

                uart_write(UART0, rsp_data, (rsp_data[3] + 4));
            }
            break;

        default:
            break;
    }

    return EVT_CONSUMED;
}

static void hci_at_command(uint8_t c)
{
    os_event_t evt;
    uart_recv.recv_data[uart_recv.indx++] = c;

    if ((uart_recv.indx > 3 ) && (uart_recv.indx >= uart_recv.recv_data[2] + 3))
    {
        if (uart_recv.recv_data[1] == 0x0E)
        {
            uart_recv.length = uart_recv.recv_data[2];

            if (uart_recv.indx == uart_recv.length + 3 )
            {
                uart_recv.finish_flag = 1;
                uart_recv.indx = 0;
                evt.param = uart_recv.recv_data + 3;
                evt.param_len = uart_recv.length;
                os_msg_post(user_at_id, &evt);
                evt_status = EVT_HCI;
            }
            else
            {
                uart_recv.indx = 0;
                uart_recv.start_flag = 0;
                uart_recv.finish_flag = 0;
                evt_status = EVT_HCI;
            }
        }
        else
        {
            uart_recv.indx = 0;
            uart_recv.start_flag = 0;
            uart_recv.finish_flag = 0;
            evt_status = EVT_HCI;
        }
    }
}

static uint8_t hci_uart_data_read(void)
{
    volatile struct uart_reg_t* uart_reg = (volatile struct uart_reg_t*)UART0_BASE;
    struct uart_env_tag* uart0_env = (struct uart_env_tag*)0x20000a40;
    rwip_eif_callback callback;
    uint8_t c;
    void* dummy;

    if (__jump_table.system_option & SYSTEM_OPTION_ENABLE_HCI_MODE)
    {
        while (uart_reg->lsr & 0x01)
        {
            c = uart_reg->u1.data;
            uart_recv.start_flag = 0;

            //AT TEST
            if ((c == 0x34) && (evt_status == EVT_HCI)/* && (uart0_env->rxsize == 1)*/)
            {
                evt_status = EVT_AT;
            }

            if (evt_status == EVT_AT)
            {
                hci_at_command(c);
            }
            else //HCI TEST
            {
                if (tx_power_mode == 0x01)
                {
                    lld_test_stop();
                    tx_power_mode = 0;
                }

                *uart0_env->rxbufptr++ = c;
                uart0_env->rxsize--;

                if ((uart0_env->rxsize == 0) && (uart0_env->rx.callback))
                {
                    uart_reg->u3.fcr.data = 0xf1;
                    NVIC_DisableIRQ(UART0_IRQn);
                    uart_reg->u3.fcr.data = 0x21;
                    callback = uart0_env->rx.callback;
                    dummy = uart0_env->dummy;
                    uart0_env->rx.callback = 0;
                    uart0_env->rxbufptr = 0;
                    callback(dummy, 0);
                    break;
                }
            }
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

static int hci_serial_data_gets(uint16_t ms, uint8_t* data_buf, uint32_t buf_size)
{
    int i, n = 0;

    for (i = 0; i < ms; i++)
    {
        co_delay_100us(10);
        n += uart_get_data_nodelay_noint(UART0, data_buf + n, buf_size - n);

        if (n == buf_size)
        {
            return n;
        }
    }

    return -1;
}

void LightSdk_hci_test_adjust(void)
{
    uint8_t get_data[10] = {0};
    uint8_t get_data1[10] = {0};
    uint8_t check_temp = 0;

    flash_OTP_read(0x1000, sizeof(get_data), get_data);
    flash_OTP_read(0x2000, sizeof(get_data1), get_data1);

    if (!memcmp(get_data, "freq", 4) && !memcmp(&get_data[6], "chip", 4))
    {
        check_temp = get_data[4] ^ 0xff;

        if (check_temp == get_data[5])
        {
            cur_freq_adjust_val = get_data[4];
        }
    }
    else if (!memcmp(get_data1, "freq", 4) && !memcmp(&get_data1[6], "chip", 4))
    {
        check_temp = get_data1[4] ^ 0xff;

        if (check_temp == get_data1[5])
        {
            cur_freq_adjust_val = get_data1[4];
        }
    }

    if (cur_freq_adjust_val && (cur_freq_adjust_val < 0x10))
    {
        if (cur_freq_adjust_val < 8)
        {
            ool_write(0x10, cur_freq_adjust_val);
        }
        else
        {
            ool_write(0x10, 7);
            ool_write(0x10, cur_freq_adjust_val);
        }
    }
}

void LightSdk_hci_test_init(uint8_t* s_version, uint8_t* h_version)
{
    system_set_port_pull(GPIO_PA2, true);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_UART0_RXD);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_UART0_TXD);
    system_set_port_pull(GPIO_PB2, true);
    system_set_port_mux(GPIO_PORT_B, GPIO_BIT_2, PORTB2_FUNC_UART1_RXD);
    system_set_port_mux(GPIO_PORT_B, GPIO_BIT_3, PORTB3_FUNC_UART1_TXD);
    uart_init(UART0, BAUD_RATE_115200);
    NVIC_EnableIRQ(UART0_IRQn);

    LightService_uart_manager_hci_mode(UART_ID_0, hci_uart_data_read);
    memcpy(dev_msg_init_p.dev_sw_version, s_version, VER_MAX_LEN);
    memcpy(dev_msg_init_p.dev_hw_version, h_version, VER_MAX_LEN);

    user_at_id = os_task_create(hci_command_handler);
}

int LightSdk_hci_test_check(void)
{
    uint8_t test_mode_get[5] = {0}; // RSP:0xBA,0xBA,0xBA,0xBA,0xBA
    uint8_t test_mode_send[5] = {0x34, 0x0E, 0x01, 0x6A, 0x01};
    uint8_t test_mode_check[] = {0x34, 0x0E, 0x02, 0x0A, 0x01};

    system_set_port_pull(GPIO_PA2, true);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_UART0_RXD);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_UART0_TXD);
    uart_init(UART0, BAUD_RATE_115200);
    co_delay_100us(50);
    uart_put_data_noint(UART0, test_mode_send, 4);

    if (hci_serial_data_gets(50, test_mode_get, 5) == 5) // wait 50ms
    {
        if (!memcmp(test_mode_get, test_mode_check, sizeof(test_mode_check)))
        {
            test_mode_send[2] = 0x02;
            uart_put_data_noint(UART0, test_mode_send, sizeof(test_mode_send));
            uart_finish_transfers(UART0);
            system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_A2);
            system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_A3);

            return 1;
        }
    }

    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_A2);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_A3);

    return 0;
}
#elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
void LightSdk_hci_test_adjust(void)
{

}

void LightSdk_hci_test_init(uint8_t* s_version, uint8_t* h_version)
{

}

int LightSdk_hci_test_check(void)
{
    return 0;
}
#else

#error "Wrong platform type."

#endif
