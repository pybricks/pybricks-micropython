// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_COUNTER_VIRTUAL_CPYTHON_H_
#define _INTERNAL_PBDRV_COUNTER_VIRTUAL_CPYTHON_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_VIRTUAL_CPYTHON

#if !PBDRV_CONFIG_COUNTER_VIRTUAL_CPYTHON_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_VIRTUAL_CPYTHON_NUM_DEV
#endif

#include <pbdrv/counter.h>

void pbdrv_counter_virtual_cpython_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_VIRTUAL_CPYTHON

#define pbdrv_counter_virtual_cpython_init(devs)

#endif // PBDRV_CONFIG_COUNTER_VIRTUAL_CPYTHON

#endif // _INTERNAL_PBDRV_COUNTER_VIRTUAL_CPYTHON_H_
