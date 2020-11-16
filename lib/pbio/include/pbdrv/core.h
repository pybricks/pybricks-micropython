// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup CoreDriver Driver: Core
 *
 * Core driver framework functions.
 *
 * @{
 */

#ifndef _PBDRV_CORE_H_
#define _PBDRV_CORE_H_

void pbdrv_init();
void pbdrv_deinit();

/**
 * Increases driver init reference count.
 *
 * Driver that have async init must call this when initialization is started
 * to indicate that init is still busy.
 */
#define pbdrv_init_busy_up()        (pbdrv_init_busy_count++)

/**
 * Decreases driver init reference count.
 *
 * Driver that have async init must call this when initialization is finished
 * to indicate that init is no longer busy.
 */
#define pbdrv_init_busy_down()      (pbdrv_init_busy_count--)

/**
 * Tests if driver initialization is still busy.
 *
 * After calling pbdrv_init(), the Contiki event loop should run until this
 * returns true to wait for all drivers to be initalized.
 */
#define pbdrv_init_busy()           (pbdrv_init_busy_count)

/**
 * Increases driver deinit reference count.
 *
 * Driver that have async deinit must call this when deinitialization is started
 * to indicate that deinit is still busy.
 */
#define pbdrv_deinit_busy_up()      (pbdrv_deinit_busy_count++)

/**
 * Decreases driver deinit reference count.
 *
 * Driver that have async deinit must call this when deinitialization is finished
 * to indicate that deinit is no longer busy.
 */
#define pbdrv_deinit_busy_down()    (pbdrv_deinit_busy_count--)

/**
 * Tests if driver deinitialization is still busy.
 *
 * After calling pbdrv_deinit(), the Contiki event loop should run until this
 * returns true to wait for all drivers to be deinitalized.
 */
#define pbdrv_deinit_busy()         (pbdrv_deinit_busy_count)

/** @cond INTERNAL */
extern uint32_t pbdrv_init_busy_count;
extern uint32_t pbdrv_deinit_busy_count;
/** @endcond */

#endif // _PBDRV_CORE_H_

/** @} */
