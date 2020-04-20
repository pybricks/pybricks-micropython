// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PVDRV_COUNTER_EV3DEV_STRETCH_IIO_H_
#define _PVDRV_COUNTER_EV3DEV_STRETCH_IIO_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#include "counter.h"

#if !PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV
#endif

// defined in counter_ev3dev_stretch_iio.c
extern const pbdrv_counter_drv_t pbdrv_counter_ev3dev_stretch_iio_drv;

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#endif // _PVDRV_COUNTER_EV3DEV_STRETCH_IIO_H_
