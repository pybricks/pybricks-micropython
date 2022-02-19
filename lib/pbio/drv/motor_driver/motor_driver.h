// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Motor driver chip internal types and functions.

#ifndef _INTERNAL_PBDRV_MOTOR_DRIVER_H_
#define _INTERNAL_PBDRV_MOTOR_DRIVER_H_

#include <pbdrv/config.h>
#include <pbdrv/led.h>
#include <pbio/color.h>
#include <pbio/error.h>

#if PBDRV_CONFIG_MOTOR_DRIVER

void pbdrv_motor_driver_init(void);

#else // PBDRV_CONFIG_MOTOR_DRIVER

#define pbdrv_motor_driver_init()

#endif // PBDRV_CONFIG_MOTOR_DRIVER

#endif // _INTERNAL_PBDRV_MOTOR_DRIVER_H_
