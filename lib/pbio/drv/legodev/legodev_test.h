// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_TEST_H_
#define _INTERNAL_PBDRV_LEGODEV_TEST_H_

#include <pbdrv/config.h>

#include <pbio/port.h>
#include <pbdrv/legodev.h>

typedef struct {
    pbio_port_id_t port_id;
    /** Device type ID of the simulated device. */
    pbdrv_legodev_type_id_t type_id;
    uint8_t motor_driver_index;
    uint8_t quadrature_index;
} pbdrv_legodev_test_platform_data_t;

#if PBDRV_CONFIG_LEGODEV_TEST

extern const pbdrv_legodev_test_platform_data_t
    pbdrv_legodev_test_platform_data[PBDRV_CONFIG_LEGODEV_TEST_NUM_DEV];

#endif // PBDRV_CONFIG_LEGODEV_TEST

#endif // _INTERNAL_PBDRV_LEGODEV_TEST_H_
