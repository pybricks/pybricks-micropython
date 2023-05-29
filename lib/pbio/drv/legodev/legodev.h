// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_H
#define _INTERNAL_PBDRV_LEGODEV_H

#include <pbdrv/config.h>
#include <pbdrv/legodev.h>

#if PBDRV_CONFIG_LEGODEV

void pbdrv_legodev_init(void);

#else // PBDRV_CONFIG_LEGODEV

static inline void pbdrv_legodev_init(void) {
}

#endif // PBDRV_CONFIG_LEGODEV

#endif // _INTERNAL_PBDRV_LEGODEV_H
