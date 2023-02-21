// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_TEST_H_
#define _INTERNAL_PBDRV_IOPORT_TEST_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_TEST

#include <pbio/iodev.h>

typedef struct {
    pbio_iodev_t iodev;
} pbdrv_ioport_test_platform_data_t;

extern const pbdrv_ioport_test_platform_data_t
    pbdrv_ioport_test_platform_data[PBDRV_CONFIG_IOPORT_TEST_NUM_PORTS];

#endif // PBDRV_CONFIG_IOPORT_TEST

#endif // _INTERNAL_PBDRV_IOPORT_TEST_H_
