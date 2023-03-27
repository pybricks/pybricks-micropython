// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

// Random number generator drivers

#ifndef _INTERNAL_PBDRV_RANDOM_H_
#define _INTERNAL_PBDRV_RANDOM_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RANDOM

void pbdrv_random_init(void);

#else // PBDRV_CONFIG_RANDOM

#define pbdrv_random_init()

#endif // PBDRV_CONFIG_RANDOM

#endif // _INTERNAL_PBDRV_RANDOM_H_
