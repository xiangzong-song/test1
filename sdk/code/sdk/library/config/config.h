#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <stdlib.h>

/*************************** flash map **************************
1. system data
0x7D000 - 0x7F000

2. image data (base on "image_size", if image_size = 0x3A000)
0x00000 - 0x73000

3. user data
0x74000 - 0x7C000

*****************************************************************
user data map:
1. easyflash env: 0x74000 - 0x76000 (4K*3)

2. device SKU(default):0x77000(4K)

2. device UUID(default): 0x78000 (4K)

3. device pair information(default): 0x79000 (4K)

4. device aging flag(default): 0x7a000 (4K)

*****************************************************************/


#define DATA_ADDR_DEFAULT                   0x00

typedef enum _config_data_type_e
{
    DEVICE_SKU = 0,
    DEVICE_UUID,
    DEVICE_PAIR_INFO,
    DEVICE_AGING_FLAG,
    DEVICE_CUSTOM_DATA
} config_type_e;

int LightSdk_config_init(void);
int LightSdk_config_env_get(char* key, uint8_t* buffer, uint32_t buf_len, size_t* value_len);
int LightSdk_config_env_set(char* key, uint8_t* value, size_t size);
int LightSdk_config_env_update(char* key, uint8_t* old, size_t old_len, uint8_t* new_default, size_t new_len);
int LightSdk_config_env_delete(char* key);
int LightSdk_config_env_reset(uint8_t reboot);

int LightSdk_config_data_write(config_type_e type, uint32_t addr, uint8_t* data, size_t len);
int LightSdk_config_data_read(config_type_e type, uint32_t addr, uint8_t* data, size_t len);
int LightSdk_config_data_erase(config_type_e type, uint32_t addr, size_t len);

#endif
