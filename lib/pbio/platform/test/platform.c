// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "../../drv/ioport/ioport_test.h"
#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"

const pbdrv_ioport_test_platform_data_t
    pbdrv_ioport_test_platform_data[PBDRV_CONFIG_IOPORT_TEST_NUM_PORTS] = {
    // Port A
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    // Port B
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
    },
    // Port C
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
    // Port D
    {
        .type_id = PBIO_IODEV_TYPE_ID_NONE,
    },
    // Port E
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR,
    },
    // Port F
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
    },
};

#define INFINITY (1e100)

const pbdrv_motor_driver_virtual_simulation_platform_data_t
    pbdrv_motor_driver_virtual_simulation_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .initial_angle = 123456,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .initial_angle = 45000,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
};
