// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Internal common button functions.

#ifndef _INTERNAL_PBDRV_BUTTON_H_
#define _INTERNAL_PBDRV_BUTTON_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON

/**
 * Initializes the low level button driver.
 */
void pbdrv_button_init(void);

#else

#define pbdrv_button_init()

#endif

#endif // _INTERNAL_PBDRV_BUTTON_H_
