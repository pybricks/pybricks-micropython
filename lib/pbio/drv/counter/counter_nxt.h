// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PVDRV_COUNTER_NXT_H_
#define _PVDRV_COUNTER_NXT_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_NXT

#if !PBDRV_CONFIG_COUNTER_NXT_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_NXT_NUM_DEV
#endif

#include <pbdrv/counter.h>

void pbdrv_counter_nxt_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_NXT

#define pbdrv_counter_nxt_init(devs)

#endif // PBDRV_CONFIG_COUNTER_NXT

#endif // _PVDRV_COUNTER_NXT_H_
