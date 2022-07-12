#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "os_mem.h"
#include "os_timer.h"
#include "gatt_api.h"
#include "gap_api.h"
#include "gatt_sig_uuid.h"
#include "ota.h"
#include "ota_service.h"
#include "ringbuffer.h"
#include "log.h"
#include "device_time.h"
#include "utils.h"
#include "sys_queue.h"
#include "uart_manager.h"
#include "runtime.h"
#include "ble_manager.h"
#include "hal_os.h"
#include "platform.h"


#define BLE_WIFI_PT_MSG_CMD_RESPONSE        0x63

#define BLE_MULTI_MSG_DATA_LEN              16
#define BLE_NEW_MULTI_DATA_LEN              17

#define MULTI_PACKAGE                       0x01
#define MULTI_PACKAGE_START                 0x02
#define MULTI_PACKAGE_END                   0x04

#define MSG_SET_HEAD                        0x33
#define MSG_GET_HEAD                        0xaa
#define MSG_GROUP_CONTROL                   0xa5
#define MSG_GROUP_CONTROL_V2                0xb0
#define MSG_HARDWARE_TEST                   0x66
#define MSG_MUTL_SET                        0xa1
#define MSG_NEW_MUTL_SET                    0xa3

#define BLE_MULT_PACKAGE_MAX_SIZE           512

#define BLE_BONDING_INFO_SAVE_ADDR          BLE_BONDING_INFO_ADDR
#define BLE_REMOTE_SERVICE_SAVE_ADDR        BLE_REMOTE_SERVICE_ADDR

#define BLE_ADV_BUFFER_LEN                  32
#define BLE_DEV_BUFFER_LEN                  32
#define GATT_CHAR1_DESC_LEN                 20
#define GATT_CHAR2_DESC_LEN                 20
#define GATT_CHAR1_VALUE_LEN                20
#define GATT_CHAR2_VALUE_LEN                20
#define GOVEE_GATT_SVC1_TX_UUID_128         "\x10\x2B\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00"
#define GOVEE_GATT_SVC1_RX_UUID_128         "\x11\x2B\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00"
#define BLE_GATT_MSG_BUFFER_SIZE            (sizeof(msg_packet_t)*24)

#define BLE_TASK_INIT_CHECK                 do { \
                                                    if (!g_ble_init_flag) \
                                                    { \
                                                        TAILQ_INIT(&g_ble_queue); \
                                                        g_ble_init_flag = 1; \
                                                    } \
                                                } while (0)


typedef enum
{
    STATE_NOT_IN_OTA = 0,
    STATE_IN_OTA
} ota_state_e;

typedef enum
{
    OTA_START = 0,
    OTA_STOP
} ota_event_e;

typedef enum
{
    DATA_RECV = 0,
    DATA_SEND,
} ble_data_e;

typedef enum
{
    CHECK_SUM_XOR = 0,
    CHECK_SUM_PLUS
} ble_checksum_e;

struct ble_entry
{
    ble_event event;
    ble_msg message;
    void* args;
    TAILQ_ENTRY(ble_entry) entry;
};

TAILQ_HEAD(ble_queue, ble_entry);

typedef struct _msg_packet_t
{
    msg_header_t t_header;
    ble_msg_t t_message;
} __attribute__ ((packed)) msg_packet_t;

typedef enum
{
    GOVEE_GATT_IDX_SERVICE = 0,
    GOVEE_GATT_IDX_CHAR1_DECLARATION,
    GOVEE_GATT_IDX_CHAR1_VALUE,
    GOVEE_GATT_IDX_CHAR1_CFG,
    GOVEE_GATT_IDX_CHAR1_USER_DESCRIPTION,
    GOVEE_GATT_IDX_CHAR2_DECLARATION,
    GOVEE_GATT_IDX_CHAR2_VALUE,
    GOVEE_GATT_IDX_CHAR2_CFG,
    GOVEE_GATT_IDX_CHAR2_USER_DESCRIPTION,
    GOVEE_GATT_IDX_NB,
} GOVEE_GATT_INDEX_E;

typedef struct
{
    uint8_t adv_data[BLE_ADV_BUFFER_LEN];
    uint16_t adv_len;
    uint8_t rsp_data[BLE_ADV_BUFFER_LEN];
    uint16_t rsp_len;
    uint8_t dev_data[BLE_DEV_BUFFER_LEN];
    uint8_t dev_len;
    uint8_t adv_init_state;
    uint16_t adv_intv;
} ble_manager_t;


