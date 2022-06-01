#include <stdint.h>
#include "log.h"

uint8_t g_sdk_log_level = 0;

void LightSdk_log_init(uint8_t level)
{
    g_sdk_log_level = level;
}
