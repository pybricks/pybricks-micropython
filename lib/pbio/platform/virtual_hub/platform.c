// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "../../drv/motor_driver/motor_driver_virtual_simulation.h"

#include <pbio/port_interface.h>
#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = NULL,
    .pin = 0,
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .motor_driver_index = 4,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .motor_driver_index = 5,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
};

#define INFINITY (1e100)

const pbdrv_motor_driver_virtual_simulation_platform_data_t
    pbdrv_motor_driver_virtual_simulation_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 123456,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -142000,
        .endstop_angle_positive = 142000,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .type_id = LEGO_DEVICE_TYPE_ID_NONE,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_S_MOTOR,
        .initial_angle = 0,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .type_id = LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR,
        .initial_angle = 45000,
        .initial_speed = 0,
        .endstop_angle_negative = -INFINITY,
        .endstop_angle_positive = INFINITY,
    },
};