static struct ble_queue g_ble_queue;
static uint16_t g_ble_task_event_id = 0;
static uint8_t g_ble_init_flag = 0;
static ble_manager_t gt_ble_config;
static uint8_t g_ble_update_flag = 0;
static uint8_t g_link_conidx = 0;
static ble_state_e g_ble_conn_state = BLE_STATE_DISCONNECT;
static uint8_t g_gatt_service_id = 0;
static LR_handler g_ble_lr = NULL;
static uint32_t g_heartbeat_tick = 0;
static uint8_t g_mult_package_buffer[BLE_MULT_PACKAGE_MAX_SIZE] = {0};
static uint8_t g_debug_print_type = 0;
static uint8_t g_debug_print_flag = 0;


static gatt_service_t g_govee_gatt_service;

static const uint8_t gatt_char1_desc[GATT_CHAR1_DESC_LEN] = "for gatt Read";
static const uint8_t gatt_char2_desc[GATT_CHAR2_DESC_LEN] = "for gatt Write";
static const uint8_t g_server_uuid[16] = {0x10, 0x19, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
static const gatt_attribute_t g_govee_gatt_profile_att_table[GOVEE_GATT_IDX_NB] =
{
    // Simple gatt Service Declaration
    [GOVEE_GATT_IDX_SERVICE] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_PRIMARY_SERVICE_UUID)},       /* UUID */
        GATT_PROP_WRITE_REQ | GATT_PROP_READ | GATT_PROP_NOTI,      /* Permissions */
        UUID_SIZE_16,                                               /* Max size of the value */     /* Service UUID size in service declaration */
        (uint8_t*)g_server_uuid,                                    /* Value of the attribute */    /* Service UUID value in service declaration */
    },

    //Write
    // Characteristic 1 Declaration
    [GOVEE_GATT_IDX_CHAR1_DECLARATION] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID)},             /* UUID */
        GATT_PROP_READ,                                             /* Permissions */
        0,                                                          /* Max size of the value */
        NULL,                                                       /* Value of the attribute */
    },
    // Characteristic 1 Value
    [GOVEE_GATT_IDX_CHAR1_VALUE] =
    {
        {UUID_SIZE_16, GOVEE_GATT_SVC1_TX_UUID_128},                /* UUID */
        GATT_PROP_READ | GATT_PROP_NOTI,                            /* Permissions */
        GATT_CHAR1_VALUE_LEN,                                       /* Max size of the value */
        NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
    },

    // Characteristic 1 client characteristic configuration
    [GOVEE_GATT_IDX_CHAR1_CFG]  =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID)},       /* UUID */
        GATT_PROP_READ | GATT_PROP_WRITE,                           /* Permissions */
        2,                                                          /* Max size of the value */
        NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
    },
    // Characteristic 1 User Description
    [GOVEE_GATT_IDX_CHAR1_USER_DESCRIPTION] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CHAR_USER_DESC_UUID)},        /* UUID */
        GATT_PROP_READ,                                             /* Permissions */
        GATT_CHAR1_DESC_LEN,                                        /* Max size of the value */
        (uint8_t*)gatt_char1_desc,                                  /* Value of the attribute */
    },

    //Read
    // Characteristic 2 Declaration
    [GOVEE_GATT_IDX_CHAR2_DECLARATION] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID)},             /* UUID */
        GATT_PROP_READ,                                             /* Permissions */
        0,                                                          /* Max size of the value */
        NULL,                                                       /* Value of the attribute */
    },
    // Characteristic 2 Value
    [GOVEE_GATT_IDX_CHAR2_VALUE] =
    {
        {UUID_SIZE_16, GOVEE_GATT_SVC1_RX_UUID_128},                /* UUID */
        GATT_PROP_WRITE | GATT_PROP_READ | GATT_PROP_NOTI,          /* Permissions */
        GATT_CHAR1_VALUE_LEN,                                       /* Max size of the value */
        NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
    },
    // Characteristic 2 client characteristic configuration
    [GOVEE_GATT_IDX_CHAR2_CFG] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID)},       /* UUID */
        GATT_PROP_READ | GATT_PROP_WRITE,                           /* Permissions */
        2,                                                          /* Max size of the value */
        NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
    },
    // Characteristic 2 User Description
    [GOVEE_GATT_IDX_CHAR2_USER_DESCRIPTION] =
    {
        {UUID_SIZE_2, UUID16_ARR(GATT_CHAR_USER_DESC_UUID)},        /* UUID */
        GATT_PROP_READ,                                             /* Permissions */
        GATT_CHAR1_DESC_LEN,                                        /* Max size of the value */
        (uint8_t*)gatt_char2_desc,                                  /* Value of the attribute */
    },
};


static void ble_ota_event_callback(uint8_t event);

static ota_state_e ble_ota_state_get(void)
{
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    return app_otas_get_status();
#else
    /* To be complete */
#endif
}

static void ble_ota_init(void)
{
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    app_ota_init(ble_ota_event_callback);
    ota_gatt_add_service();
#else
        /* To be complete */
#endif
}

static void ble_address_get(mac_addr_t *addr)
{
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    gap_address_get(addr);
#else
    enum ble_addr_type addr_type;

    gap_address_get(addr, &addr_type);
#endif
}

