#ifndef _LIGHT_SYSTEM_H_
#define _LIGHT_SYSTEM_H_

typedef enum _sdk_log_level_e
{
    SDK_LOG_CLOSE = 0,       //disable
    SDK_LOG_ERROR,           //error
    SDK_LOG_WARNING,         //warning
    SDK_LOG_INFOR,           //information
    SDK_LOG_DEBUG            //debug
} log_level_e;

int LightSystem_init(log_level_e level);

#endif
