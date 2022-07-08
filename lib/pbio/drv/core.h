// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal core driver framework functions.

#ifndef _INTERNAL_PBDRV_CORE_H_
#define _INTERNAL_PBDRV_CORE_H_

#include <stdint.h>

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
 * returns true to wait for all drivers to be initialized.
 */
#define pbdrv_init_busy()           (pbdrv_init_busy_count)

// Don't use this directly, use macros above instead.
extern uint32_t pbdrv_init_busy_count;

#endif // _INTERNAL_PBDRV_CORE_H_