static void ble_heartbeat_update(void)
{
    g_heartbeat_tick = LightSdk_time_system_tick_get();
}

static void ble_data_print(uint8_t* buffer, uint32_t length, ble_data_e data_type)
{
    ble_msg_t* msg = (ble_msg_t*)buffer;
    uint8_t type = 0;

    switch (buffer[0])
    {
        case MSG_SET_HEAD:
            type = BLE_DEBUG_PRINT_SET;
            break;
        case MSG_GET_HEAD:
            type = BLE_DEBUG_PRINT_GET;
            break;
        case MSG_GROUP_CONTROL:
        case MSG_GROUP_CONTROL_V2:
            type = BLE_DEBUG_PRINT_GROUP_CONTROL;
            break;
        case MSG_HARDWARE_TEST:
            type = BLE_DEBUG_PRINT_HARDWARE_TEST;
            break;
        case MSG_MUTL_SET:
            type = BLE_DEBUG_PRINT_MUTL_SET;
            break;
        case MSG_NEW_MUTL_SET:
            type = BLE_DEBUG_PRINT_NEW_MUTL_SET;
            break;
        default:
            break;
    }

    if (type & g_debug_print_type)
    {
        if (type == BLE_DEBUG_PRINT_GET && msg->command == 0x01 && (g_debug_print_flag & BLE_DEBUG_PRINT_NO_HEARTBEAT))
        {
            return;
        }

        HAL_printf("[ble %s]: ", data_type == DATA_RECV ? "recv" : "send");
        LightSdk_byte_print(buffer, length, 0);
    }
}

static uint8_t ble_check_sum(uint8_t* p_data, uint32_t length, ble_checksum_e type)
{
    int i = 0;
    uint8_t check_sum = 0;

    if (p_data != NULL)
    {
        if (type == CHECK_SUM_XOR)
        {
            for (i = 0; i < length; i++)
            {
                check_sum ^= p_data[i];
            }
        }
        else if (type == CHECK_SUM_PLUS)
        {
            for (i = 0; i < length; i++)
            {
                check_sum += p_data[i];
            }
        }
    }

    return check_sum;
}

static void ble_gatt_read_callback(uint8_t* p_read, uint16_t* len, uint16_t att_idx)
{
    //do nothing
}

static void ble_gatt_write_callback(uint8_t* write_buf, uint16_t len, uint16_t att_idx)
{
    msg_header_t t_header;

    if (NULL == write_buf)
    {
        SDK_PRINT(LOG_ERROR, "BLE read data invalid.\r\n");
        return;
    }

    if (Lite_ring_buffer_left_get(g_ble_lr) < sizeof(msg_header_t) + len)
    {
        SDK_PRINT(LOG_WARNING, "Ble ring buffer full.\r\n");
        return;
    }

    memset(&t_header, 0, sizeof(msg_header_t));
    t_header.source = MSG_BLE;
    t_header.length = len;
    Lite_ring_buffer_write_data(g_ble_lr, (uint8_t*)&t_header, sizeof(msg_header_t));
    Lite_ring_buffer_write_data(g_ble_lr, (uint8_t*)write_buf, len);
}

static uint16_t ble_gatt_msg_handle(gatt_msg_t* p_msg)
{
    switch (p_msg->msg_evt)
    {
        case GATTC_MSG_READ_REQ:
            ble_gatt_read_callback((uint8_t*)(p_msg->param.msg.p_msg_data), &(p_msg->param.msg.msg_len), p_msg->att_idx);
            break;

        case GATTC_MSG_WRITE_REQ:
            if (p_msg->att_idx == GOVEE_GATT_IDX_CHAR1_CFG)
            {
                g_ble_conn_state = BLE_STATE_CONNECTED;
                g_link_conidx = p_msg->conn_idx;
            }

            ble_gatt_write_callback((uint8_t*)(p_msg->param.msg.p_msg_data), (p_msg->param.msg.msg_len), p_msg->att_idx);
            break;

        default:
            break;
    }

    return p_msg->param.msg.msg_len;
}

static uint8_t ble_gatt_service_add(gatt_service_t* pt_service)
{
    pt_service->p_att_tb = g_govee_gatt_profile_att_table;
    pt_service->att_nb = GOVEE_GATT_IDX_NB;
    pt_service->gatt_msg_handler = ble_gatt_msg_handle;
    return gatt_add_service(pt_service);
}

