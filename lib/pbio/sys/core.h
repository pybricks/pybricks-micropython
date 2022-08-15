// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBSYS_SYS_CORE_H_
#define _PBSYS_SYS_CORE_H_

#include <stdint.h>

/**
 * Increases system init or deinit reference count.
 *
 * Modules that have async (de)init must call this when (de)initialization is
 * started to indicate that (de)init is still busy.
 */
#define pbsys_init_busy_up()        (pbsys_init_busy_count++)

/**
 * Decreases system init or deinit reference count.
 *
 * Modules that have async (de)init must call this when (de)initialization is
 * finished to indicate that (de)init is no longer busy.
 */
#define pbsys_init_busy_down()      (pbsys_init_busy_count--)

/**
 * Tests if system init or deinitialization is still busy.
 *
 * After calling pbsys_init(), the Contiki event loop should run until this
 * returns true to wait for all drivers to be (de)initialized.
 */
#define pbsys_init_busy()           (pbsys_init_busy_count)

// Don't use this directly, use macros above instead.
extern uint32_t pbsys_init_busy_count;

#endif // _PBSYS_SYS_CORE_H_
