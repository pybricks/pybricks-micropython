// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Driver that simulates motor driver chips with a dc motor attached to it.

#ifndef _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_
#define _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#include <stdint.h>

#include <pbio/dcmotor.h>

#include <pbdrv/motor_driver.h>

/**
 * Description of virtual motor environment.
 */
typedef struct {
    /** Port identifier. */
    pbio_port_id_t port_id;
    /** Initial angle of the motor (mdeg). */
    double initial_angle;
    /** Initial speed of the motor (mdeg/s). */
    double initial_speed;
    /** Location of physical endstop in negative direction (mdeg). */
    double endstop_angle_negative;
    /** Location of physical endstop in positive direction (mdeg). */
    double endstop_angle_positive;
} pbdrv_motor_driver_virtual_simulation_platform_data_t;

extern const pbdrv_motor_driver_virtual_simulation_platform_data_t
    pbdrv_motor_driver_virtual_simulation_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

void pbdrv_motor_driver_virtual_simulation_get_angle(pbdrv_motor_driver_dev_t *dev, int32_t *rotations, int32_t *millidegrees);

#if !PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION_AUTO_START
void pbdrv_motor_driver_init_manual(void);
#endif

#endif // PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#endif // _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_
