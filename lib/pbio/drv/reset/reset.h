// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RESET_H_
#define _INTERNAL_PBDRV_RESET_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESET

/**
 * Initializes the reset driver.
 */
void pbdrv_reset_init(void);

#else // PBDRV_CONFIG_RESET

#define pbdrv_reset_init()

#endif // PBDRV_CONFIG_RESET

#endif // _INTERNAL_PBDRV_RESET_H_
