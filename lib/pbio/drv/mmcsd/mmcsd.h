// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// MMC/SD controller driver internal interface

#ifndef _INTERNAL_PBDRV_MMCSD_H_
#define _INTERNAL_PBDRV_MMCSD_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MMCSD

void pbdrv_mmcsd_init(void);

#else // PBDRV_CONFIG_MMCSD

#define pbdrv_mmcsd_init()

#endif // PBDRV_CONFIG_MMCSD

#endif // _INTERNAL_PBDRV_MMCSD_H_
