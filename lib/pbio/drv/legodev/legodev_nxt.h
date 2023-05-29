// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_NXT_H_
#define _INTERNAL_PBDRV_LEGODEV_NXT_H_

#include <pbdrv/config.h>

#include <pbdrv/legodev.h>

#include <pbio/port.h>
#include <pbdrv/legodev.h>

typedef struct {
    pbio_port_id_t port_id;
    uint8_t motor_driver_index;
} pbdrv_legodev_nxt_motor_platform_data_t;

typedef struct {
    pbio_port_id_t port_id;
} pbdrv_legodev_nxt_sensor_platform_data_t;

#if PBDRV_CONFIG_LEGODEV_NXT

extern const pbdrv_legodev_nxt_motor_platform_data_t
    pbdrv_legodev_nxt_motor_platform_data[PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR];

extern const pbdrv_legodev_nxt_sensor_platform_data_t
    pbdrv_legodev_nxt_sensor_platform_data[PBDRV_CONFIG_LEGODEV_NXT_NUM_SENSOR];

#endif // PBDRV_CONFIG_LEGODEV_NXT

#endif // _INTERNAL_PBDRV_LEGODEV_NXT_H_
