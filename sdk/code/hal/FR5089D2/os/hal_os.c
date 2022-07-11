#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#include "os_mem.h"
#include "hal_os.h"


int HAL_printf(const char* format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int HAL_sprintf(char* out, const char* format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vsprintf(out, format, args);
    va_end(args);

    return(ret);
}

void *HAL_memcpy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

void* HAL_memset(void* dest, int c, size_t len)
{
    return memset(dest, c, len) ;
}

void HAL_free(void* ptr)
{
    return os_free(ptr);
}

void* HAL_malloc(size_t size)
{
    return os_malloc(size);
}

void* HAL_realloc(void*ptr, size_t new_size)
{
    return os_realloc(ptr,new_size);
}

void* HAL_calloc(size_t count,size_t size)
{
    return os_calloc(count,size);
}

uint16_t HAL_free_heap_get(void)
{
    return os_get_free_heap_size();
}
