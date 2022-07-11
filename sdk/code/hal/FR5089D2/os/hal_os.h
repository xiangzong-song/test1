#ifndef _HAL_OS_H_
#define _HAL_OS_H_

#include <unistd.h>
#include <stdint.h>

int HAL_printf(const char* format, ...);
int HAL_sprintf(char* out, const char* format, ...);
void *HAL_memcpy(void *dest, const void *src, size_t size);
void* HAL_memset(void* dest, int c, size_t len);
void HAL_free(void* ptr);
void* HAL_malloc(size_t size);
void* HAL_realloc(void*ptr, size_t new_size);
void* HAL_calloc(size_t count,size_t size);
uint16_t HAL_free_heap_get(void);

#endif
