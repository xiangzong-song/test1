#include <string.h>
#include <stdint.h>
#include <string.h>
#include "driver_iomux.h"
#include "driver_pmu.h"
#include "os_task.h"
#include "os_mem.h"
#include "button.h"
#include "sys_queue.h"
#include "driver_system.h"
#include "button_manager.h"
#include "log.h"


#define BUTTON_COUNT_MAX                    8
#define BUTTON_TASK_INIT_CHECK              do { \
                                            	if (!g_button_init_flag) \
                                                { \
                                                    TAILQ_INIT(&g_button_queue); \
                                                    g_button_init_flag = 1; \
                                                } \
                                            } while (0)

struct button_entry
{
    button_event event;
    void* args;
    TAILQ_ENTRY(button_entry) entry;
};

TAILQ_HEAD(button_queue, button_entry);


static struct button_queue g_button_queue;
static uint8_t g_button_init_flag = 0;

static button_config_t gt_button_table[BUTTON_COUNT_MAX];
static uint16_t g_button_task = 0xff;
static uint8_t g_button_count = 0;


static const char *g_button_type_str[] =
{
    "BUTTON_PRESSED",
    "BUTTON_RELEASED",
    "BUTTON_SHORT_PRESSED",
    "BUTTON_MULTI_PRESSED",
    "BUTTON_LONG_PRESSED",
    "BUTTON_LONG_PRESSING",
    "BUTTON_LONG_RELEASED",
    "BUTTON_LONG_LONG_PRESSED",
    "BUTTON_LONG_LONG_RELEASED",
    "BUTTON_COMB_PRESSED",
    "BUTTON_COMB_RELEASED",
    "BUTTON_COMB_SHORT_PRESSED",
    "BUTTON_COMB_LONG_PRESSED",
    "BUTTON_COMB_LONG_PRESSING",
    "BUTTON_COMB_LONG_RELEASED",
    "BUTTON_COMB_LONG_LONG_PRESSED",
    "BUTTON_COMB_LONG_LONG_RELEASED",
};


static int button_task_process(os_event_t *param)
{
    struct button_msg_t *button_msg = NULL;
    button_data_t button_evt;
    int8_t button_id = -1;
    uint8_t button_type = 0;
    uint8_t i = 0;
    struct button_entry* var = NULL;

    if (param->event_id != USER_EVT_BUTTON || NULL == (struct button_msg_t *)param->param)
    {
        return 0;
    }

    button_msg = (struct button_msg_t *)param->param;
    SDK_PRINT(LOG_DEBUG, "button_type : %d, button_index : 0x%08x, button_count : %d\r\n",
        button_msg->button_type, button_msg->button_index, button_msg->button_cnt);

    for (i = 0; i < sizeof(gt_button_table)/sizeof(button_config_t); i++)
    {
        if (button_msg->button_index & GPIO2PORT(gt_button_table[i].port, gt_button_table[i].bits))
        {
            button_id = gt_button_table[i].button_id;
            button_type = button_msg->button_type;
            break;
        }
    }

    if (button_id >= 0)
    {
        SDK_PRINT(LOG_DEBUG, "<%d> %s.\r\n", button_id, g_button_type_str[button_type]);
        button_evt.button_id = button_id;
        button_evt.button_value = button_msg->button_index;
        button_evt.type = (enum button_type_t)button_type;
        button_evt.count = button_msg->button_cnt;

        TAILQ_FOREACH(var, &g_button_queue, entry)
        {
            if (var->event)
            {
                (*var->event)(button_evt, var->args);
            }
        }
    }

    return 0;
}

int LightService_button_manager_init(button_config_t* buttons, int count)
{
    uint32_t button_mask = 0;
    int i = 0;

    if (count > BUTTON_COUNT_MAX)
    {
        SDK_PRINT(LOG_ERROR, "Button count beyond capcity.\r\n");
        return -1;
    }

    g_button_task = os_task_create(button_task_process);
    memset(gt_button_table, 0, sizeof(button_config_t)*BUTTON_COUNT_MAX);
    memcpy(gt_button_table, buttons, sizeof(button_config_t)*count);
    g_button_count = count;

    for (i = 0; i < g_button_count; i++)
    {
        pmu_set_pin_to_PMU(gt_button_table[i].port, 1 << gt_button_table[i].bits);
        pmu_set_port_mux(gt_button_table[i].port, gt_button_table[i].bits, PMU_PORT_MUX_KEYSCAN);
        pmu_set_pin_pull(gt_button_table[i].port, 1 << gt_button_table[i].bits, true);
        button_mask |= GPIO2PORT(gt_button_table[i].port, gt_button_table[i].bits);
    }

    pmu_port_wakeup_func_set(button_mask);
    button_init(button_mask, g_button_task);

    BUTTON_TASK_INIT_CHECK;

    return 0;
}

int LightService_button_manager_register(button_event event, void* args)
{
    struct button_entry* element = NULL;

    BUTTON_TASK_INIT_CHECK;

    if (NULL == (element = HAL_malloc(sizeof(struct button_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc button entry failed.\r\n");
        return -1;
    }

    element->event = event;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_button_queue, element, entry);

    return 0;
}

void LightService_button_manager_unregister(button_event event)
{
    struct button_entry* var = NULL;

    TAILQ_FOREACH(var, &g_button_queue, entry)
    {
        if (var->event == event)
        {
            TAILQ_REMOVE(&g_button_queue, var, entry);
            HAL_free(var);
            break;
        }
    }
}

void LightService_button_manager_deinit(void)
{
    struct button_entry* var = NULL;

    TAILQ_FOREACH(var, &g_button_queue, entry)
    {
        if (var)
        {
            TAILQ_REMOVE(&g_button_queue, var, entry);
            HAL_free(var);
        }
    }

    /* Warning : Not all resource be free */
}

