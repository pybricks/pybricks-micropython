// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_LEGODEV_EV3DEV_H_
#define _INTERNAL_PBDRV_LEGODEV_EV3DEV_H_

#include <pbdrv/config.h>

#include <pbdrv/legodev.h>

#include <pbio/port.h>
#include <pbdrv/legodev.h>

typedef struct {
    pbio_port_id_t port_id;
    uint8_t motor_driver_index;
} pbdrv_legodev_ev3dev_motor_platform_data_t;

typedef struct {
    pbio_port_id_t port_id;
} pbdrv_legodev_ev3dev_sensor_platform_data_t;

#if PBDRV_CONFIG_LEGODEV_EV3DEV

extern const pbdrv_legodev_ev3dev_motor_platform_data_t
    pbdrv_legodev_ev3dev_motor_platform_data[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR];

extern const pbdrv_legodev_ev3dev_sensor_platform_data_t
    pbdrv_legodev_ev3dev_sensor_platform_data[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR];

#endif // PBDRV_CONFIG_LEGODEV_EV3DEV

#endif // _INTERNAL_PBDRV_LEGODEV_EV3DEV_H_
