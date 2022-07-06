#include "os_task.h"
#include "sys_queue.h"
#include "hal_os.h"
#include "log.h"
#include "runtime.h"
#include "device_time.h"
#include "platform.h"


struct loop_task_entry
{
    loop_task task;
    uint32_t tick;
    uint32_t interval;
    void* args;
    TAILQ_ENTRY(loop_task_entry) entry;
};

struct event_task_entry
{
    uint16_t event_id;
    event_task task;
    void* args;
    TAILQ_ENTRY(event_task_entry) entry;
};

TAILQ_HEAD(loop_task_queue, loop_task_entry);

TAILQ_HEAD(event_task_queue, event_task_entry);


static struct loop_task_queue g_loop_task;
static struct event_task_queue g_event_task;
static uint16_t g_runtime_task = 0xff;


static void runtime_loop_task(void)
{
    struct loop_task_entry* var = NULL;

    TAILQ_FOREACH(var, &g_loop_task, entry)
    {
        if (var->task && LightSdk_time_is_exceed(&var->tick, var->interval, 1))
        {
            (*var->task)(var->args);
        }
    }
}

static int runtime_event_task(os_event_t *event)
{
    struct event_task_entry* var = NULL;

    TAILQ_FOREACH(var, &g_event_task, entry)
    {
        if (var->event_id == event->event_id && var->task)
        {
            (*var->task)(event->param, var->args);
            break;
        }
    }

    return EVT_CONSUMED;
}

void LightRuntime_init(void)
{
    TAILQ_INIT(&g_loop_task);
    TAILQ_INIT(&g_event_task);

    g_runtime_task = os_task_create(runtime_event_task);

#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    os_user_loop_event_set(runtime_loop_task);
#endif
}

int LightRuntime_loop_task_register(loop_task task, uint32_t interval, void* args)
{
    struct loop_task_entry* element = NULL;

    if (NULL == (element = HAL_malloc(sizeof(struct loop_task_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc loop task entry failed.\r\n");
        return -1;
    }

    element->task = task;
    element->tick = 0;
    element->interval = interval;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_loop_task, element, entry);

    return 0;
}

void LightRuntime_loop_task_unregister(loop_task task)
{
    struct loop_task_entry* var = NULL;

    TAILQ_FOREACH(var, &g_loop_task, entry)
    {
        if (var->task == task)
        {
            TAILQ_REMOVE(&g_loop_task, var, entry);
            HAL_free(var);
            break;
        }
    }
}

int LightRuntime_event_task_register(uint16_t* event_id, event_task task, void* args)
{
    static uint16_t task_event_id = 0;
    struct event_task_entry* element = NULL;

    if (task_event_id++ >= 0xffff)
    {
        SDK_PRINT(LOG_ERROR, "Event task register full.\r\n");
        return -1;
    }

    if (NULL == (element = HAL_malloc(sizeof(struct event_task_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc event task entry failed.\r\n");
        return -1;
    }

    *event_id = task_event_id;

    element->event_id = task_event_id;
    element->task = task;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_event_task, element, entry);

    return 0;
}

int LightRuntime_event_task_post(uint16_t event_id, void* param, int len)
{
    os_event_t task_event;

    task_event.event_id = event_id;
    task_event.param = param;
    task_event.param_len = len;

    os_msg_post(g_runtime_task, &task_event);

    return 0;
}

void LightRuntime_loop_handler(void)
{
    runtime_loop_task();
}
