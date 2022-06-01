#include <stdio.h>
#include <string.h>
#include "os_timer.h"
#include "driver_timer.h"
#include "driver_system.h"
#include "device_time.h"
#include "log.h"
#include "runtime.h"


static timeval_t g_time_value;
static uint8_t g_time_set_flag = 0;

static uint32_t g_update_cycle = 0;
static time_update_cb g_callback_func = NULL;
static void* g_callback_param = NULL;
static uint16_t g_time_task_event_id = 0;


static void time_update(void* args);

#if TIME_COUNTING_HARDWARE
__attribute__((section("ram_code"))) void timer0_isr_ram(void)
{
    static uint32_t count = 0;

    timer_clear_interrupt(TIMER0);

    if (++count >= 10)
    {
        time_update(NULL);
        count = 0;
    }
}
#endif

static void time_update(void* args)
{
    static uint32_t update_count = 0;

    if (g_time_set_flag)
    {
        g_time_value.sec++;
        if (g_time_value.sec == 60)
        {
            g_time_value.sec = 0;
            g_time_value.min++;
            if (g_time_value.min == 60)
            {
                g_time_value.min = 0;
                g_time_value.hour++;
                if (g_time_value.hour == 24)
                {
                    g_time_value.hour = 0;
                    g_time_value.week++;
                    if (g_time_value.week > 7)
                    {
                        g_time_value.week = 1;
                    }
                }
            }
        }

        if (g_update_cycle && ++update_count % g_update_cycle == 0 && g_callback_func)
        {
            LightRuntime_event_task_post(g_time_task_event_id, NULL, 0);
            update_count = 0;
        }
    }
    else
    {
        if (++update_count % 2 == 0 && g_callback_func)
        {
            LightRuntime_event_task_post(g_time_task_event_id, NULL, 0);
            update_count = 0;
        }
    }
}

static void time_event_task(void* param, void* args)
{
    if (g_callback_func)
    {
        (*g_callback_func)(g_callback_param);
    }
}

int LightSdk_time_set(timeval_t* pt_time)
{
    if (pt_time)
    {
        memcpy(&g_time_value, pt_time, sizeof(timeval_t));
        g_time_set_flag = 1;
        SDK_PRINT(LOG_INFOR, "Set time <%d:%d:%d %d>.\r\n", pt_time->hour, pt_time->min, pt_time->sec, pt_time->week);
    }

    return 0;
}

int LightSdk_time_get(timeval_t* pt_time)
{
    if (!g_time_set_flag)
    {
        return -1;
    }

    if (pt_time)
    {
        memcpy(pt_time, &g_time_value, sizeof(timeval_t));
    }

    return 0;
}

int LightSdk_time_state(void)
{
    return g_time_set_flag;
}

int LightSdk_time_is_exceed(uint32_t* tick, uint32_t interval, uint8_t b_update)
{
    uint32_t now = system_get_curr_time();
    int ret = 0;

    if (*tick == 0)
    {
        *tick = now;
        return 0;
    }

    if (now < *tick)
    {
        *tick = now;
        ret = 1;
    }
    else
    {
        if (now - *tick >= interval)
        {
            ret = 1;
            if (b_update)
            {
                *tick = now;
            }
        }
    }

    return ret;
}

int LightSdk_time_update_set(uint32_t update, time_update_cb callback, void* args)
{
    g_update_cycle = update;
    g_callback_func = callback;
    g_callback_param = args;

    return 0;
}

int LightSdk_time_init(uint32_t update, time_update_cb callback, void* args)
{
#if TIME_COUNTING_HARDWARE
    timer_init(TIMER0, 100*1000, TIMER_PERIODIC);
    timer_run(TIMER0);

    NVIC_SetPriority(TIMER0_IRQn, 2);
    NVIC_EnableIRQ(TIMER0_IRQn);
#else
    static os_timer_t timer;

    memset(&timer, 0, sizeof(os_timer_t));
    os_timer_init(&timer, time_update, NULL);
    os_timer_start(&timer, 1000, 1);
#endif

    g_update_cycle = update;
    g_callback_func = callback;
    g_callback_param = args;

    LightRuntime_event_task_register(&g_time_task_event_id, time_event_task, NULL);

    return 0;
}

uint32_t LightSdk_time_system_tick_get(void)
{
    return system_get_curr_time();
}

