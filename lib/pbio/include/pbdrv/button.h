// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup ButtonDriver Driver: Button
 * @{
 */

#ifndef _PBDRV_BUTTON_H_
#define _PBDRV_BUTTON_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#if PBDRV_CONFIG_BUTTON

/**
 * Get bitmask indicating currently pressed buttons.
 *
 * @return                  Bitmask indicating which buttons are pressed
 */
pbio_button_flags_t pbdrv_button_get_pressed(void);

#else

static inline pbio_button_flags_t pbdrv_button_get_pressed(void) {
    return 0;
}

#endif

#endif // _PBDRV_BUTTON_H_

/** @} */
