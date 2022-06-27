#include <stdint.h>
#include "driver_wdt.h"

void LightSdk_watchdog_init(uint8_t time_s)
{
    wdt_init(WDT_ACT_RST_CHIP, time_s);
    wdt_start();
}

void LightSdk_watchdog_feed(void)
{
    wdt_feed();
}

void LightSdk_watchdog_deinit(void)
{
    wdt_stop();
}
