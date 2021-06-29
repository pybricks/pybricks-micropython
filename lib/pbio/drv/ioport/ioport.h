// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_H_
#define _INTERNAL_PBDRV_IOPORT_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT

#include "ioport_lpf2.h"

/**
 * Initializes the ioport driver.
 */
static inline void pbdrv_ioport_init(void) {
    pbdrv_ioport_lpf2_init();
}

#else // PBDRV_CONFIG_IOPORT

#define pbdrv_ioport_init()

#endif // PBDRV_CONFIG_IOPORT

#endif // _INTERNAL_PBDRV_IOPORT_H_
