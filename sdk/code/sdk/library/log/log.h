#ifndef _LOG_H_
#define _LOG_H_

#include <stdint.h>
#include "co_printf.h"
#include "runtime.h"

#define LOG_ERROR           SDK_LOG_ERROR           //error
#define LOG_WARNING         SDK_LOG_WARNING         //warning
#define LOG_INFOR           SDK_LOG_INFOR           //information
#define LOG_DEBUG           SDK_LOG_DEBUG           //debug

extern uint8_t g_sdk_log_level;

#define SDK_PRINT(level, fmt, args...) \
    do \
    { \
        if (g_sdk_log_level >= level) \
        { \
            switch(level) \
            { \
                case LOG_DEBUG: \
                    co_printf("[SDK debug] <%s, %d> "fmt, __FUNCTION__, __LINE__, ##args); \
                    break; \
                case LOG_INFOR: \
                    co_printf("[SDK info] <%s, %d> "fmt, __FUNCTION__, __LINE__, ##args); \
                    break;\
                case LOG_WARNING: \
                    co_printf("[SDK warning] <%s, %d> "fmt, __FUNCTION__, __LINE__, ##args); \
                    break;\
                case LOG_ERROR:\
                    co_printf("[SDK error] <%s, %d> "fmt, __FUNCTION__, __LINE__, ##args); \
                    break;\
                default: \
                    break; \
            } \
        } \
    } \
    while (0)


void LightSdk_log_init(uint8_t level);

#endif

