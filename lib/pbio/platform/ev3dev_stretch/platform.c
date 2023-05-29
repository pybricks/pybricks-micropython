// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include "../../drv/legodev/legodev_ev3dev.h"

const pbdrv_legodev_ev3dev_motor_platform_data_t pbdrv_legodev_ev3dev_motor_platform_data[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
    },
};

const pbdrv_legodev_ev3dev_sensor_platform_data_t pbdrv_legodev_ev3dev_sensor_platform_data[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR] = {
    {
        .port_id = PBIO_PORT_ID_1,
    },
    {
        .port_id = PBIO_PORT_ID_2,
    },
    {
        .port_id = PBIO_PORT_ID_3,
    },
    {
        .port_id = PBIO_PORT_ID_4,
    },
};
