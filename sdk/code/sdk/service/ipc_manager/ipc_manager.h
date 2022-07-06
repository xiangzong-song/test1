#ifndef _IPC_MANAGER_H_
#define _IPC_MANAGER_H_
#include <stdint.h>

typedef void (*ipc_msg)(uint8_t, uint8_t*, size_t, void*);

int LightService_ipc_manager_send(uint8_t *data, uint16_t len);
int LightService_ipc_manager_register(ipc_msg callback, void* args);
void LightService_ipc_manager_unregister(ipc_msg callback);
int LightService_ipc_manager_init(void);
void LightService_ipc_manager_deinit(void);

#endif
