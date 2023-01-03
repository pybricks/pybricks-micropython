// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Driver that simulates motor driver chips with a dc motor attached to it.

#ifndef _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_
#define _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#include <stdint.h>

#include <pbdrv/motor_driver.h>

void pbdrv_motor_driver_virtual_simulation_get_angle(pbdrv_motor_driver_dev_t *dev, int32_t *rotations, int32_t *millidegrees);

#endif // PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#endif // _INTERNAL_PBDRV_MOTOR_DRIVER_VIRTUAL_SIMULATION_H_
