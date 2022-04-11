// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PVDRV_COUNTER_VIRTUAL_H_
#define _INTERNAL_PVDRV_COUNTER_VIRTUAL_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_VIRTUAL

#if !PBDRV_CONFIG_COUNTER_VIRTUAL_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_VIRTUAL_NUM_DEV
#endif

#include <pbdrv/counter.h>

void pbdrv_counter_virtual_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_VIRTUAL

#define pbdrv_counter_virtual_init(devs)

#endif // PBDRV_CONFIG_COUNTER_VIRTUAL

#endif // _INTERNAL_PVDRV_COUNTER_VIRTUAL_H_
