// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_COUNTER_EV3DEV_STRETCH_IIO_H_
#define _INTERNAL_PBDRV_COUNTER_EV3DEV_STRETCH_IIO_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#if !PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV
#error Platform must define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV
#endif

#include <pbdrv/counter.h>

void pbdrv_counter_ev3dev_stretch_iio_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#define pbdrv_counter_ev3dev_stretch_iio_init(devs)

#endif // PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO

#endif // _INTERNAL_PBDRV_COUNTER_EV3DEV_STRETCH_IIO_H_
