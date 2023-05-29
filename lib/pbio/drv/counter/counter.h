// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_COUNTER_H_
#define _INTERNAL_PBDRV_COUNTER_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER

void pbdrv_counter_init(void);

#else // PBDRV_CONFIG_COUNTER

#define pbdrv_counter_init()

#endif // PBDRV_CONFIG_COUNTER

#endif // _INTERNAL_PBDRV_COUNTER_H_
