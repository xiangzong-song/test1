#include <stdint.h>
#include "driver_wdt.h"
#include "driver_plf.h"
#include "core_cm3.h"
#include "platform.h"


void LightSdk_watchdog_init(uint8_t time_s)
{
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    wdt_init(WDT_ACT_RST_CHIP, time_s);
    wdt_start();
#endif

#if (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
    NVIC_EnableIRQ(PMU_IRQn);
    wdt_feed();
    wdt_init(WDT_ACT_RST_CHIP, rc_clock, time_s);
    wdt_start();
#endif
}

void LightSdk_watchdog_feed(void)
{
    wdt_feed();
}

void LightSdk_watchdog_deinit(void)
{
    wdt_stop();
}
