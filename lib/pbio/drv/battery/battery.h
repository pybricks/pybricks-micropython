// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal common battery functions.

#ifndef _INTERNAL_PBDRV_BATTERY_H_
#define _INTERNAL_PBDRV_BATTERY_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY

void pbdrv_battery_init();

#else // PBDRV_CONFIG_BATTERY

#define pbdrv_battery_init()

#endif // PBDRV_CONFIG_BATTERY

#endif // _INTERNAL_PBDRV_BATTERY_H_
