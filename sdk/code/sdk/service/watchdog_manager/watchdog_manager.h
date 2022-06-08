#ifndef _WATCHDOG_MANGER_H_
#define _WATCHDOG_MANGER_H_
#include <stdint.h>


int LightService_watchdog_manager_init(uint8_t time_s);
void LightService_watchdog_manager_deinit(void);

#endif