static void ble_advertising_start(uint8_t force)
{
    gap_adv_param_t adv_param;
    memset(&adv_param, 0, sizeof(gap_adv_param_t));
    adv_param.adv_mode = GAP_ADV_MODE_UNDIRECT;
    adv_param.disc_mode = GAP_ADV_DISC_MODE_GEN_DISC;
    adv_param.adv_addr_type = GAP_ADDR_TYPE_PUBLIC;
    adv_param.adv_chnl_map = GAP_ADV_CHAN_ALL;
    adv_param.adv_filt_policy = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.adv_intv_min = gt_ble_config.adv_intv;
    adv_param.adv_intv_max = gt_ble_config.adv_intv;
    gap_set_advertising_param(&adv_param);
    gap_set_advertising_data(gt_ble_config.adv_data, gt_ble_config.adv_len);
    gap_set_advertising_rsp_data(gt_ble_config.rsp_data, gt_ble_config.rsp_len);

    if (force)
    {
        SDK_PRINT(LOG_INFOR, "Start advertising...\r\n");
        gap_start_advertising(0);
    }
    else
    {
        if (gt_ble_config.adv_init_state)
        {
            SDK_PRINT(LOG_INFOR, "Start advertising...\r\n");
            gap_start_advertising(0);
        }
    }
}

static void ble_gap_event_callback(gap_event_t* p_event)
{
    ble_event_e event_data = BLE_DISCONNECT;

    switch (p_event->type)
    {
        case GAP_EVT_ADV_END:
        {
            SDK_PRINT(LOG_INFOR, "adv_end, status:0x%02x\r\n", p_event->param.adv_end.status);

            if (g_ble_update_flag == 1)
            {
                g_ble_update_flag = 0;
                ble_advertising_start(1);
            }
        }
        break;

        case GAP_EVT_ALL_SVC_ADDED:
        {
            SDK_PRINT(LOG_INFOR, "All service added\r\n");
            ble_advertising_start(0);
            event_data = BLE_SERVICE_OK;
            LightRuntime_event_task_post(g_ble_task_event_id, (void*)&event_data, sizeof(event_data));
        }
        break;

        case GAP_EVT_SLAVE_CONNECT:
        {
            g_link_conidx = p_event->param.slave_connect.conidx;
            gap_conn_param_update(p_event->param.link_update.conidx, 20, 50, 0, 300);
            ble_heartbeat_update();
            g_ble_conn_state = BLE_STATE_CONNECTED;
            event_data = BLE_CONNECT;
            LightRuntime_event_task_post(g_ble_task_event_id, (void*)&event_data, sizeof(event_data));
            SDK_PRINT(LOG_INFOR, "slave[%d],connect. link_num:%d\r\n", p_event->param.slave_connect.conidx, gap_get_connect_num());
        }
        break;

        case GAP_EVT_DISCONNECT:
        {
            if (g_link_conidx == p_event->param.disconnect.conidx)
            {
                g_ble_conn_state = BLE_STATE_DISCONNECT;
            }

            event_data = BLE_DISCONNECT;
            LightRuntime_event_task_post(g_ble_task_event_id, (void*)&event_data, sizeof(event_data));
            SDK_PRINT(LOG_INFOR, "Link[%d] disconnect, reason:0x%02X\r\n", p_event->param.disconnect.conidx, p_event->param.disconnect.reason);
            ble_advertising_start(1);
        }
        break;

        case GAP_EVT_LINK_PARAM_REJECT:
            SDK_PRINT(LOG_INFOR, "Link[%d]param reject,status:0x%02x\r\n", p_event->param.link_reject.conidx, p_event->param.link_reject.status);
            break;

        case GAP_EVT_LINK_PARAM_UPDATE:
            SDK_PRINT(LOG_INFOR, "Link[%d]param update,interval:%d,latency:%d,timeout:%d\r\n", p_event->param.link_update.conidx
                      , p_event->param.link_update.con_interval, p_event->param.link_update.con_latency, p_event->param.link_update.sup_to);
            break;

        case GAP_EVT_PEER_FEATURE:
            SDK_PRINT(LOG_INFOR, "peer[%d] feats ind\r\n", p_event->param.peer_feature.conidx);
            break;

        case GAP_EVT_MTU:
            SDK_PRINT(LOG_INFOR, "mtu update,conidx=%d,mtu=%d\r\n", p_event->param.mtu.conidx, p_event->param.mtu.value);
            break;

        case GAP_EVT_LINK_RSSI:
            SDK_PRINT(LOG_INFOR, "link rssi %d\r\n", p_event->param.link_rssi);
            break;

        default:
            break;
    }
}

static void ble_link_check(void)
{
    static uint32_t link_check_tick = 0;
    ota_state_e state = ble_ota_state_get();

    if (LightSdk_time_system_tick_get() < link_check_tick)
    {
        ble_heartbeat_update();
    }

    link_check_tick = LightSdk_time_system_tick_get();
    if (link_check_tick - g_heartbeat_tick > 10 * 1000)
    {
        if (state == STATE_NOT_IN_OTA)
        {
            LightService_ble_manager_disconnect();
        }
        else
        {
            ble_heartbeat_update();
        }
    }
}

