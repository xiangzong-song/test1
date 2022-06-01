#ifndef _DEVICE_TIME_H_
#define _DEVICE_TIME_H_

#include <stdint.h>

#define TIME_COUNTING_HARDWARE      0

typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t week;
    uint8_t offset_flag;
    uint8_t hour_offset;
    uint8_t min_offset;
} timeval_t;

typedef void (*time_update_cb)(void*);


int LightSdk_time_set(timeval_t* pt_time);
int LightSdk_time_get(timeval_t* pt_time);
int LightSdk_time_state(void);
int LightSdk_time_is_exceed(uint32_t* tick, uint32_t interval, uint8_t b_update);
int LightSdk_time_update_set(uint32_t update, time_update_cb callback, void* args);
int LightSdk_time_init(uint32_t update, time_update_cb callback, void* args);
uint32_t LightSdk_time_system_tick_get(void);

#endif

