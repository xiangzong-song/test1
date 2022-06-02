#include "hal_os.h"
#include <string.h>
#include <stdio.h>
#include "CppUTest/TestHarness_c.h"
int HAL_printf(const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int ret =vprintf(format, arglist);
    va_end(arglist);
    return ret;
}

int HAL_sprintf(char *out, const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int ret =sprintf(out,format,arglist);
    va_end(arglist);
    return ret;
}

void *HAL_memcpy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}
void  *HAL_memset(void*dest, int c, size_t len)
{
    return memset(dest,c,len) ;
}
void HAL_free(void * ptr)
{
    return cpputest_free(ptr);
}
void *HAL_malloc(size_t size)
{
    return cpputest_malloc(size);
}
void* HAL_realloc(void*ptr, size_t new_size)
{
    return cpputest_realloc(ptr,new_size);
}
void *HAL_calloc(size_t count,size_t size)
{
    return cpputest_calloc(count,size);
}