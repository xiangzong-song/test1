#ifndef _BLE_MANAGER_H_
#define _BLE_MANAGER_H_

#include <stdint.h>
#include <stdlib.h>

#define BLE_PKG_DATA_LEN                            20
#define BLE_MSG_DATA_LEN                            17

#define BLE_DEBUG_PRINT_NO_HEARTBEAT                0x01
#define BLE_DEBUG_PRINT_RESPONSE                    (0x01 << 1)
#define BLE_DEBUG_PRINT_SET                         0x01
#define BLE_DEBUG_PRINT_GET                         (0x01 << 1)
#define BLE_DEBUG_PRINT_GROUP_CONTROL               (0x01 << 2)
#define BLE_DEBUG_PRINT_HARDWARE_TEST               (0x01 << 3)
#define BLE_DEBUG_PRINT_MUTL_SET                    (0x01 << 4)
#define BLE_DEBUG_PRINT_NEW_MUTL_SET                (0x01 << 5)


typedef enum
{
    BLE_STATE_DISCONNECT = 0,
    BLE_STATE_CONNECTED
} ble_state_e;

typedef enum
{
    MSG_BLE = 0,
    MSG_UART
} msg_source_e;

typedef enum
{
    BLE_SLAVE = 0,
    BLE_MASTER,
} ble_role_e;

typedef enum
{
    BLE_DISCONNECT = 0,
    BLE_CONNECT,
    BLE_SERVICE_OK,
    BLE_OTA_START,
    BLE_OTA_SUSPEND,
    BLE_OTA_OVER
} ble_event_e;

typedef struct _ble_msg_t
{
    uint8_t type;
    uint8_t command;
    uint8_t data[BLE_MSG_DATA_LEN];
    uint8_t check_code;
} __attribute__ ((packed)) ble_msg_t;

typedef struct _msg_header_t
{
    msg_source_e source;
    uint8_t length;
    uint8_t flag;
    uint8_t uart_cmd;
    uint8_t uart_id;
    ble_role_e ble_role;
} __attribute__ ((packed)) msg_header_t;

typedef struct
{
    uint8_t* p_ble_adv;
    uint8_t* p_ble_resp;
    uint8_t* local_name;
    int ble_adv_len;
    int ble_resp_len;
    int ble_name_len;
    uint8_t adv_init_state;
} ble_config_t;

typedef struct
{
    uint8_t type;
    uint8_t cmd;
    uint8_t* data;
    uint32_t size;
} ble_pack_t;

typedef struct _ble_iot_msg_t
{
    msg_source_e source;
    uint8_t uart_cmd;
    uint8_t uart_id;
    uint8_t* data;
    uint32_t size;
} ble_iot_t;

typedef void (*ble_event)(ble_event_e event, void* args);
typedef void (*ble_msg)(msg_header_t header, ble_pack_t pack, void* args);


int LightService_ble_manager_init(ble_config_t* pt_ble, size_t size);
int LightService_ble_manager_register(ble_msg msg, ble_event event, void* args);
void LightService_ble_manager_unregister(ble_msg msg, ble_event event);
void LightService_ble_manager_disconnect(void);
int LightService_ble_manager_mac_get(uint8_t* mac);
int LightService_ble_manager_adv_switch(uint8_t on_off);
int LightService_ble_manager_adv_update(ble_config_t* pt_ble);
ble_state_e LightService_ble_manager_state_get(void);
int LightService_ble_manager_data_write(uint8_t* buffer, uint32_t length);
int LightService_ble_manager_data_pack(ble_pack_t input, ble_msg_t* output);
int LightService_ble_manager_data_save(ble_iot_t iot_data);
int LightService_ble_manager_data_response(msg_header_t* pt_header, ble_pack_t data);
void LightService_ble_manager_print_set(uint8_t type, uint8_t flag);

#endif

