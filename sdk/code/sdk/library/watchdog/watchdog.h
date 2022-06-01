#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_
#include <stdint.h>

void LightSdk_watchdog_init(uint8_t time_s);
void LightSdk_watchdog_feed(void);
void LightSdk_watchdog_deinit(void);

#endif