static int ble_msg_write(uint8_t type, uint8_t command, uint8_t* p_data, uint32_t data_len)
{
    gatt_ntf_t ntf_att;
    ble_msg_t t_message;
    uint8_t check_sum = 0;

    if (data_len > BLE_MSG_DATA_LEN)
    {
        SDK_PRINT(LOG_ERROR, "BLE write data too long.\r\n");
        return -1;
    }

    memset(&t_message, 0, sizeof(ble_msg_t));
    t_message.type = type;
    t_message.command = command;

    if (p_data && data_len > 0)
    {
        memcpy(t_message.data, p_data, data_len);
    }

    check_sum = ble_check_sum((uint8_t*)&t_message, BLE_PKG_DATA_LEN - 1, CHECK_SUM_XOR);
    t_message.check_code = check_sum;
    ntf_att.att_idx = GOVEE_GATT_IDX_CHAR1_VALUE;
    ntf_att.conidx = g_link_conidx;
    ntf_att.svc_id = g_gatt_service_id;
    ntf_att.data_len = sizeof(ble_msg_t);
    ntf_att.p_data = (uint8_t*)&t_message;
    gatt_notification(ntf_att);

    if (g_sdk_log_level >= SDK_LOG_DEBUG)
    {
        if (g_debug_print_flag & BLE_DEBUG_PRINT_RESPONSE)
        {
            ble_data_print((uint8_t*)&t_message, sizeof(ble_msg_t), DATA_SEND);
        }
    }

    return 0;
}

static int ble_msg_read_bytes(uint8_t* buffer, uint32_t length)
{
    uint32_t left = Lite_ring_buffer_size_get(g_ble_lr);

    if (NULL == buffer || length > left)
    {
        return -1;
    }

    memset(buffer, 0, length);
    return Lite_ring_buffer_read_data(g_ble_lr, buffer, length);
}

static void ble_msg_mult_package(msg_header_t* pt_header, uint8_t type, uint8_t command, uint8_t* data)
{
    static uint8_t package_counts = 0;
    static uint8_t total_package = 0;
    uint8_t index = data[0];
    ble_pack_t pack = {0, 0, NULL, 0};
    struct ble_entry* var = NULL;

    if (index == 0x00)
    {
        package_counts = data[1];
        memset(g_mult_package_buffer, 0, sizeof(g_mult_package_buffer));
        total_package = 0;
    }
    else if (index == 0xff)
    {
        if (total_package != package_counts)
        {
            SDK_PRINT(LOG_ERROR, "lost some mult package.\r\n");
            return;
        }

        pack.type = type;
        pack.cmd = command;
        pack.data = g_mult_package_buffer;
        pack.size = 0;
        TAILQ_FOREACH(var, &g_ble_queue, entry)
        {
            if (var->message)
            {
                (*var->message)(*pt_header, pack, var->args);
            }
        }
    }
    else
    {
        total_package++;

        if (total_package != index)
        {
            SDK_PRINT(LOG_ERROR, "Wrong mult package index.\r\n");
            total_package--;
            return;
        }

        memcpy(g_mult_package_buffer + BLE_MULTI_MSG_DATA_LEN * (index - 1), data + 1, BLE_MULTI_MSG_DATA_LEN);
    }
}

static void ble_msg_new_mult_package(msg_header_t* pt_header, uint8_t type, uint8_t* data)
{
    static uint8_t package_counts = 0;
    static uint8_t total_package = 0;
    uint8_t index = data[0];
    uint8_t command = 0;
    ble_pack_t pack = {0, 0, NULL, 0};
    struct ble_entry* var = NULL;

    if (index == 0x00)
    {
        package_counts = data[2];
        memset(g_mult_package_buffer, 0, sizeof(g_mult_package_buffer));
        memcpy(g_mult_package_buffer, data + 3, BLE_NEW_MULTI_DATA_LEN - 2);
        total_package = 0;
    }
    else if (index == 0xff)
    {
        total_package++;

        if (total_package + 1 != package_counts)
        {
            SDK_PRINT(LOG_ERROR, "lost some mult package.\r\n");
            return;
        }

        memcpy(g_mult_package_buffer + BLE_NEW_MULTI_DATA_LEN * total_package - 2, data + 1, BLE_NEW_MULTI_DATA_LEN);
        command = g_mult_package_buffer[0];
        pack.type = type;
        pack.cmd = command;
        pack.data = g_mult_package_buffer + 1;
        pack.size = BLE_NEW_MULTI_DATA_LEN * (total_package + 1) - 2;
        TAILQ_FOREACH(var, &g_ble_queue, entry)
        {
            if (var->message)
            {
                (*var->message)(*pt_header, pack, var->args);
            }
        }
    }
    else
    {
        total_package++;

        if (total_package != index)
        {
            SDK_PRINT(LOG_ERROR, "Wrong mult package index.\r\n");
            total_package--;
            return;
        }

        memcpy(g_mult_package_buffer + BLE_NEW_MULTI_DATA_LEN * index - 2, data + 1, BLE_NEW_MULTI_DATA_LEN);
    }
}

