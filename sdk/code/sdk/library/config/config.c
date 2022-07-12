#include <stdio.h>
#include <string.h>
#include "driver_system.h"
#include "driver_flash.h"
#include "sys_utils.h"
#include "easyflash.h"
#include "log.h"
#include "config.h"
#include "platform.h"


#define CONFIG_INIT_KEY             "INIT"
#define CONFIG_INIT_VAL             "GOVEE"
#define CONFIG_RESET_VAL            "RESET"


int LightSdk_config_init(void)
{
    int ret = 0;
    char data[8] = {0};
    size_t len = 0;

    ret = easyflash_init();
    if (ret != 0)
    {
        SDK_PRINT(LOG_ERROR, "Easy Flash init failed. ret = %d\r\n", ret);
        return -1;
    }

    if (ef_load_env() != 0)
    {
        SDK_PRINT(LOG_ERROR, "ef load env failed. ret = %d\r\n", ret);
        return -1;
    }

    LightSdk_config_env_get(CONFIG_INIT_KEY, (uint8_t*)data, sizeof(data), &len);
    if (strncmp(data, CONFIG_INIT_VAL, strlen(CONFIG_INIT_VAL)) != 0)
    {
        SDK_PRINT(LOG_ERROR, "Config data need reset.\r\n", ret);
        ef_env_set_default();
    }

    return 0;
}

int LightSdk_config_env_get(char* key, uint8_t* buffer, uint32_t buf_len, size_t* value_len)
{
    ef_get_env_blob(key, buffer, buf_len, value_len);

    return 0;
}

int LightSdk_config_env_set(char* key, uint8_t* value, size_t size)
{
    return ef_set_env_blob(key, value, size);
}

int LightSdk_config_env_update(char* key, uint8_t* old, size_t old_len, uint8_t* new_default, size_t new_len)
{
    if (new_len < old_len)
    {
        SDK_PRINT(LOG_ERROR, "invalid parameters, old length : %d, new lenght : %d\r\n", old_len, new_len);
        return -1;
    }

    memcpy(new_default, old, old_len);

    return ef_set_env_blob(key, new_default, new_len);
}

int LightSdk_config_env_delete(char* key)
{
    return ef_del_env(key);
}

int LightSdk_config_env_reset(uint8_t reboot)
{
    LightSdk_config_env_set(CONFIG_INIT_KEY, (uint8_t*)CONFIG_RESET_VAL, sizeof(CONFIG_RESET_VAL));

    if (reboot)
    {
        co_delay_100us(100);
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
        platform_reset_patch(0);
#endif
    }

    return 0;
}

int LightSdk_config_data_write(config_type_e type, uint32_t addr, uint8_t* data, size_t len)
{
    uint32_t address[4] = {DEVICE_SKU_ADDR_DEFAULT, DEVICE_UUID_ADDR_DEFAULT, DEVICE_PAIR_DATA_ADDR_DEFAULT, DEVICE_AGING_FLAG_ADDR_DEFAULT};
    uint32_t addr_base = 0x00;
    uint8_t buffer[64] = {0};
    size_t check_size = (len > 64) ? 64 : len;

    if (type < DEVICE_CUSTOM_DATA)
    {
        addr_base = (addr == DATA_ADDR_DEFAULT) ? address[type] : addr;
        if (addr_base < address[0] || addr_base >= 0x7d000)
        {
            SDK_PRINT(LOG_ERROR, "Wrong flash address : %08x.\r\n", addr_base);
            return -1;
        }
    }
    else
    {
        if (addr == DATA_ADDR_DEFAULT)
        {
            SDK_PRINT(LOG_ERROR, "Wrong flash address : %08x.\r\n", addr);
            return -1;
        }
        addr_base = addr;
    }

    flash_write(addr_base, (uint32_t)len, data);
    flash_read(addr_base, (uint32_t)check_size, buffer);
    if (memcmp(buffer, data, check_size) != 0)
    {
        SDK_PRINT(LOG_ERROR, "Check flash data failed.\r\n");
        return -1;
    }

    return 0;
}

int LightSdk_config_data_read(config_type_e type, uint32_t addr, uint8_t* data, size_t len)
{
    uint32_t address[4] = {DEVICE_SKU_ADDR_DEFAULT, DEVICE_UUID_ADDR_DEFAULT, DEVICE_PAIR_DATA_ADDR_DEFAULT, DEVICE_AGING_FLAG_ADDR_DEFAULT};
    uint32_t addr_base = 0x00;

    if (type == DEVICE_CUSTOM_DATA && addr == DATA_ADDR_DEFAULT)
    {
        SDK_PRINT(LOG_ERROR, "Wrong flash address : %08x.\r\n", addr_base);
        return -1;
    }
    addr_base = (addr == DATA_ADDR_DEFAULT) ? address[type] : addr;

    flash_read(addr_base, (uint32_t)len, data);

    return 0;
}

int LightSdk_config_data_erase(config_type_e type, uint32_t addr, size_t len)
{
    uint32_t address[4] = {DEVICE_SKU_ADDR_DEFAULT, DEVICE_UUID_ADDR_DEFAULT, DEVICE_PAIR_DATA_ADDR_DEFAULT, DEVICE_AGING_FLAG_ADDR_DEFAULT};
    uint32_t addr_base = 0x00;

    if (len % 0x1000 != 0)
    {
        SDK_PRINT(LOG_ERROR, "Wrong erase size : %08x.\r\n", len);
        return -1;
    }

    if (addr != DATA_ADDR_DEFAULT)
    {
        if (addr % 0x1000 != 0 || addr < DEVICE_SKU_ADDR_DEFAULT || addr >= BLE_BONDING_INFO_ADDR)
        {
            SDK_PRINT(LOG_ERROR, "Wrong erase flash address : %08x.\r\n", addr);
            return -1;
        }

        addr_base = addr;
    }
    else
    {
        if (type == DEVICE_CUSTOM_DATA)
        {
            SDK_PRINT(LOG_ERROR, "Wrong erase flash address : %08x.\r\n", addr);
            return -1;
        }
        addr_base = address[type];
    }

    flash_erase(addr_base, len);

    return 0;
}

