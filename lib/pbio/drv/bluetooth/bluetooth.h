// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal common bluetooth functions.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_H_
#define _INTERNAL_PBDRV_BLUETOOTH_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH

void pbdrv_bluetooth_init();

#else // PBDRV_CONFIG_BLUETOOTH

#define pbdrv_bluetooth_init()

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
