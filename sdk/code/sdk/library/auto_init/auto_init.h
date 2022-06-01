/*
 * automatic_initialization.h
 *
 *  Created on: 2021-5-14
 *      Author: Hammer
 */

#ifndef __AUTOMATIC_INITIALIZATION_H
#define __AUTOMATIC_INITIALIZATION_H

/*
 * MACROS
 */
#define SECTION(x)  __attribute__((section(x)))
#define USED        __attribute__((used))

/* initialization export */
#define INIT_EXPORT(fn, level) USED const init_fn_t __govee_init_##fn SECTION(".gi_fn." level) = fn


#define INIT_PREV_EXPORT(fn)            INIT_EXPORT(fn, "1")
/* hal initialization */
#define INIT_BOARD_EXPORT(fn)           INIT_EXPORT(fn, "2")
/* device initialization */
#define INIT_DEVICE_EXPORT(fn)          INIT_EXPORT(fn, "3")
/* components initialization (dfs, lwip, ...) */
#define INIT_COMPONENT_EXPORT(fn)       INIT_EXPORT(fn, "4")
/* environment initialization (mount disk, ...) */
#define INIT_ENV_EXPORT(fn)             INIT_EXPORT(fn, "5")
/* appliation initialization (rtgui application etc ...) */
#define INIT_APP_EXPORT(fn)             INIT_EXPORT(fn, "6")

/*
 * TYPEDEFS
 */
typedef int (*init_fn_t)(void);

void LightSdk_auto_init_start(void);

#endif
