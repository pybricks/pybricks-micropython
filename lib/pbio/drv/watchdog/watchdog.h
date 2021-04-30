// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_WATCHDOG_H_
#define _INTERNAL_PBDRV_WATCHDOG_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_WATCHDOG

/**
 * Initializes and enables the watchdog timer.
 */
void pbdrv_watchdog_init(void);

#else // PBDRV_CONFIG_WATCHDOG

#define pbdrv_watchdog_init()

#endif // PBDRV_CONFIG_WATCHDOG

#endif // _INTERNAL_PBDRV_WATCHDOG_H_
