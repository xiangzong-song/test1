#include <string.h>
#include "log.h"
#include "config.h"
#include "device_time.h"

#define LIGHT_SYSTEM_VERSION        "1.00.01"


int LightSystem_init(log_level_e level)
{
    int ret = 0;

    do
    {
        LightSdk_log_init(level);
        ret = LightSdk_config_init();
        if (ret != 0)
        {
            SDK_PRINT(LOG_ERROR, "system config init failed.\r\n");
            break;
        }

        ret = LightSdk_time_init(0, NULL, NULL);
        if (ret != 0)
        {
            SDK_PRINT(LOG_ERROR, "system time init failed.\r\n");
            break;
        }

        HAL_printf("SDK platform : %s, version : %s\r\n", PLATFORM_TYPE, LIGHT_SYSTEM_VERSION);

        return 0;
    }
    while (0);

    SDK_PRINT(LOG_ERROR, "System init failed.\r\n");

    return -1;
}

int LightSystem_information_get(char* platform, char* version)
{
    if (NULL == platform || NULL == version)
    {
        SDK_PRINT(LOG_ERROR, "Parameter point is null.\r\n");
        return -1;
    }

    strcpy(platform, PLATFORM_TYPE);
    strcpy(version, LIGHT_SYSTEM_VERSION);

    return 0;
}