static void ble_msg_handler(msg_packet_t* pt_packet)
{
    msg_header_t* pt_header = &pt_packet->t_header;
    ble_msg_t* pt_msg = &pt_packet->t_message;
    ble_pack_t pack = {0, 0, NULL, 0};
    struct ble_entry* var = NULL;
    uint8_t check_sum = 0;
    check_sum = ble_check_sum((uint8_t*)pt_msg, sizeof(ble_msg_t) - 1, CHECK_SUM_XOR);

    if (check_sum != pt_msg->check_code)
    {
        SDK_PRINT(LOG_DEBUG, "check pkg error: check_sum=%02x, pt_msg->check_code=%02x\r\n", check_sum, pt_msg->check_code);
        return;
    }

    if (pt_msg->type == MSG_SET_HEAD || pt_msg->type == MSG_GET_HEAD || pt_msg->type == MSG_HARDWARE_TEST)
    {
        pack.type = pt_msg->type;
        pack.cmd = pt_msg->command;
        pack.data = pt_msg->data;
        pack.size = BLE_MSG_DATA_LEN;
        TAILQ_FOREACH(var, &g_ble_queue, entry)
        {
            if (var->message)
            {
                (*var->message)(*pt_header, pack, var->args);
            }
        }
    }
    else if (pt_msg->type == MSG_MUTL_SET)
    {
        ble_msg_mult_package(pt_header, pt_msg->type, pt_msg->command, pt_msg->data);
    }
    else if (pt_msg->type == MSG_NEW_MUTL_SET)
    {
        ble_msg_new_mult_package(pt_header, pt_msg->type, (uint8_t*)(pt_msg->data) - 1);
    }
    else
    {
        SDK_PRINT(LOG_ERROR, "msg head error %#02x\r\n", pt_msg->type);
    }
}

static void ble_msg_recieve(void* args)
{
    msg_packet_t t_packet;
    msg_header_t t_header;
    uint8_t buffer[256] = {0};
    ble_pack_t pack = {0, 0, NULL, 0};
    struct ble_entry* var = NULL;
    uint8_t check_sum = 0;

    if (ble_msg_read_bytes((uint8_t*)&t_header, sizeof(msg_header_t)) == 0)
    {
        ble_heartbeat_update();

        if (ble_msg_read_bytes(buffer, t_header.length) == 0)
        {
            if (g_sdk_log_level >= SDK_LOG_DEBUG)
            {
                ble_data_print(buffer, t_header.length, DATA_RECV);
            }

            if (buffer[0] == MSG_GROUP_CONTROL || (buffer[0] & 0xf0) == MSG_GROUP_CONTROL_V2)
            {
                check_sum = ble_check_sum(buffer, t_header.length - 1, CHECK_SUM_PLUS);

                if (check_sum != buffer[t_header.length - 1])
                {
                    SDK_PRINT(LOG_ERROR, "Wrong group data check sum, %d, %d\r\n", check_sum, buffer[t_header.length - 1]);
                    return;
                }

                pack.type = buffer[0];
                pack.data = buffer + 1;
                pack.size = t_header.length - 2;
                TAILQ_FOREACH(var, &g_ble_queue, entry)
                {
                    if (var->message)
                    {
                        (*var->message)(t_header, pack, var->args);
                    }
                }
            }
            else
            {
                memset(&t_packet, 0, sizeof(msg_packet_t));
                memcpy(&t_packet.t_header, &t_header, sizeof(msg_header_t));
                memcpy(&t_packet.t_message, buffer, sizeof(ble_msg_t));
                ble_msg_handler(&t_packet);
            }
        }
    }
    else
    {
        ble_link_check();
    }
}

static void ble_event_task(void* param, void* args)
{
    struct ble_entry* var = NULL;
    ble_event_e event = *(ble_event_e*)param;

    TAILQ_FOREACH(var, &g_ble_queue, entry)
    {
        if (var->event)
        {
            (*var->event)(event, var->args);
        }
    }
}

static void ble_ota_event_callback(uint8_t event)
{
    /* 0 -- OTA START, 1 -- OTA STOP */
    ble_event_e ble_event = BLE_DISCONNECT;

    ble_event = (event == 0) ? BLE_OTA_START : BLE_OTA_OVER;
    LightRuntime_event_task_post(g_ble_task_event_id, (void*)&ble_event, sizeof(ble_event));
}

