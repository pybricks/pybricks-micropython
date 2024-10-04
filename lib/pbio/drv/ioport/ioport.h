// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_H
#define _INTERNAL_PBDRV_IOPORT_H

#include <pbdrv/config.h>
#include <stdbool.h>

#if PBDRV_CONFIG_IOPORT

void pbdrv_ioport_init(void);

#else // PBDRV_CONFIG_IOPORT

static inline void pbdrv_ioport_init(void) {
}

#endif // PBDRV_CONFIG_IOPORT

#endif // _INTERNAL_PBDRV_IOPORT_H
