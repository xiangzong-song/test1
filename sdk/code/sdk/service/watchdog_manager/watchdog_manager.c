#include <string.h>
#include "watchdog.h"
#include "runtime.h"

#define WATCHDOG_TIMEOUT_MIN        3

static void wathdog_feed_task(void* args)
{
    LightSdk_watchdog_feed();
}

int LightService_watchdog_manager_init(uint8_t time_s)
{
    uint8_t timeout = (time_s < WATCHDOG_TIMEOUT_MIN) ? WATCHDOG_TIMEOUT_MIN : time_s;

    LightSdk_watchdog_init(timeout);
    LightRuntime_loop_task_register(wathdog_feed_task, timeout / 2, NULL);

    return 0;
}

void LightService_watchdog_manager_deinit(void)
{
    LightRuntime_loop_task_unregister(wathdog_feed_task);
    LightSdk_watchdog_deinit();
}

