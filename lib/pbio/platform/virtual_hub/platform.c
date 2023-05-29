// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"
#include "../../drv/legodev/legodev_virtual.h"

#include <pbio/port.h>
#include <pbdrv/config.h>

const pbdrv_legodev_virtual_platform_data_t pbdrv_legodev_virtual_platform_data[PBDRV_CONFIG_LEGODEV_VIRTUAL_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .quadrature_index = 0,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .quadrature_index = 1,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .quadrature_index = 2,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .quadrature_index = 3,
        .type_id = PBDRV_LEGODEV_TYPE_ID_NONE,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .motor_driver_index = 4,
        .quadrature_index = 4,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .motor_driver_index = 5,
        .quadrature_index = 5,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
};

#define INFINITY (1e100)

const pbdrv_motor_driver_virtual_simulation_platform_data_t
    pbdrv_motor_driver_virtual_simulation_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 123456,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -142000,
        .endstop_angle_positive = 142000,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .type_id = PBDRV_LEGODEV_TYPE_ID_NONE,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .type_id = PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 45000,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
};
