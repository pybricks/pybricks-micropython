// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

#if PBDRV_CONFIG_BLOCK_DEVICE

void pbdrv_block_device_init(void);

#else // PBDRV_CONFIG_BLOCK_DEVICE

static inline void pbdrv_block_device_init(void) {
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_H_
