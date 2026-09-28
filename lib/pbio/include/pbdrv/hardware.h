// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors


#ifndef _PBDRV_HARDWARE_H_
#define _PBDRV_HARDWARE_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_HARDWARE

/**
 * Gets a user facing description of the hub hardware version.
 *
 * Hardware detection may rely on peripherals that are not up yet, so the
 * result can change during boot. It is not meant to be cached.
 *
 * @return  The hardware version, or @c NULL if it is not known.
 */
const char *pbdrv_hardware_get_version(void);

#else // PBDRV_CONFIG_HARDWARE

static inline const char *pbdrv_hardware_get_version(void) {
    return NULL;
}

#endif // PBDRV_CONFIG_HARDWARE

#endif // _PBDRV_HARDWARE_H_
