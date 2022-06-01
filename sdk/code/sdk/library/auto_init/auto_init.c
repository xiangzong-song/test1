/*
 *  auto_init.c
 *
 *  Created on: 2021-5-14
 *      Author: Hammer
 */

/*
 * INCLUDES
 */
#include "auto_init.h"

/*
 * LOCAL VARIABLES
 */


/*
 * FUNCTIONS
 */

static int gi_start(void)
{
    return 0;
}
INIT_EXPORT(gi_start, "0");

static int gi_board_start(void)
{
    return 0;
}
INIT_EXPORT(gi_board_start, "0.end");

static int gi_board_end(void)
{
    return 0;
}
INIT_EXPORT(gi_board_end, "1.end");

static int gi_end(void)
{
    return 0;
}
INIT_EXPORT(gi_end, "6.end");


void LightSdk_auto_init_start(void)
{
    const init_fn_t *fn_ptr;

    for (fn_ptr = &__govee_init_gi_board_start; fn_ptr < &__govee_init_gi_end; fn_ptr++)
    {
        (*fn_ptr)();
    }
}
