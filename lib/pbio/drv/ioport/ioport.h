// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_H
#define _INTERNAL_PBDRV_IOPORT_H

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT

void pbdrv_ioport_init(void);

void pbdrv_ioport_deinit(void);

#else // PBDRV_CONFIG_IOPORT

static inline void pbdrv_ioport_init(void) {
}

static inline void pbdrv_ioport_deinit(void) {
}

#endif // PBDRV_CONFIG_IOPORT

#endif // _INTERNAL_PBDRV_IOPORT_H
