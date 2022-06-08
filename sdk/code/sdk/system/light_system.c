#include "log.h"
#include "config.h"
#include "device_time.h"

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

        return 0;
    }
    while (0);

    SDK_PRINT(LOG_ERROR, "System init failed.\r\n");

    return -1;
}
