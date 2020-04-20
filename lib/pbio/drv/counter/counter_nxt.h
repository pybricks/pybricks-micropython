// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PVDRV_COUNTER_NXT_H_
#define _PVDRV_COUNTER_NXT_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_NXT

#include "counter.h"

#if !PBDRV_CONFIG_COUNTER_NXT_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_NXT_NUM_DEV
#endif

// defined in counter_nxt.c
extern const pbdrv_counter_drv_t pbdrv_counter_nxt_drv;

#endif // PBDRV_CONFIG_COUNTER_NXT

#endif // _PVDRV_COUNTER_NXT_H_
