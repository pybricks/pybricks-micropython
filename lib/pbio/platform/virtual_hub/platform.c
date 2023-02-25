// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "../../drv/ioport/ioport_test.h"
#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"

const pbio_iodev_capability_flags_t smart_motor = PBIO_IODEV_CAPABILITY_FLAG_IS_DC_OUTPUT | PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_REL_POS | PBIO_IODEV_CAPABILITY_FLAG_HAS_MOTOR_ABS_POS;

const pbio_iodev_info_t iodev_info[PBDRV_CONFIG_IOPORT_TEST_NUM_PORTS] = {
    // Port A
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
        .capability_flags = smart_motor,
    },
    // Port B
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR,
        .capability_flags = smart_motor,
    },
    // Port C
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
        .capability_flags = smart_motor,
    },
    // Port D
    {
        .type_id = PBIO_IODEV_TYPE_ID_NONE,
    },
    // Port E
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR,
        .capability_flags = smart_motor,
    },
    // Port F
    {
        .type_id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR,
        .capability_flags = smart_motor,
    },
};

const pbdrv_ioport_test_platform_data_t
    pbdrv_ioport_test_platform_data[PBDRV_CONFIG_IOPORT_TEST_NUM_PORTS] = {
    // Port A
    {
        .iodev = {
            .info = &iodev_info[0],
            .mode = PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB,
        },
    },
    // Port B
    {
        .iodev = {
            .info = &iodev_info[1],
            .mode = PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB,
        },
    },
    // Port C
    {
        .iodev = {
            .info = &iodev_info[2],
            .mode = PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB,
        },
    },
    // Port D
    {
        .iodev = {
            .info = &iodev_info[3],
        },
    },
    // Port E
    {
        .iodev = {
            .info = &iodev_info[4],
            .mode = PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB,
        },
    },
    // Port F
    {
        .iodev = {
            .info = &iodev_info[5],
            .mode = PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB,
        },
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
        .endstop_angle_negative = -142000,
        .endstop_angle_positive = 142000,
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