int LightService_ble_manager_init(ble_config_t* pt_ble, size_t size)
{
    size_t buffer_size = (size == 0) ? BLE_GATT_MSG_BUFFER_SIZE : size;
    mac_addr_t addr;
    gap_security_param_t param =
    {
        .mitm = false,
        .ble_secure_conn = false,
        .io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
        .pair_init_mode = GAP_PAIRING_MODE_WAIT_FOR_REQ,
        .bond_auth = true,
        .password = 0,
    };

    if (pt_ble == NULL)
    {
        SDK_PRINT(LOG_ERROR, "Parameter is null point\r\n");
        return -1;
    }

    memset(&gt_ble_config, 0, sizeof(ble_manager_t));
    memcpy(gt_ble_config.adv_data, pt_ble->p_ble_adv, pt_ble->ble_adv_len);
    memcpy(gt_ble_config.rsp_data, pt_ble->p_ble_resp, pt_ble->ble_resp_len);
    memcpy(gt_ble_config.dev_data, pt_ble->local_name, pt_ble->ble_name_len);
    gt_ble_config.adv_len = pt_ble->ble_adv_len;
    gt_ble_config.rsp_len = pt_ble->ble_resp_len;
    gt_ble_config.dev_len = pt_ble->ble_name_len;
    gt_ble_config.adv_intv = 320;
    gt_ble_config.adv_init_state = pt_ble->adv_init_state;
    gap_set_dev_name(gt_ble_config.dev_data, gt_ble_config.dev_len);
    gap_security_param_init(&param);
    gap_set_cb_func(ble_gap_event_callback);
    gap_bond_manager_init(BLE_BONDING_INFO_SAVE_ADDR, BLE_REMOTE_SERVICE_SAVE_ADDR, 8, true);
    gap_bond_manager_delete_all();
    ble_address_get(&addr);
    SDK_PRINT(LOG_INFOR, "Local BDAddr : 0x%02x%02x%02x%02x%02x%02x\r\n", addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3],
              addr.addr[4], addr.addr[5]);
    g_gatt_service_id = ble_gatt_service_add(&g_govee_gatt_service);
    g_ble_lr = Lite_ring_buffer_init(buffer_size);

    if (g_ble_lr == NULL)
    {
        SDK_PRINT(LOG_ERROR, "BLE ring buffer init failed.\r\n");
        return -1;
    }

    LightRuntime_loop_task_register(ble_msg_recieve, 0, NULL);
    LightRuntime_event_task_register(&g_ble_task_event_id, ble_event_task, NULL);
    BLE_TASK_INIT_CHECK;

    ble_ota_init();

    return 0;
}

