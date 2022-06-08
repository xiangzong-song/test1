#ifndef _RUNTIME_H_
#define _RUNTIME_H_
#include <stdint.h>

typedef void (*loop_task)(void* args);
typedef void (*event_task)(void* param, void* args);

void LightRuntime_init(void);
int LightRuntime_loop_task_register(loop_task task, uint32_t interval, void* args);
void LightRuntime_loop_task_unregister(loop_task task);
int LightRuntime_event_task_register(uint16_t* event_id, event_task task, void* args);
int LightRuntime_event_task_post(uint16_t event_id, void* param, int len);

#endif
