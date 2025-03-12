// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_DISPLAY_H_
#define _INTERNAL_PBDRV_DISPLAY_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

#if PBDRV_CONFIG_DISPLAY

void pbdrv_display_init(void);

#else // PBDRV_CONFIG_DISPLAY

static inline void pbdrv_display_init(void) {
}

#endif // PBDRV_CONFIG_DISPLAY

#endif // _INTERNAL_PBDRV_DISPLAY_H_