int LightService_ble_manager_register(ble_msg msg, ble_event event, void* args)
{
    struct ble_entry* element = NULL;
    BLE_TASK_INIT_CHECK;

    if (NULL == (element = HAL_malloc(sizeof(struct ble_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc ble entry failed.\r\n");
        return -1;
    }

    element->event = event;
    element->message = msg;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_ble_queue, element, entry);
    return 0;
}

void LightService_ble_manager_unregister(ble_msg msg, ble_event event)
{
    struct ble_entry* var = NULL;
    TAILQ_FOREACH(var, &g_ble_queue, entry)
    {
        if (var->event == event && var->message == msg)
        {
            TAILQ_REMOVE(&g_ble_queue, var, entry);
            HAL_free(var);
            break;
        }
    }
}

void LightService_ble_manager_disconnect(void)
{
    gap_disconnect_req(g_link_conidx);
}

int LightService_ble_manager_mac_get(uint8_t* mac)
{
    mac_addr_t addr;

    if (mac == NULL)
    {
        SDK_PRINT(LOG_ERROR, "parameter is null pointer\r\n");
        return -1;
    }

    ble_address_get(&addr);
    memcpy(mac, &addr.addr[0], 6);
    return 0;
}

int LightService_ble_manager_adv_switch(uint8_t on_off)
{
    SDK_PRINT(LOG_INFOR, "BLE advertising <%s>\r\n", on_off ? "ON" : "OFF");

    if (on_off)
    {
        gap_start_advertising(0);
    }
    else
    {
        gap_stop_advertising();
    }

    return 0;
}

int LightService_ble_manager_adv_update(ble_config_t* pt_ble)
{
    memcpy(gt_ble_config.adv_data, pt_ble->p_ble_adv, pt_ble->ble_adv_len);
    gt_ble_config.adv_len = pt_ble->ble_adv_len;
    memcpy(gt_ble_config.rsp_data, pt_ble->p_ble_resp, pt_ble->ble_resp_len);
    gt_ble_config.rsp_len = pt_ble->ble_resp_len;
    memcpy(gt_ble_config.dev_data, pt_ble->local_name, pt_ble->ble_name_len);
    gt_ble_config.dev_len = pt_ble->ble_name_len;
    gap_set_dev_name(gt_ble_config.dev_data, gt_ble_config.dev_len);
    gap_set_advertising_data(gt_ble_config.adv_data, gt_ble_config.adv_len);
    gap_set_advertising_rsp_data(gt_ble_config.rsp_data, gt_ble_config.rsp_len);
    return 0;
}

ble_state_e LightService_ble_manager_state_get(void)
{
    return g_ble_conn_state;
}

int LightService_ble_manager_data_write(uint8_t* buffer, uint32_t length)
{
    gatt_ntf_t ntf_att;
    uint8_t data[128] = {0};
    ble_checksum_e checksum_type = CHECK_SUM_XOR;

    if (!buffer || length == 0 || length >127)
    {
        return -1;
    }

    if (buffer[0] == MSG_GROUP_CONTROL || (buffer[0] & 0xf0) == MSG_GROUP_CONTROL_V2)
    {
        checksum_type = CHECK_SUM_PLUS;
    }
    memcpy(data, buffer, length);
    data[length] = ble_check_sum(data, length, checksum_type);

    ntf_att.att_idx = GOVEE_GATT_IDX_CHAR1_VALUE;
    ntf_att.conidx = g_link_conidx;
    ntf_att.svc_id = g_gatt_service_id;
    ntf_att.data_len = length + 1;
    ntf_att.p_data = data;
    gatt_notification(ntf_att);

    return 0;
}

int LightService_ble_manager_data_save(ble_iot_t iot_data)
{
    msg_packet_t ble_msg;
    int i = 0;

    if (NULL == iot_data.data)
    {
        SDK_PRINT(LOG_ERROR, "BLE read data invalid.\r\n");
        return 0;
    }

    memset(&ble_msg, 0, sizeof(msg_packet_t));

    if (iot_data.size > BLE_PKG_DATA_LEN)
    {
        ble_msg.t_header.flag |= MULTI_PACKAGE;
    }

    for (i = 0; i < iot_data.size / BLE_PKG_DATA_LEN; i++)
    {
        memset(&ble_msg.t_message, 0, sizeof(ble_msg_t));
        ble_msg.t_header.source = iot_data.source;
        ble_msg.t_header.uart_cmd = iot_data.uart_cmd;
        ble_msg.t_header.uart_id = iot_data.uart_id;

        if (i == (iot_data.size / BLE_PKG_DATA_LEN - 1))
        {
            ble_msg.t_header.flag |= MULTI_PACKAGE_END;
        }

        ble_msg.t_header.length = BLE_PKG_DATA_LEN;
        memcpy(&ble_msg.t_message, iot_data.data + i * BLE_PKG_DATA_LEN, BLE_PKG_DATA_LEN);
        Lite_ring_buffer_write_data(g_ble_lr, (uint8_t*)&ble_msg, sizeof(msg_packet_t));
    }

    return 0;
}

int LightService_ble_manager_data_pack(ble_pack_t input, ble_msg_t* output)
{
    uint8_t check_sum = 0;
    memset(output, 0, sizeof(ble_msg_t));
    output->type = input.type;
    output->command = input.cmd;

    if (input.data && input.size > 0)
    {
        memcpy(output->data, input.data, input.size);
    }

    check_sum = ble_check_sum((uint8_t*)output, BLE_PKG_DATA_LEN - 1, CHECK_SUM_XOR);
    output->check_code = check_sum;
    return 0;
}

int LightService_ble_manager_data_response(msg_header_t* pt_header, ble_pack_t data)
{
    static uint8_t multi_package[512] = {0};
    static uint8_t i = 0;
    ble_msg_t t_message;
    uart_data_t uart_data = {0, NULL, 0};
    int ret = -1;

    if (pt_header->source == MSG_BLE)
    {
        ret = ble_msg_write(data.type, data.cmd, data.data, data.size);
    }
    else if (pt_header->source == MSG_UART)
    {
        LightService_ble_manager_data_pack(data, &t_message);

        if ((pt_header->flag & MULTI_PACKAGE) && (data.type != MSG_MUTL_SET))
        {
            memcpy(multi_package + i * BLE_PKG_DATA_LEN, (uint8_t*)&t_message, BLE_PKG_DATA_LEN);
            i++;

            if (pt_header->flag & MULTI_PACKAGE_END)
            {
                uart_data.command = BLE_WIFI_PT_MSG_CMD_RESPONSE;
                uart_data.buffer = multi_package;
                uart_data.size = i * BLE_PKG_DATA_LEN;
                LightService_uart_manager_send(pt_header->uart_id, uart_data);
                memset(multi_package, 0, 256);
                i = 0;
            }
        }
        else
        {
            uart_data.command = BLE_WIFI_PT_MSG_CMD_RESPONSE;
            uart_data.buffer = (uint8_t*)&t_message;
            uart_data.size = sizeof(ble_msg_t);
            LightService_uart_manager_send(pt_header->uart_id, uart_data);
        }
    }

    return ret;
}

void LightService_ble_manager_print_set(uint8_t type, uint8_t flag)
{
    g_debug_print_type = type;
    g_debug_print_flag = flag;
}
