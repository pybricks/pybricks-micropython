// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_H_
#define _INTERNAL_PBDRV_RPROC_H_

#include <stdbool.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RPROC

/**
 * Initializes "remote" coprocessors early during the startup sequence
 */
void pbdrv_rproc_init(void);

/**
 * Determines whether remote coprocessors have been initialized
 */
bool pbdrv_rproc_is_ready(void);

#else // PBDRV_CONFIG_RPROC

#define pbdrv_rproc_init()
#define pbdrv_rproc_is_ready()  true

#endif // PBDRV_CONFIG_RPROC

#endif // _INTERNAL_PBDRV_RPROC_H_
